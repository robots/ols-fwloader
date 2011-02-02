/*
 * OLS flash loader
 *
 * 2011 - Michal Demin
 *
 */

#include <config.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ols-boot.h"
#include "ols.h"
#include "data_file.h"

#if IS_WIN32
#define sleep(n) Sleep((1000 * (n)))
#endif

enum {
	CMD_READ = 1,
	CMD_WRITE = 2,
	CMD_VERIFY = 4,
	CMD_ERASE = 8,
	CMD_RESET = 16,
	CMD_SELFTEST = 32,
};

enum {
	DEV_BOOT = 1,
	DEV_APP = 2,
	DEV_SWITCH = 4,
};

static int memverify(uint8_t *ref, uint8_t *mem, uint32_t len, uint32_t offset, int verbose);

static void usage()
{
	printf("This is " PACKAGE_STRING);
	printf("ols-fwloader [-d] [-V] [-W] [-R] [-E] [-r rfile] [-w wfile] [-t type] [-v vid] [-p pid]\n\n");
	printf("  -f dev  - select which device to work with (BOOT or APP)\n");
	printf("  -V      - veriy Flash against wfile\n");
	printf("  -E      - erase flash\n");
	printf("  -W      - erase and write flash with wfile\n");
	printf("  -R      - read flash to rfile\n");
	printf("  -T      - reset device at the end\n\n");
	printf("  -t type - File type (BIN/HEX) (default: HEX)\n");
	printf("  -w file - file to be read and written to flash\n");
	printf("  -r file - file where the flash content should be written to\n");
	printf("  -d      - be verbosse\n");

	printf("BOOT only options: \n");
	printf("  -p pid  - Set usb PID\n");
	printf("  -v vid  - Set usb VID\n");
	printf("  -n      - enter bootloader first\n");

	printf("APP only options: \n");
	printf("  -P port - Serial port device\n");
	printf("  -l num  - Limit number of read/written pages to num\n");
	printf("  -S      - run selftest\n");

	printf("\n\n");
	printf("Write PIC firmware with verification, enter bootloader first:\n");
	printf(" ols-fwloader -f BOOT -n -P /dev/ttyACM0 -V -W -w firmware.hex\n");
	printf("Write FPGA bitstream (HEX):\n");
	printf(" ols-fwloader -f APP -W -w bitstream.mcs\n");
	printf("Write FPGA bitstream (BIN):\n");
	printf(" ols-fwloader -f APP -W -w bitstream.bit -t BIN\n");
	printf("\n");

}

int main(int argc, char** argv)
{
	struct ols_boot_t *ob;
	struct ols_t *ols;

	uint8_t *bin_buf;
	uint8_t *bin_buf_tmp;
	uint32_t bin_buf_size;

	uint16_t vid = OLS_VID;
	uint16_t pid = OLS_PID;

	int error = 0;
	int ret;
	int i;
	uint16_t pages;
	int debug = 0;

	// aguments
	struct file_ops_t *fo;
	char *file_write = NULL;
	char *file_read = NULL;
	uint8_t cmd = 0;
	char *port = NULL;
	uint8_t device = 0;
	uint16_t page_limit = 0;
	uint32_t max_addr = 0;

	// getopt
	int opt;

	fo = GetFileOps("HEX");

	// parse args
	while ((opt = getopt(argc, argv, "WRVETSnr:w:v:p:t:P:f:hd")) != -1) {
		switch (opt) {
			case 'd':
				debug = 1;
				break;
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
			case 'T':
				cmd |= CMD_RESET;
				break;
			case 'S':
				cmd |= CMD_SELFTEST;
				break;
			case 'l':
				page_limit = atoi(optarg);
				break;
			case 'v': // vid
				vid = atoi(optarg);
				break;
			case 'p': // vid
				pid = atoi(optarg);
				break;
			case 'P':
				if (port != NULL) {
					fprintf(stderr, "Two ports ?!\n");
					exit(-1);
				}
				port = strdup(optarg);
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
			case 'n':
				device |= DEV_SWITCH;
				break;
			case 'f':
				if (device & (DEV_APP | DEV_BOOT)) {
					fprintf(stderr, "Two devices ??\n");
					exit(-1);
				}
				if (strncasecmp(optarg, "APP", 3) == 0) {
					device |= DEV_APP;
				} else if (strncasecmp(optarg, "BOOT", 4) == 0) {
					device |= DEV_BOOT;
				} else {
					fprintf(stderr, "Unknown device specified\n");
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

	if ((device & DEV_APP) || device & DEV_SWITCH) {
		if (port == NULL) {
			fprintf(stderr, "Missing port \n");
			error = 1;
		}
	}

	if ((device & 3) == 0) {
		fprintf(stderr, "Device not set\n");
		error = 1;
	}

	if (cmd == 0) {
		fprintf(stderr, "Missing command\n");
		error = 1;
	}

	if ((error)) {
		usage();
		exit(1);
	}

	// execute commands
	// Working with APP or switch to bootloader first
	if ((device & DEV_APP) || (device & DEV_SWITCH)) {
		ols = OLS_Init(port, 921600);

		if (ols == NULL) {
			fprintf(stderr, "Unable to open OLS\n");
			exit(-1);
		}

		if (debug) {
			ols->verbose = 1;
		}

		bin_buf_size = ols->flash->pages * ols->flash->page_size;
		if (device & DEV_SWITCH) {
			if (device & DEV_BOOT) {
				OLS_EnterBootloader(ols);
				OLS_Deinit(ols);

				// wait for device to appear
				sleep(2);
			} else {
				fprintf(stderr, "Not switching to bootloader.\n");
				device &= ~DEV_SWITCH;
			}
		}
	}

	// Initialize bootloader
	if (device & DEV_BOOT) {
		ob = BOOT_Init(vid, pid);
		if (ob == NULL) {
			exit(1);
		}

		// 2 times, first might fail for reason unknown
		for (i = 0; i < 2; i++) {
			ret = BOOT_Version(ob);
			if (ret) {
				exit(1);
			}
		}

		bin_buf_size = OLS_FLASH_TOTSIZE;
	}

	// allocate buffers
	bin_buf = malloc(bin_buf_size);
	bin_buf_tmp = malloc(bin_buf_size);
	if ((bin_buf == NULL) || (bin_buf_tmp == NULL)) {
		fprintf(stderr, "Error allocating memory \n");
		exit(1);
	}

	if (cmd & CMD_SELFTEST) {
		if (device & DEV_APP) {
			ret = OLS_RunSelftest(ols);
			if (ret) {
				exit(1);
			}
		}
	}

	if (cmd & CMD_READ) {
		printf("Reading flash \n");
		memset(bin_buf, 0xff, bin_buf_size);

		if (device & DEV_APP) {
			int i;
			if (page_limit != 0) {
				pages = page_limit;
			} else {
				pages = ols->flash->pages;
			}
			pages = (pages > ols->flash->pages) ? ols->flash->pages : pages;
			for (i = 0; i < pages; i ++) {
				// read i-th page into buffer
				ret = OLS_FlashRead(ols, i, bin_buf + (ols->flash->page_size * i));
				if (ret) {
					exit(1);
				}
				if ((i % 32) == 0) {
					printf(".");
					fflush(stdout);
				}
			}
			printf("\n");
		} else {
			// reads whole flash (inc bootloader)
			ret = BOOT_Read(ob, 0x0000, bin_buf, bin_buf_size);
			if (ret) {
				exit(1);
			}
		}
		printf("Writing file '%s'\n", file_read);
		fo->WriteFile(file_read, bin_buf, bin_buf_size);
	}

	// JaWi: first read the entire data file before going to erase/write stuff.
	// This way, we're fairly sure we can leave the device in a workable state
	if (cmd & CMD_WRITE) {
		printf("Reading file '%s'\n", file_write);
		memset(bin_buf_tmp, 0xff, bin_buf_size);

		max_addr = fo->ReadFile(file_write, bin_buf, bin_buf_size);
		if (max_addr == 0) {
			// error reading
			fprintf(stderr, "Error reading file - skipping write\n");
			exit(1);
		}
	}

	// writing implies erase
	if ((cmd & CMD_ERASE) || (cmd & CMD_WRITE)) {
		printf("Erasing flash ...\n");
		if (device & DEV_APP) {
			// erase spi flash, bulk erase
			ret = OLS_FlashErase(ols);
			if (ret) {
				exit(1);
			}
		} else {
			// erases flash (this is done internally by bootloader)
			ret = BOOT_Erase(ob);
			if (ret) {
				exit(1);
			}
		}
	}

	if (cmd & CMD_WRITE) {
		if ((max_addr != 0) && (device & DEV_APP)) {
			int i;
			// determine number of pages (round up)
			if (page_limit != 0) {
				pages = page_limit;
			} else {
				pages = (max_addr + ols->flash->page_size - 1) / ols->flash->page_size;
			}
			pages = (pages > ols->flash->pages) ? ols->flash->pages : pages;
			printf("Will write %d pages \n", pages);
			for (i = 0; i < pages; i ++) {
				ret = OLS_FlashWrite(ols, i, bin_buf + (ols->flash->page_size * i));
				if (ret) {
					exit(1);
				}
				if ((i % 32) == 0) {
					printf(".");
					fflush(stdout);
				}
			}
			printf("\n");
		} else if ((max_addr != 0) && (device & DEV_BOOT)) {
			int size;
			// we write only application
			size = ((max_addr - OLS_FLASH_ADDR) > OLS_FLASH_SIZE)? OLS_FLASH_SIZE : max_addr - OLS_FLASH_ADDR;
			printf("Writing flash ... (0x%04x - 0x%04x) \n", OLS_FLASH_ADDR, size + OLS_FLASH_ADDR);
			ret = BOOT_Write(ob, OLS_FLASH_ADDR, &bin_buf[OLS_FLASH_ADDR], size);
			if (ret) {
				exit(1);
			}
		}
	}

	if (cmd & CMD_VERIFY) {
		// read the whole flash
		memset(bin_buf_tmp, 0xff, bin_buf_size);
		memset(bin_buf, 0xff, bin_buf_size);

		max_addr = fo->ReadFile(file_write, bin_buf_tmp, bin_buf_size);
		if (max_addr == 0) {
			// error reading
			fprintf(stderr, "Error reading file - skipping verify\n");
		}

		if ((max_addr != 0) && (device & DEV_APP)) {
			int i;

			printf("Checking flash ...\n");

			if (page_limit != 0) {
				pages = page_limit;
			} else {
				pages = (max_addr + ols->flash->page_size - 1) / ols->flash->page_size;
			}
			pages = (pages > ols->flash->pages) ? ols->flash->pages : pages;
			for (i = 0; i < pages; i ++) {
				uint32_t off;
				off = ols->flash->page_size * i;
				// read i-th page into buffer
				ret = OLS_FlashRead(ols, i, bin_buf + off);
				if (ret) {
					exit(1);
				}
				if ((i % 32) == 0) {
					printf(".");
					fflush(stdout);
				}
			}
			printf("\n");
			// compare page by page
			if (memverify(bin_buf_tmp, bin_buf, max_addr, 0, debug)) {
				printf("Verify error\n");
			} else {
				printf("Verify OK\n");
			}
		} else if ((max_addr != 0) && (device & DEV_BOOT)) {
			int size;
			ret = BOOT_Read(ob, 0x0000, bin_buf, OLS_FLASH_TOTSIZE);
			if (ret) {
				exit(1);
			}
			// compare only application
			size = ((max_addr - OLS_FLASH_ADDR) > OLS_FLASH_SIZE)? OLS_FLASH_SIZE : max_addr - OLS_FLASH_ADDR;
			printf("Checking flash ... (0x%04x - 0x%04x)\n", OLS_FLASH_ADDR, size + OLS_FLASH_ADDR);

			if (memverify(&bin_buf_tmp[OLS_FLASH_ADDR], &bin_buf[OLS_FLASH_ADDR], size, OLS_FLASH_ADDR, debug) == 0) {
				printf("Verified OK! :)\n");
			} else {
				printf("Verify failed :(\n");
			}
		}
	}

	if (cmd & CMD_RESET) {
		if (device & DEV_APP) {
			printf("Reseting to normal mode \n");
			OLS_EnterRunMode(ols);
		} else {
			printf("Reseting device \n");
			BOOT_Reset(ob);
		}
	}

	if (device & DEV_APP) {
		OLS_Deinit(ols);
	} else {
		BOOT_Deinit(ob);
	}

	// free allocated memory
	free(bin_buf_tmp);
	free(bin_buf);

	return 0;
}

static int memverify(uint8_t *ref, uint8_t *mem, uint32_t len, uint32_t offset, int verbose)
{
	int ret = 0;
	uint32_t i;

	for (i = 0; i < len; i++) {
		if (ref[i] != mem[i]) {
			if (verbose)
				printf("Diff @0x%04x (Is 0x%02x should be 0x%02x)\n", offset + i, mem[i], ref[i]);
			ret = 1;
		}
	}

	return ret;
}

