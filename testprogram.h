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

#ifndef TESTPROGRAM_H_
#define TESTPROGRAM_H_

#include <stdint.h>
#include "opcodes.h"
#include "registers.h"
#include "prg.h"

uint32_t program_unit_test_basic[] = {
	PRG_MAGIC,		/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(1024 << 0),	/* program size  */
	(nop << 0),
	(setposxy << 0) | 10  << 8 | (1 << 20),
	(screenout << 0) | ('T' << 8),
	(setposxy << 0) | 11  << 8 | (1 << 20),
	(screenout << 0) | ('S' << 8),
	(setposxy << 0) | 12  << 8 | (1 << 20),
	(screenout << 0) | ('T' << 8),
	(nop << 0),

	(movi << 0) | (R10 << 8) | OP_DST_REG | (10000 << 16),
	(mov << 0) | (R11 << 8) | OP_DST_REG | (55 << 16),
	(mov << 0) | (R10 << 8) | OP_SRC_REG | OP_DST_MEM | (8192 << 16),
	(mov << 0) | (R11 << 8) | OP_SRC_REG | OP_DST_MEM | (8194 << 16),

	(halt << 0),
};

uint32_t program_unit_test[] = {
	PRG_MAGIC,		/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(1024 << 0),	/* program size  */
	(nop << 0),
	(mov << 0) | (R1 << 8)  | OP_DST_REG | (0xff << 16),
	(mov << 0) | (R4 << 8)  | OP_DST_REG | (0xa0 << 16),
	(mov << 0) | (R7 << 8)  | OP_DST_REG | (0x21 << 16),
	(mov << 0) | (R10 << 8) | OP_SRC_REG | OP_DST_REG | (R4 << 16),
	(mov << 0) | (R14 << 8) | OP_SRC_REG | OP_DST_REG | (R3 << 16),
	(mov << 0) | (R12 << 8) | OP_SRC_MEM | OP_DST_REG | (58 << 16),
	(mov << 0) | (R13 << 8) | OP_SRC_REG | OP_DST_REG | (1 << 16),
	(mov << 0) | (R9 << 8) | OP_SRC_REG | OP_DST_REG | (3 << 16),
	(mov << 0) | (R6 << 8) | OP_SRC_MEM | OP_DST_REG | (59 << 16),
	(mov << 0) | (R0 << 8) | OP_SRC_MEM | OP_DST_REG | (61 << 16),
	(mov << 0) | (R10 << 8) | OP_SRC_REG | OP_DST_MEM | (1 << 16),
	(halt << 0),
};

uint32_t program_add_sub[] = {
	PRG_MAGIC,		/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(1024 << 0),	/* program size  */
	(nop << 0),
	(add << 0) | (R1 << 8)  | OP_DST_REG | (64 << 16),
	(add << 0) | (R0 << 8)  | OP_DST_REG | OP_SRC_REG | (3 << 16),
	(add << 0) | (R7 << 8)  | OP_DST_REG | OP_SRC_REG | (6 << 16),
	(add << 0) | (R2 << 8)  | OP_DST_MEM | OP_SRC_REG | (61 << 16),
	(sub << 0) | (R1 << 8)  | OP_DST_REG | OP_SRC_REG | (0 << 16),
	(halt << 0),
};

uint32_t rom[] = {
	PRG_MAGIC,		/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(1024 << 0),	/* program size  */
	(nop << 0),
	(clrscr << 0),
	(setposxy << 0) | 10  << 8 | (1 << 20),
	(screenout << 0) | ('e' << 8),
	(setposxy << 0) | 11  << 8 | (1 << 20),
	(screenout << 0) | ('i' << 8),
	(setposxy << 0) | 12  << 8 | (1 << 20),
	(screenout << 0) | ('r' << 8),
	(setposxy << 0) | 13  << 8 | (1 << 20),
	(screenout << 0) | ('a' << 8),
	(setposxy << 0) | 14  << 8 | (1 << 20),
	(screenout << 0) | ('-' << 8),
	(setposxy << 0) | 15  << 8 | (1 << 20),
	(screenout << 0) | ('1' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('|' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('/' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('-' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('\'' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('|' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('/' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('-' << 8),
	(setposxy << 0) | 12  << 8 | (2 << 20),
	(screenout << 0) | ('\'' << 8),

	/* so the plan is to jump to program memory and start execute
	 * ... if there is a program loaded there.
	 * otherwise roll back to start of ROM code and wait for a program
     * to be loaded (TBD)
     */
	(movi << 0) | (R1 << 8) | OP_SRC_MEM | OP_DST_REG | (MEM_START_PRG << 16), 			/* mov r1, @mem_start_prog[0]+[2] */
	(movi << 0) | (R2 << 8) | OP_SRC_MEM | OP_DST_REG | ((MEM_START_PRG + 2) << 16), 	/* mov r2, @mem_start_prog[3]+[4] */

	(mov << 0) | (R3 << 8) | OP_DST_REG | (PRG_MAGIC << 16),							/* mov r3, prog magic low bits */
	(mov << 0) | (R4 << 8) | OP_DST_REG | ((PRG_MAGIC >> 16) << 16),					/* mov r4. prog magic high bits */

	(cmp << 0) | (R1 << 8) | OP_DST_REG | OP_SRC_REG | (3 << 16),						/* r1 == r3? */
	(brneq << 0) | ((MEM_START_ROM + PRG_HEADER_SIZE) << 16),							/* not eq. jump to start of ROM */

	(cmp << 0) | (R2 << 8) | OP_DST_REG | OP_SRC_REG | (4 << 16),						/* r2 == r4? */
	(brneq << 0) | ((MEM_START_ROM + PRG_HEADER_SIZE) << 16),							/* not eq. jump to start of ROM */
	(jmp << 0) | ((MEM_START_PRG + PRG_HEADER_SIZE) << 8),								/* run program */

	(nop << 0),
	(halt << 0),
};

#endif /* TESTPROGRAM_H_ */
