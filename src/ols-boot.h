/* 
 * Part of ols-fwloader - Diolan bootloader interface
 *
 * Copyright (C) 2011 Michal Demin <michal.demin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BOOT_h_
#define BOOT_h_

#include <config.h>
#include <stdint.h>
#if IS_WIN32
#include <wtypes.h>
#include <windows.h>
#include <ddk/hidsdi.h>
#include <setupapi.h>
#else
#include <libusb.h>
#endif

#define OLS_VID         0x04d8
#define OLS_PID         0xfc90

#define OLS_TIMEOUT     1000
#define OLS_PAGE_SIZE   64
#define OLS_READ_SIZE   (sizeof(rsp.read_flash.data))

#define OLS_FLASH_SIZE  0x3400 // 16*0x400 - 3*0x400
#define OLS_FLASH_ADDR  0x0800 // protect bootloader

#define OLS_FLASH_TOTSIZE 0x4000

enum {
	OLS_WRITE_FLUSH = 1,
	OLS_WRITE_2BYTE = 2,
};

struct ols_boot_t {
#if IS_WIN32
	HANDLE hDevice;
#else
	libusb_context *ctx;
	libusb_device_handle *dev;
#endif
	int attach;

	uint8_t cmd_id;
};

struct ols_boot_t *BOOT_Init(uint16_t vid, uint16_t pid);
uint8_t BOOT_Version(struct ols_boot_t *ob);
uint8_t BOOT_Read(struct ols_boot_t *ob, uint16_t addr, uint8_t *buf, uint16_t size);
uint8_t BOOT_Write(struct ols_boot_t *ob, uint16_t addr, uint8_t *buf, uint16_t size);
uint8_t BOOT_Erase(struct ols_boot_t *ob);
uint8_t BOOT_Reset(struct ols_boot_t *ob);
void BOOT_Deinit(struct ols_boot_t *ob);
#endif

