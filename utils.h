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

#include <stdio.h>
#include <stdint.h>

#include "opcodes.h"

#define DBG_HISTORY 8 /* fixme: program argument instead */
#define DUMP_RAM_SIZE_DEFAULT 32

struct _dbg {
	uint32_t instr;
	char opcode[OPCODE_NAME_MAX];
	uint16_t op_arg1;
	uint16_t op_arg2;
	unsigned long op_result;
};

void debug_opcode(struct _dbg *dbg, int index, const char op[]);

void debug_instr(struct _dbg *dbg, int index, uint32_t *instr);

void debug_result(struct _dbg *dbg, int index, unsigned long res);

void debug_args(struct _dbg *dbg, int index, uint16_t *arg1,uint16_t *arg2);

void dump_instr(struct _dbg *dbg, int dbg_index);

void dump_ram(uint8_t *RAM, int from, int to);

void dump_regs(uint16_t *GP_REG);

void dump_io(uint16_t in, uint16_t out);

char *int_to_str(int num);

