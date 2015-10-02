/* 
 * Part of ols-fwloader - Serial port routines
 * Inspired by pirate-loader, written by Piotr Pawluczuk 
 * http://the-bus-pirate.googlecode.com/svn/trunk/bootloader-v4/pirate-loader/source/pirate-loader.c
 *
 * Copyright (C) 2010 Piotr Pawluczuk
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

#ifndef MYSERIAL_H_
#define MYSERIAL_H_

#include <config.h>
#include <stdint.h>

#if IS_WIN32
#include <windows.h>
#include <time.h>

#define B115200 115200
#define B921600 921600

typedef long speed_t;
#else // WIN32

#include <unistd.h>
#include <termios.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/time.h>

#endif

#if IS_DARWIN
#include <IOKit/serial/ioss.h>
#include <sys/ioctl.h>

#define B1500000 1500000
#define B1000000 1000000
#define B921600  921600
#endif

int serial_setup(int fd, unsigned long speed);
int serial_write(int fd, const char *buf, int size);
int serial_read(int fd, char *buf, int size, int timeout);
int serial_open(const char *port);
int serial_close(int fd);


#endif

