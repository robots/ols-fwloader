/* 
 * Part of ols-fwloader - Open Bench Logic Sniffer update interface
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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_file.h"
#include "serial.h"
#include "ols.h"

const struct ols_flash_t OLS_Flash[] = {
	{
		"\x1f\x24\x00\x00",
		264, // size of page
		2048, // number of pages
		"ATMEL AT45DB041D"
	},
	{
		"\x1f\x23\x00\x00",
		264, // size of page
		1024, // number of pages
		"ATMEL AT45DB021D"
	},
	{
		"\xef\x30\x12\x00",
		256, // size of page
		1024, // number of pages
		"WINBOND W25X20"
	},
	{
		"\xef\x30\x13\x00",
		256, // size of page
		2048, // number of pages
		"WINBOND W25X40"
	},
	{
		"\xef\x30\x14\x00",
		256, // size of page
		4096, // number of pages
		"WINBOND W25X80"
	},
	{
		"\xef\x40\x14\x00",
		256, // size of page
		4096, // number of pages
		"WINBOND W25Q80"
	},
};

#define OLS_FLASH_NUM (sizeof(OLS_Flash)/sizeof(struct ols_flash_t))

struct ols_t *OLS_Init(char *port, unsigned long speed)
{
	int fd;
	int ret;
	struct ols_t *ols;

	fd = serial_open(port);
	if (fd < 0) {
		fprintf(stderr, "Unable to open port '%s' \n", port);
		return NULL;
	}

	ret = serial_setup(fd, speed);
	if (ret) {
		fprintf(stderr, "Unable to set serial port parameters \n");
		return NULL;
	}

	ols = malloc(sizeof(struct ols_t));
	if (ols == NULL) {
		fprintf(stderr, "Error allocating memory \n");
		return NULL;
	}

	ols->fd = fd;
	ols->verbose = 0;
	ols->flash = NULL;

	ret = OLS_GetID(ols);
	if (ret) {
		fprintf(stderr, "Unable to read ID \n");
		free(ols);
		return NULL;
	}

	ret = OLS_GetFlashID(ols);
	if (ret) {
		fprintf(stderr, "Unable to read Flash ID \n");
		free(ols);
		return NULL;
	}

	return ols;
}

int OLS_Deinit(struct ols_t *ols)
{
	serial_close(ols->fd);
	free(ols);

	return 0;
}

/*
 * Does OLS self-test
 * ols->fd - fd of OLS com port
 */
int OLS_RunSelftest(struct ols_t *ols)
{
	uint8_t cmd[4] = {0x07, 0x00, 0x00, 0x00};
	uint8_t status;
	int res, retry;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	retry=0;
	while (1) {
		res = serial_read(ols->fd, &status, 1, 100);

		if (res < 1) {
			retry ++;
		}

		if (res == 1) {
			printf("done...\n");
			break;
		}

		// 20 second timenout
		if (retry > 60) {
			printf("failed :( - timeout\n");
			return -1;
		}
	}

	if(status == 0x00){
		printf("Passed self-test :) \n");
	}else{
		printf("Failed :( - '%x'\n", status);
		if(status & 0x01 /* 0b1 */)
			printf("ERROR: 1V2 supply failed self-test :( \n");
		if(status & 0x02 /* 0b10 */)
			printf("ERROR: 2V5 supply failed self-test :( \n");
		if(status & 0x04 /* 0b100 */)
			printf("ERROR: PROG_B pull-up failed self-test :( \n");
		if(status & 0x08 /* 0b1000 */)
			printf("ERROR: DONE pull-up failed self-test :( \n");
		if(status & 0x10 /* 0b10000 */)
			printf("ERROR: unknown ROM JEDEC ID (this could be ok...) :( \n");
		if(status & 0x20 /* 0b100000 */)
			printf("ERROR: UPDATE button pull-up failed self-test :( \n");
	}

	return 0;
}


/*
 * Reads OLS status
 * ols->fd - fd of ols com port
 */
int OLS_GetStatus(struct ols_t *ols)
{
	uint8_t cmd[4] = {0x05, 0x00, 0x00, 0x00};
	uint8_t status;
	int res;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	res = serial_read(ols->fd, &status, 1, 100);

	if (res != 1) {
		printf("Error reading OLS status\n");
		return -1;
	}

	printf("OLS status: %02x\n", status);
	return 0;
}

/*
 * Reads OLS version
 * ols->fd - fd of ols com port
 */
int OLS_GetID(struct ols_t *ols)
{
	uint8_t cmd[4] = {0x00, 0x00, 0x00, 0x00};
	uint8_t ret[7];
	int res;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	res = serial_read(ols->fd, ret, 7, 10);
	if (res != 7) {
		printf("Error reading OLS id\n");
		return -1;
	}

	if (ret[0] != 'H' || ret[2] != 'F' || ret[5] != 'B') {
		printf("Error reading OLS id - invalid data returned\n");
		return -1;
	}

	printf("Found OLS HW: %d, FW: %d.%d, Boot: %d\n", ret[1], ret[3], ret[4], ret[6]);
	return 0;
}

/*
 * commands OLS to enter bootloader mode
 * ols->fd - fd of ols com port
 */
int OLS_EnterBootloader(struct ols_t *ols)
{
	uint8_t cmd[4] = {0x24, 0x24, 0x24, 0x24};
	int res;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	printf("OLS switched to bootloader mode\n");
	return 0;
}

/*
 * Switch the OLS to run mode
 * ols->fd - fd of ols com port
 */
int OLS_EnterRunMode(struct ols_t *ols)
{
	uint8_t cmd[4] = {0xFF, 0xFF, 0xFF, 0xFF};
	int res;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	printf("OLS switched to RUN mode\n");
	return 0;
}

/*
 * ask the OLS for JEDEC id
 * ols->fd - fd of ols com port
 */
int OLS_GetFlashID(struct ols_t *ols) {
	uint8_t cmd[4] = {0x01, 0x00, 0x00, 0x00};
	uint8_t ret[4];
	int res;
	int i;

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	res = serial_read(ols->fd, ret, 4, 10);
	if (res != 4) {
		printf("Error reading JEDEC ID\n");
		return -1;
	}

	if (ret[0] == 'H' && ret[2] == 'F') {
		printf("Error reading flash id - OLS id was returned instead\n");
		return -1;
	}

	for (i = 0; i < OLS_FLASH_NUM; i++) {
		if (memcmp(ret, OLS_Flash[i].jedec_id, 4) == 0) {
			ols->flash = (struct ols_flash_t *)&OLS_Flash[i];
			printf("Found flash: %s \n", OLS_Flash[i].name);
			break;
		}
	}

	if(ols->flash == NULL) {
		printf("Error - unknown flash type (%02x %02x %02x %02x)\n", ret[0], ret[1], ret[2], ret[3]);
		printf("Is OLS in update mode ??\n");
		return -1;
	}

	return 0;
}

/*
 * erases OLS flash
 * ols->fd - fd of ols com port
 */
int OLS_FlashErase(struct ols_t *ols)
{
	uint8_t cmd[4] = {0x04, 0x00, 0x00, 0x00};
	uint8_t status;
	int res;
	int retry = 0;

	if (ols->flash == NULL) {
		printf("Cannot erase unknown flash\n");
		return -3;
	}

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing to OLS\n");
		return -2;
	}

	printf("Chip erase ... ");

	while (1) {
		res = serial_read(ols->fd, &status, 1, 100);

		if (res <1) {
			retry ++;
		}

		if (res == 1) {
			if (status == 0x01) {
				printf("done :)\n");
				return 0;
			}
			printf("failed :( - bad reply '%x' Number of reply bytes: %x \n", status, res);
			return -1;
		}

		// 20 second timenout
		if (retry > 60) {
			printf("failed :( - timeout\n");
			return -1;
		}
	}

	return 0;
}

/*
 * Reads data from flash
 * ols->fd - fd of ols com port
 * page - which page should be read
 * buf - buffer where the data will be stored
 */
int OLS_FlashRead(struct ols_t *ols, uint16_t page, uint8_t *buf)
{
	uint8_t cmd[4] = {0x03, 0x00, 0x00, 0x00};
	int res;

	if (ols->flash == NULL) {
		printf("Cannot READ  unknown flash\n");
		return -3;
	}

	if (page > ols->flash->pages) {
		printf("You are trying to read page %d, but we have only %d !\n", page, ols->flash->pages);
		return -2;
	}

	if(ols->flash->page_size == 264){//ATMEL ROM with 264 byte pages
		cmd[1] = (page >> 7) & 0xff;
		cmd[2] = (page << 1) & 0xff;
	} else { //most 256 byte page roms
		cmd[1] = (page >> 8) & 0xff;
		cmd[2] = (page) & 0xff;
	}

	if (ols->verbose)
		printf("Page 0x%04x read ... ", page);

	res = serial_write(ols->fd, cmd, 4);
	if (res != 4) {
		printf("Error writing CMD to OLS\n");
		return -2;
	}

	res = serial_read(ols->fd, buf, ols->flash->page_size, 100);

	if (res == ols->flash->page_size) {
		if (ols->verbose)
			printf("OK\n");
		return 0;
	}

	printf("Failed :(\n");
	return -1;
}

/*
 * writes data to flash
 * ols->fd - fd of ols com port
 * page - where the data should be written to
 * buf - data to be written
 */
int OLS_FlashWrite(struct ols_t *ols, uint16_t page, uint8_t *buf)
{
	uint8_t cmd[4] = {0x02, 0x00, 0x00, 0x00};
	uint8_t status;
	uint8_t chksum;
	int res;

	if (ols->flash == NULL) {
		printf("Cannot Write unknown flash\n");
		return -3;
	}

	if (page > ols->flash->pages) {
		printf("You are trying to Write page %d, but we have only %d !\n", page, ols->flash->pages);
		return -2;
	}

	if(ols->flash->page_size == 264){//ATMEL ROM with 264 byte pages
		cmd[1] = (page >> 7) & 0xff;
		cmd[2] = (page << 1) & 0xff;
	} else { //most 256 byte page roms
		cmd[1] = (page >> 8) & 0xff;
		cmd[2] = (page) & 0xff;
	}

	chksum = Data_Checksum(buf, ols->flash->page_size);

	if (ols->verbose)
		printf("Page 0x%04x write ... (0x%04x 0x%04x)", page, cmd[1], cmd[2]);

	res = serial_write(ols->fd, cmd, 4);
	res += serial_write(ols->fd, buf, ols->flash->page_size);
	res += serial_write(ols->fd, &chksum, 1);
	if (res != 4 + 1 + ols->flash->page_size) {
		printf("Error writing CMD to OLS\n");
		return -2;
	}

	res = serial_read(ols->fd, &status, 1, 100);

	if (res != 1) {
		printf("Page writing timeout\n");
		return -1;
	}

	if (status == 0x01) {
		if (ols->verbose)
			printf("OK\n");
		return 0;
	}

	printf("Checksum error :(\n");
	return -1;
}

