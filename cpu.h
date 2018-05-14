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

#ifndef __CPU_H__
#define __CPU_H_

#include <stdint.h>

#include "registers.h"
#include "vdc.h"

enum op_size {
	SIZE_BYTE,
	SIZE_INT,
};

enum conditions {
	COND_EQ = 1,
	COND_NEQ = 2,
	COND_ZERO = 4,
	COND_NZERO = 8,
	COND_GR = 16,
	COND_LE = 32,
	COND_UNDEF = 64,
};

struct _cpu_regs {
	uint16_t GP_REG[GP_REG_MAX];	/* general purpose registers */
	unsigned long pc;		/* program counter */
	int sp;				/* stack pointer */
	int cr;				/* conditional register */
	long exception;
	unsigned int mclk;
	unsigned char gpu_request;
	unsigned char vdc_request;
	unsigned char reset;
	unsigned char dbg;		/* enable debug mode */
	unsigned char panic;		/* halt cpu */
};

void cpu_reset(void *mach);

void *cpu_machine(void *mach);

#endif /* __CPU_H__ */
