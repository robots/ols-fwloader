/* 
 * Part of ols-fwloader - data file parsing/generaion
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

#ifndef DATA_FILE_H_
#define DATA_FILE_H_

#include <stdint.h>
struct file_ops_t {
	char *name;

	uint32_t (*ReadFile)(const char *, uint8_t *, uint32_t);
	int (*WriteFile)(const char *, uint8_t *, uint32_t);
};


uint8_t Data_Checksum(uint8_t *buf, uint16_t size);

struct file_ops_t *GetFileOps(char *);

uint32_t HEX_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size);
int HEX_WriteFile(const char *file, uint8_t *in_buf, uint32_t in_buf_size);
uint32_t BIN_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size);
int BIN_WriteFile(const char *file, uint8_t *in_buf, uint32_t in_buf_size);

#endif

