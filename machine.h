/*
 * Eira Virtual Machine
 *
 * Copyright (C) 2017
 *
 * Author: Björn Östby <bjorn.ostby@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __MACHINE_H__
#define __MACHINE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <argp.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "cpu.h"
#include "vdc.h"
#include "ioport.h"
#include "memory.h"

#define MACHINE_RESET_VECTOR	(MEM_START_ROM - sizeof(uint32_t))
#define MACHINE_DEVICE_LIST_END	'\0'
#define MACHINE_MASTER_CLOCK	1400	/* Master oscillator runs @ 1.4 MHz */

struct _machine_reg {
		uint8_t *prg_loading;
		uint8_t *boot_msg;
		uint8_t *boot_anim;
};

struct _machine {
	uint8_t RAM[RAM_SIZE];
	struct _cpu_regs cpu_regs;
	struct _machine_reg mach_regs;
	struct _vdc_regs vdc_regs;
	struct _display_adapter display;
	struct _io_regs *ioport;
	exception_t exception;
};

#endif /* __MACHINE_H_ */