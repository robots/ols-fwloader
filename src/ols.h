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

#ifndef OLS_H_
#define OLS_H_

#include <stdint.h>

struct ols_flash_t {
	const char *jedec_id;
	uint16_t page_size;
	uint16_t pages;
	const char *name;
};

struct ols_t {
	int fd;
	struct ols_flash_t *flash;
	int verbose;
};


struct ols_t *OLS_Init(char *, unsigned long); 
int OLS_Deinit(struct ols_t *);
int OLS_RunSelftest(struct ols_t *);
int OLS_GetStatus(struct ols_t *);
int OLS_GetID(struct ols_t *);
int OLS_EnterBootloader(struct ols_t *);
int OLS_EnterRunMode(struct ols_t *);
int OLS_GetFlashID(struct ols_t *);
struct ols_flash_t *OLS_GetFlash(struct ols_t *);
int OLS_FlashErase(struct ols_t *);
int OLS_FlashRead(struct ols_t *, uint16_t page, uint8_t *buf);
int OLS_FlashWrite(struct ols_t *, uint16_t page, uint8_t *buf);

#endif

