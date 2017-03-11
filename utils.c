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
#include "utils.h"

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


void dump_regs(uint16_t *GP_REG)
{
	int i,o;
	int grid = 4;
	int r = 0;

	printf("General Purpose Registers:\n--------------------------\n");
	for (i=0; i <= (16 / grid); i++) {
		for (o=0; o < grid; o++) {
			printf("R%d:\t%d\t",r,*(GP_REG + r));
			r++;
		}
		printf("\n");
	}
	printf("\n");
}

