#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "boot_if.h"
#include "ols-boot.h"

struct ols_boot_t *BOOT_Init(uint16_t vid, uint16_t pid)
{
#if IS_WIN32
	GUID HidGuid;
	HDEVINFO hDevInfo;
	SP_DEVICE_INTERFACE_DATA DevInterfaceData;
	unsigned long DevIndex = 0;
	unsigned long DetailsSize;
	PSP_INTERFACE_DEVICE_DETAIL_DATA pDetails;
	HANDLE hHidDevice;
	HIDD_ATTRIBUTES Attr;
#endif

	struct ols_boot_t *ob;
	int ret;

	ob = malloc(sizeof(struct ols_boot_t));
	if (ob == NULL) {
		fprintf(stderr, "Not enough memory \n");
		return NULL;
	}
	memset(ob, 0, sizeof(struct ols_boot_t));

#if IS_WIN32
	HidD_GetHidGuid( &HidGuid);
	hDevInfo = SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
	if (hDevInfo == INVALID_HANDLE_VALUE)
	{
		fprintf(stderr, "INVALID_HANDLE_VALUE\n");
		return NULL;
	}
	DevInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	// iterate thgough all HID devices
	// this part is inspirated by Diolan's fw_update
	while(1)
	{
		int bad = 0;

		if (!SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &HidGuid, DevIndex, &DevInterfaceData)) {
			fprintf(stderr, "Device does not exist\n");
			return NULL;
		}

		SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, NULL, 0, &DetailsSize, NULL);
		pDetails = malloc(DetailsSize);
		if (pDetails == NULL)
		{
			SetupDiDestroyDeviceInfoList(hDevInfo);
			fprintf(stderr, "Not enough memory \n");
			return NULL;
		}

		pDetails->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		if (!SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevInterfaceData, pDetails, DetailsSize, NULL, NULL))
		{
			free(pDetails);
			SetupDiDestroyDeviceInfoList(hDevInfo);
			fprintf(stderr, "SetupDiGetDeviceInterfaceDetail failed\n");
			return NULL;
		}

		hHidDevice = CreateFile(pDetails->DevicePath,	GENERIC_READ | GENERIC_WRITE,	FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);

		if (hHidDevice == INVALID_HANDLE_VALUE) {
			bad = 1;
		}

		if ((bad == 0) && (!HidD_GetAttributes (hHidDevice, &Attr))) {
			bad = 2;
		}

		if ((bad == 0) && (Attr.VendorID == vid) && (Attr.ProductID == pid)) {
			ob->hDevice = hHidDevice;
			free(pDetails);
			break;
		}

		if (bad > 1) {
			CloseHandle(hHidDevice);
		}

		free(pDetails);
		DevIndex++;
	}
	SetupDiDestroyDeviceInfoList(hDevInfo);
#else
	ret = libusb_init(&ob->ctx);
	if (ret != 0) {
		fprintf(stderr, "libusb_init proobem\n");
	}

	libusb_set_debug(ob->ctx, 4);
	ob->dev = libusb_open_device_with_vid_pid(ob->ctx, vid, pid);
	if (ob->dev == NULL) {
		fprintf(stderr, "Device not found\n");
		free(ob);
		return NULL;
	}

	if (libusb_kernel_driver_active(ob->dev, 0)) {
		// reattach later
		ob->attach = 1;
		if (libusb_detach_kernel_driver(ob->dev, 0)) {
			fprintf(stderr, "Error detaching kernel driver \n");
			free(ob);
			return NULL;
		}
	}

	ret = libusb_claim_interface(ob->dev, 0);
	if (ret != 0) {
		fprintf(stderr, "Cannot claim device\n");
	}

	ret = libusb_set_interface_alt_setting(ob->dev, 0, 0);
	if (ret != 0) {
		fprintf(stderr, "Unable to set alternative interface \n");
	}
#endif

	return ob;
}

static uint8_t BOOT_Recv(struct ols_boot_t *ob, boot_rsp *rsp)
{
#if IS_WIN32
	OVERLAPPED read_over;
	unsigned long readed;
	unsigned char read_buf[sizeof(boot_rsp)+1];
	unsigned long res;

	memset(&read_over, 0, sizeof(OVERLAPPED));

	read_over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if ((!ReadFile(ob->hDevice, read_buf, sizeof(boot_rsp)+1, &readed, &read_over))	&& (GetLastError() != ERROR_IO_PENDING)) {
		fprintf(stderr, "LastError = %d\n", GetLastError());
		return 3;
	}
	if (WAIT_TIMEOUT == (res = WaitForSingleObject(read_over.hEvent, 5000)))
	{
		fprintf(stderr, "Write timeout\n");
		return 2;
	}
	if ((!GetOverlappedResult(ob->hDevice, &read_over, &readed, FALSE)) || (readed != sizeof(boot_rsp) + 1)) {
		fprintf(stderr, "LastError = %d, readed = %d\n", GetLastError(), readed);
		return 1;
	}
	memcpy(rsp, read_buf + 1, sizeof(boot_rsp));
	CloseHandle(read_over.hEvent);
	return 0;
#else
	int ret;
	int len;

	memset (rsp, 0, sizeof(boot_rsp));
	ret = libusb_interrupt_transfer(ob->dev, 0x81, (uint8_t *)rsp, sizeof(boot_rsp), &len, OLS_TIMEOUT);
	if ((ret == 0) && (len == sizeof(boot_rsp))) {
		return 0;
	} else if ((ret == 0) && (len != sizeof(boot_rsp))) {
		fprintf(stderr, "Transfered too little (%d)\n", len);
		return 1;
	} else if (ret == LIBUSB_ERROR_TIMEOUT) {
		fprintf(stderr, "Com timeout\n");
		return 2;
	} else if (ret == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "Error sending, not ols ?\n");
		return 3;
	} else if (ret == LIBUSB_ERROR_NO_DEVICE) {
		fprintf(stderr, "Device disconnected \n");
		return 4;
	}
	fprintf(stderr, "Other error - recv \n");
	return 5;
#endif
}

static uint8_t BOOT_Send(struct ols_boot_t *ob, boot_cmd *cmd)
{
#if IS_WIN32
	unsigned char write_buf[sizeof(boot_cmd)+1];
	unsigned long written;
	OVERLAPPED write_over;
	BOOL write_res;

	memset(&write_over, 0, sizeof(OVERLAPPED));
	write_over.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	write_buf[0] = 0;
	memcpy(write_buf + 1, cmd, sizeof(boot_cmd));

	write_res = WriteFile(ob->hDevice, write_buf, sizeof(boot_cmd) + 1, &written, &write_over);
	if ((!write_res) && (GetLastError() != ERROR_IO_PENDING)) {
		fprintf(stderr, "LastError = %d\n", GetLastError());
		return 2;
	}
	if (WaitForSingleObject(write_over.hEvent, OLS_TIMEOUT) == WAIT_TIMEOUT) {
		fprintf(stderr, "Write timeout");
		return 1;
	}
	CloseHandle(write_over.hEvent);
	return 0;
#else
	int ret;

	ret = libusb_control_transfer(ob->dev, 0x21, 0x09, 0x0000, 0x0000, (uint8_t *)cmd, sizeof(boot_cmd), OLS_TIMEOUT);
	if (ret == sizeof(boot_cmd)) {
		return 0;
	} else if (ret == LIBUSB_ERROR_TIMEOUT) {
		fprintf(stderr, "Com timeout\n");
		return 2;
	} else if (ret == LIBUSB_ERROR_PIPE) {
		fprintf(stderr, "Error sending, not ols ?\n");
		return 3;
	} else if (ret == LIBUSB_ERROR_NO_DEVICE) {
		fprintf(stderr, "Device disconnected \n");
		return 4;
	}
	fprintf(stderr, "Other error \n");
	return 5;
#endif
}

static uint8_t BOOT_SendRecv(struct ols_boot_t *ob, boot_cmd *cmd, boot_rsp *rsp)
{
	int ret;

	ret = BOOT_Send(ob, cmd);

	if (ret != 0)
		return ret;

	ret = BOOT_Recv(ob, rsp);

	if (ret != 0)
		return ret;

	// check echo byte
	if (cmd->header.echo != rsp->header.echo) {
		fprintf(stderr, "Id doesn't match. Bootloader error\n");
		return 1;
	}
	return 0;
}

uint8_t BOOT_Version(struct ols_boot_t *ob)
{
	boot_cmd cmd;
	boot_rsp rsp;
	int ret;

	memset(&cmd, 0, sizeof(cmd));

	cmd.header.cmd = BOOT_GET_FW_VER;

	ret = BOOT_SendRecv(ob, &cmd, &rsp);

	if (ret) {
		return 1;
	}

	printf("Bootloader version %d.%d.%d\n", rsp.get_fw_ver.major,
		rsp.get_fw_ver.minor, rsp.get_fw_ver.sub_minor);

	return 0;
}

uint8_t BOOT_Read(struct ols_boot_t *ob, uint16_t addr, uint8_t *buf, uint16_t size)
{
	// todo loop
	boot_cmd cmd;
	boot_rsp rsp;
	uint16_t address = 0;
	uint16_t len;
	int ret;

	memset(&cmd, 0, sizeof(cmd));

	while (size > 0) {
		len = (size > OLS_READ_SIZE) ? OLS_READ_SIZE : size;
		cmd.header.cmd = BOOT_READ_FLASH;
		cmd.header.echo = ob->cmd_id ++;

		cmd.read_flash.addr_hi = (address >> 8) & 0xff;
		cmd.read_flash.addr_lo = address & 0xff;
		cmd.read_flash.size8 = (len%2)?len+1:len; // round if odd

		ret = BOOT_SendRecv(ob, &cmd, &rsp);
		if (ret != 0) {
			fprintf(stderr, "Error reading memory\n");
			return 1;
		}
		memcpy(buf, rsp.read_flash.data, len);
		buf += len;
		size -= len;
		address += len;
	}
	return 0;
}

uint8_t BOOT_Write(struct ols_boot_t *ob, uint16_t addr, uint8_t *buf, uint16_t size_in)
{
	// todo loop
	boot_cmd cmd;
	boot_rsp rsp;
	uint16_t address = 0;
	uint16_t len;
	int32_t size = size_in;
	int flush = 1;
	int ret;

	address = addr;

	while (size > 0) {
		memset(&cmd, 0, sizeof(cmd));
#if OLS_PAGE_SIZE == 2
		len = (size > OLS_PAGE_SIZE) ? OLS_PAGE_SIZE : size;
		len = (len % 2) ? len + 1: len; // round odd

		memcpy(cmd.write_flash.data, buf, len);
		// command bootloader to only write 2 bytes
		cmd.write_flash.flush = OLS_WRITE_2BYTE | OLS_WRITE_FLUSH;
#elif OLS_PAGE_SIZE == 64
		flush ^= 1; // toggle flush

		len = (size > (OLS_PAGE_SIZE/2)) ? OLS_PAGE_SIZE/2 : size;

		memset(cmd.write_flash.data, 0xff, sizeof(cmd.write_flash.data));
		memcpy(cmd.write_flash.data, buf, len);

		// in this mode we always write 32 bytes
		// rest is padded with 0xff
		len = OLS_PAGE_SIZE/2;
		// command bootloader to write 64bytes
		cmd.write_flash.flush = flush & OLS_WRITE_FLUSH;
#else
#error "Unsupported page size"
#endif

		cmd.header.cmd = BOOT_WRITE_FLASH;
		cmd.header.echo = ob->cmd_id ++;

		cmd.write_flash.addr_hi = (address >> 8) & 0xff;
		cmd.write_flash.addr_lo = address & 0xff;
		cmd.write_flash.size8 = len;

		if (address < OLS_FLASH_ADDR) {
			fprintf(stderr, "Protecting bootloader - skip @0x%04x\n", address);
		} if (address + len >= OLS_FLASH_ADDR + OLS_FLASH_SIZE) {
			fprintf(stderr, "Protecting bootloader - skip @0x%04x\n", address);
			// we end
			break;
		} else {
			ret = BOOT_SendRecv(ob, &cmd, &rsp);
		}
		if (ret != 0) {
			fprintf(stderr, "Error writing memory\n");
			return 1;
		}

		buf += len;
		size -= len;
		address += len;
	}
	return 0;
}

uint8_t BOOT_Erase(struct ols_boot_t *ob)
{
	// todo loop
	boot_cmd cmd;
	boot_rsp rsp;
	uint16_t address = 0;
	int ret;

	memset(&cmd, 0, sizeof(cmd));

	cmd.header.cmd = BOOT_ERASE_FLASH;
	cmd.header.echo = ob->cmd_id ++;

	cmd.erase_flash.addr_hi = 0x08;//(address >> 8) & 0xff;
	cmd.erase_flash.addr_lo = 0x00;//address & 0xff;
	cmd.erase_flash.size_x64 = 0x0d;//64;

	ret = BOOT_SendRecv(ob, &cmd, &rsp);
	if (ret != 0) {
		fprintf(stderr, "Error erasing memory\n");
	}
	return ret;
}

uint8_t BOOT_Reset(struct ols_boot_t *ob)
{
	boot_cmd cmd;

	memset(&cmd, 0, sizeof(cmd));

	cmd.header.cmd = BOOT_RESET;
	BOOT_Send(ob, &cmd);

	// device wont exist after reset
	ob->attach = 0;
}


void BOOT_Deinit(struct ols_boot_t *ob)
{
#if IS_WIN32
	CloseHandle(ob->hDevice);
	ob->hDevice = INVALID_HANDLE_VALUE;
#else
	libusb_release_interface(ob->dev, 0);

	if (ob->attach) {
		if (libusb_attach_kernel_driver(ob->dev, 0)) {
			fprintf(stderr, "Unable to reattach kernel driver\n");
		}
	}

	libusb_close(ob->dev);
	libusb_exit(ob->ctx);

	ob->dev = NULL;
	ob->ctx = NULL;
#endif
}

