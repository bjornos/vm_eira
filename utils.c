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

#include <assert.h>
#include <string.h>
#include "utils.h"
#include "registers.h"
#include "display.h"



void debug_opcode(struct _dbg *dbg, int index, const char op[])
{
	memset(dbg[index].opcode, '\0', 16);
	strncpy(dbg[index].opcode, op, strlen(op));
}

void debug_instr(struct _dbg *dbg, int index, uint32_t *instr)
{
	dbg[index].instr = *instr;
}

void debug_result(struct _dbg *dbg, int index, long *res)
{
	dbg[index].op_result = *res;
}

void debug_args(struct _dbg *dbg, int index, uint16_t *arg1,uint16_t *arg2)
{
	if (arg1)
		dbg[index].op_arg1 = *arg1;
	if (arg2)
		dbg[index].op_arg2 = *arg2;
}


void dump_instr(struct _dbg *dbg, int dbg_index)
{
	unsigned int q,c;

	c = dbg_index;

	printf("frame\tinstr\t\topcode\t\targ1\targ2\tresult\n");
	printf("=============================================================\n");
	for (q=0; q < DBG_HISTORY; q++) {
		printf("\033[2K");
		printf("%d\t", q);
		printf("0x%08x\t", dbg[c].instr);
		printf("%-16s", dbg[c].opcode);
		printf("%d\t", dbg[c].op_arg1);
		printf("%d\t", dbg[c].op_arg2);
		printf("%ld\n", dbg[c].op_result);
		c = (c -1) % DBG_HISTORY;
	}
	printf("\n");
}

void dump_ram(uint8_t *RAM, int from, int to)
{
	int r,v;
	int p=0;
	int rows=0;
	int grid = 8;
	int range;

	printf("RAM  %d -> %d:\n----\n", from, to);

	/* round up range to nearest grid size */
	range = ((to - from) + grid)  & ~(grid -1);

	for (r=0; r<(range / grid); r++) {
		printf("0x%.2x:\t", rows + from);
		for (v=0; v<grid; v++) {
			assert(from + p <= 0xffff); /* fixme #define RAM_SIZE */
			printf("%x\t", *(RAM + from + p) & 0xff);
			p++;
		}
		rows +=grid;
		printf("\n");
	}
 	printf("\n");
}


void dump_regs(uint16_t *REG)
{
	int i,o;
	int grid = 4;
	int r = 0;

	printf("Registers:\n=========\n");
	for (i=0; i < (16 / grid); i++) {
		for (o=0; o < grid; o++) {
			printf("R%d:\t%d\t",r,*(REG + r));
			r++;
		}
		printf("\n");
	}
	printf("PC:\t%d",*(REG + GP_REG_MAX + 1)); /* yes. this assumes pc is placed right after gp regs in struct */
	printf("\n");
}


void dump_io(uint16_t in, uint16_t out)
{
	printf("I/O:\n=========\n");
	printf("in:\t0x%x\n",in);
	printf("out:\t0x%x\n",out);
	printf("\n");
}
