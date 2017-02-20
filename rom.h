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

#ifndef __ROM_H_
#define __ROM_H_

#include "opcodes.h"
#include "registers.h"
#include "prg.h"

uint32_t rom[] = {
	PRG_MAGIC,		/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(1024 << 0),		/* program size  */
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
	(movi << 0) | (R1 << 8) | OP_SRC_MEM | OP_DST_REG | (MEM_START_PRG << 16), 		/* mov r1, @mem_start_prog[0]+[2] */
	(movi << 0) | (R2 << 8) | OP_SRC_MEM | OP_DST_REG | ((MEM_START_PRG + 2) << 16),	/* mov r2, @mem_start_prog[3]+[4] */

	(mov << 0) | (R3 << 8) | OP_DST_REG | (PRG_MAGIC << 16),				/* mov r3, prog magic low bits */
	(mov << 0) | (R4 << 8) | OP_DST_REG | ((PRG_MAGIC >> 16) << 16),			/* mov r4. prog magic high bits */

	(cmp << 0) | (R1 << 8) | OP_DST_REG | OP_SRC_REG | (3 << 16),				/* r1 == r3? */
	(brneq << 0) | ((MEM_START_ROM + PRG_HEADER_SIZE) << 16),				/* not eq. jump to start of ROM */

	(cmp << 0) | (R2 << 8) | OP_DST_REG | OP_SRC_REG | (4 << 16),				/* r2 == r4? */
	(brneq << 0) | ((MEM_START_ROM + PRG_HEADER_SIZE) << 16),				/* not eq. jump to start of ROM */
	(jmp << 0) | ((MEM_START_PRG + PRG_HEADER_SIZE) << 8),					/* run program */

	(nop << 0),
	(halt << 0),
};

#endif /* __ROM_H_ */
