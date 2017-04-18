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
	long op_result;
};

void dump_instr(struct _dbg *dbg, int dbg_index);

void dump_ram(uint8_t *RAM, int from, int to);

void dump_regs(uint16_t *GP_REG);

