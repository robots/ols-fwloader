/*
 * HEX/BIN Data file parser and writer
 *
 * 2010 - Michal Demin
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "data_file.h"

#undef DEBUG

#define FILE_OPS_CNT (sizeof(file_ops)/sizeof(struct file_ops_t))
const struct file_ops_t file_ops[] = {
	{
		.name = "BIN",
		.ReadFile = BIN_ReadFile,
		.WriteFile = BIN_WriteFile,
	},
	{
		.name = "HEX",
		.ReadFile = HEX_ReadFile,
		.WriteFile = HEX_WriteFile,
	}
};

struct file_ops_t *GetFileOps(char *name)
{
	int i;

	for (i = 0; i < FILE_OPS_CNT; i++) {
		if (strcasecmp(name, file_ops[i].name) == 0) {
			return (struct file_ops_t *)&file_ops[i];
		}
	}

	return NULL;
}

/*
 * returns checksum of input buffer
 */
uint8_t Data_Checksum(uint8_t *buf, uint16_t size) {
	uint16_t i;
	uint8_t sum = 0;

	for (i=0; i<size; i++) {
		sum -= buf[i];
	}
	return sum;
}

/*
 * reads hex file
 * file - name of hexfile
 * buf - buffer where the data should be written to
 * size - size of buffer
 */
#define MAX_LINE_SIZE 256
uint32_t HEX_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size) {
	char raw_line[MAX_LINE_SIZE];
	int line = 0;
	uint32_t base_addr;
	FILE *fp;
	uint32_t addr_max=0;
	uint8_t *tmp;
	uint16_t tmp_len;
	uint8_t chksum;
	uint16_t i;
	unsigned int b;

	fp = fopen(file, "r");

	if (fp == NULL) {
		return 0;
	}

	tmp = malloc(sizeof(raw_line)+1);
	if (tmp == NULL) {
		fprintf(stderr, "Memory allocation problem\n");
		return 0;
	}


	while(fgets(raw_line, sizeof(raw_line), fp) != 0) {
		// read line header
		uint8_t byte_count;
		uint32_t addr;
		uint8_t rec_type;
		const char *p;

		if (raw_line[0] != ':') {
			printf("File '%s' is note a hex file !\n", file);
			return 0;
		}

		line ++;
		p = raw_line + 1;

		tmp_len = 0;
		// read byte count (1byte), address (2byte), record type (1byte)
		for (i = 0; i < 4; i ++) {
			sscanf(p, "%2x", &b);
			p += 2;
			tmp[tmp_len] = b;
			tmp_len ++;
		}

		byte_count = tmp[0];
		addr = (tmp[1] << 8) | tmp[2];
		rec_type = tmp[3];

		if (rec_type == 0x00) {
			// data record
			uint16_t i;
			addr = base_addr | addr;

			if (out_buf_size < byte_count + addr) {
				fprintf(stderr, "Data won't fit into buffer (size= %04x want %04x)\n", out_buf_size, byte_count + addr);
				fclose(fp);
				return 0;
			}

			for (i = 0; i < byte_count; i++) {
				sscanf(p, "%2x", &b);
				p += 2;

				tmp[tmp_len] = b;
				tmp_len++;

				out_buf[addr + i] = b;

				if (addr_max < addr + i) {
					addr_max = addr + i;
				}
			}
		} else if (rec_type == 0x04) {
			// extended linear: base addr
			base_addr = 0;
			for (i = 0; i < byte_count; i++) {
				sscanf(p, "%2x", &b);
				p += 2;

				tmp[tmp_len] = b;
				tmp_len ++;
				base_addr = (base_addr << 8) | (b & 0xff);
			}

			base_addr <<= 16;
#ifdef DEBUG
			fprintf(stderr, "new (linear) base addr = %08x\n", base_addr);
#endif
		} else if (rec_type == 0x01) {
			// end record
			break;
		} else {
			fprintf(stderr, "Unknown record type on line %d\n", line);
			return 0;
		}

		sscanf(p, "%2x", &b);
		chksum = b;

		// calculate checksum after line was parsed
		if (Data_Checksum(tmp, tmp_len) != chksum) {
			fprintf(stderr, "Checksum error on line %d\n", line);
			return 0;
		}
	}

	fclose(fp);
	return addr_max + 1;
}

/*
 * writes hex file
 * file - name of hexfile
 * buf - buffer where data is stored
 * size - size of buffer
 */
static void HEX_WriteRec(FILE *fp, uint8_t rec_id, uint8_t byte_count, uint16_t addr, uint8_t *data) {
	int res;
	uint8_t bin_line[128];
	char raw_line[256];
	uint8_t i=0;
	uint8_t j;

	// fill header
	bin_line[0] = byte_count;
	bin_line[1] = (addr >> 8) & 0xff;
	bin_line[2] = addr & 0xff;
	bin_line[3] = rec_id;

	// copy data
	for (i=0; i<byte_count; i++) {
		bin_line[4 + i] = data[i];
	}

	// add checksum
	bin_line[4 + i] = Data_Checksum(bin_line, 4+i);

	// rewrite bin data to hex
	res = sprintf(raw_line, ":");
	for (j = 0; j < 4 + i + 1; j++) {
		res += sprintf(raw_line + res, "%02X", bin_line[j]);
	}
	res += sprintf(raw_line + res, "\n");

	// output to file
	res = fwrite(raw_line, sizeof(char), res, fp);
}

/*
 * writes hex file
 * file - name of hexfile
 * buf - buffer which contains the data
 * size - size of buffer
 */
int HEX_WriteFile(const char *file, uint8_t *in_buf, uint32_t in_buf_size) {
	uint32_t written = 0;
	uint16_t base = 0x0000;
	uint32_t addr = 0x0000;
	FILE *fp;

	fp = fopen(file, "w");

	if (fp == NULL) {
		return -1;
	}

	// ext address record
	HEX_WriteRec(fp, 0x04, 2, 0x0000, (uint8_t*)&base);

	while (written < in_buf_size) {
		uint8_t byte_count;
		if ((in_buf_size - written) > 0x10) {
			byte_count = 0x10;
		} else {
			byte_count = in_buf_size - written;
		}

		// write data record
		HEX_WriteRec(fp, 0x00, byte_count, addr, &in_buf[written]);
		written += byte_count;
		addr += byte_count;

		// write ext addr record
		if (addr & 0x10000) {
			uint8_t tmp[2];

			base ++;

			tmp[0] = (base >> 8) & 0xff;
			tmp[1] = (base & 0xff);

			HEX_WriteRec(fp, 0x04, 2, 0x0000, tmp);

			addr -= 0x10000;
		}
	}

	// end record
	HEX_WriteRec(fp, 0x01, 0x00, 0x0000, NULL);

	fclose(fp);
	return 0;
}

/*
 * reads bin file
 * file - name of hexfile
 * buf - buffer where the data should be written to
 * size - size of buffer, returns actual size read
 */
uint32_t BIN_ReadFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size) {
	int res;
	long fsize;
	FILE *fp;

	fp = fopen(file, "rb");
	if (fp == NULL) {
		return 0;
	}

	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (fsize > out_buf_size) {
		printf("file won't fit into buffer :(\n");
		return 0;
	}

	res = fread(out_buf, sizeof(uint8_t), fsize, fp);
	if (res <= 0) {
		printf("error reading file %s \n", file);
	}

	fclose(fp);
	return (res < 0) ? 0 : res;
}

/*
 * writes bin file
 * file - name of hexfile
 * buf - buffer which contains the data
 * size - size of buffer
 */
int BIN_WriteFile(const char *file, uint8_t *out_buf, uint32_t out_buf_size) {
	FILE *fp;
	int res;

	fp = fopen(file, "wb");
	if (fp == NULL) {
		return -1;
	}
	res = fwrite(out_buf, sizeof(uint8_t), out_buf_size, fp);
	if (res != out_buf_size) {
		printf("error writing file %s\n", file);
	}
	fclose(fp);

	return 0;
}
