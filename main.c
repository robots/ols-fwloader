/*
 * OLS flash loader
 *
 * 2011 - Michal Demin
 *
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ols-boot.h"
#include "data_file.h"

enum {
	CMD_READ = 1,
	CMD_WRITE = 2,
	CMD_VERIFY = 4,
	CMD_ERASE = 8
};

enum {
	TYPE_BIN,
	TYPE_HEX,
};

void usage()
{
	printf("ols-fwloader [-V] [-W] [-R] [-E] [-r rfile] [-w wfile] [-t type] [-v vid] [-p pid]\n\n");
	printf("  -V      - veriy Flash against wfile\n");
	printf("  -E      - erase flash\n");
	printf("  -W      - erase and write flash with wfile\n");
	printf("  -R      - read flash to rfile\n");
	printf("  -p pid  - Set usb PID\n");
	printf("  -v vid  - Set usb VID\n");
	printf("  -t type - File type (BIN/HEX) (default: BIN)\n");

	printf("  -w file - file to be read and written to flash\n");
	printf("  -r file - file where the flash content should be written to\n");
	printf("\n\n");
	printf("Write firmware with verification:\n");
	printf(" ols-fwloader -V -W -w firmware.hex -t HEX \n");
	printf("Read whole flash:\n");
	printf(" ols-fwloader -R -r firmware.hex\n");
	printf("\n");

}

int main(int argc, char** argv)
{
	struct ols_boot_t *ob;

	uint8_t *bin_buf;
	uint8_t *bin_buf_tmp;

	uint16_t vid = OLS_VID;
	uint16_t pid = OLS_PID;

	int error = 0;
	int i;

	// aguments
	struct file_ops_t *fo;
	char *file_write = NULL;
	char *file_read = NULL;
	uint8_t file_type = TYPE_BIN;
	uint8_t cmd = 0;

	// getopt
	int opt;

	fo = GetFileOps("BIN");

	// parse args
	while ((opt = getopt(argc, argv, "WRVEr:w:v:p:t:h")) != -1) {
		switch (opt) {
			case 'h':
				usage();
				exit(1);
				break;
			case 'R':
				cmd |= CMD_READ;
				break;
			case 'W':
				cmd |= CMD_WRITE;
				break;
			case 'E':
				cmd |= CMD_ERASE;
				break;
			case 'V':
				cmd |= CMD_VERIFY;
				break;
			case 'v': // vid
				vid = atoi(optarg);
				break;
			case 'p': // vid
				pid = atoi(optarg);
				break;
			case 'r': // readfile
				if (file_read != NULL) {
					fprintf(stderr, "Two files?\n");
					exit(-1);
				}
				file_read = strdup(optarg);
				break;
			case 'w': // readfile
				if (file_write != NULL) {
					fprintf(stderr, "Two files?\n");
					exit(-1);
				}
				file_write = strdup(optarg);
				break;
			case 't':
				fo = GetFileOps(optarg);
				if (fo == NULL) {
					fprintf(stderr, "Unknown type \n");
					exit(-1);
				}
				break;
		}
	}

	// check parameters
	if ((cmd & CMD_READ) && (file_read == NULL)) {
		fprintf(stderr, "Read command but no file ? \n");
		error = 1;
	}

	if ((cmd & CMD_WRITE) && (file_write == NULL)) {
		fprintf(stderr, "Write command but no file ? \n");
		error = 1;
	}

	if ((cmd & CMD_VERIFY) && (file_write == NULL)) {
		fprintf(stderr, "Verify command but no file ? \n");
		error = 1;
	}

	if ((cmd == 0) || (error)) {
		usage();
		exit(1);
	}

	// execute commands
	bin_buf = malloc(OLS_FLASH_TOTSIZE);
	bin_buf_tmp = malloc(OLS_FLASH_TOTSIZE);
	if ((bin_buf == NULL) || (bin_buf_tmp == NULL)) {
		fprintf(stderr, "Error allocating memory \n");
		return 1;
	}

	ob = BOOT_Init(vid, pid);
	if (ob == NULL) {
		exit(1);
	}

	// 2 times, first might fail for reason unknown
	for (i = 0; i < 2; i++)
		BOOT_Version(ob);
	
	if (cmd & CMD_READ) {
		printf("Reading flash ...\n");
		// reads whole flash (inc bootloader)
		BOOT_Read(ob, 0x0000, bin_buf, OLS_FLASH_TOTSIZE);
		BIN_WriteFile(file_read, bin_buf, OLS_FLASH_TOTSIZE);
	}

	// writing implies erase
	if ((cmd & CMD_ERASE) || (cmd & CMD_WRITE)) {
		printf("Erasing flash ...\n");
		// erases flash (this is done internally by bootloader)
		BOOT_Erase(ob);
	}

	if (cmd & CMD_WRITE) {
		uint32_t max_addr;
		max_addr = BIN_ReadFile(file_write, bin_buf, OLS_FLASH_TOTSIZE);
		printf("Writing flash ... (0x%04x - 0x%04x) \n", OLS_FLASH_ADDR, max_addr);
		// we write only application
		BOOT_Write(ob, OLS_FLASH_ADDR, &bin_buf[OLS_FLASH_ADDR], max_addr - OLS_FLASH_ADDR);
	}

	if (cmd & CMD_VERIFY) {
		uint32_t max_addr;
		// read the whole flash
		printf("Checking flash ...\n");
		max_addr = BIN_ReadFile(file_write, bin_buf_tmp, OLS_FLASH_TOTSIZE);
		BOOT_Read(ob, 0x0000, bin_buf, OLS_FLASH_TOTSIZE);

		// compare only application
		if (memcmp(&bin_buf[OLS_FLASH_ADDR], &bin_buf_tmp[OLS_FLASH_ADDR], max_addr - OLS_FLASH_ADDR) == 0) {
			printf("Verified OK! :)\n");
		} else {
			printf("Verify failed :(\n");
		}
	}

	// free allocated memory
	free(bin_buf_tmp);
	free(bin_buf);
	BOOT_Deinit(ob);

	return 0;
}
