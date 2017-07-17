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
#include "display.h"

const uint32_t program_reset[] = {
	(jmp << 0) | ((MEM_START_ROM + 4) << 8),
	(nop << 0),
	(halt << 0),
};

const uint32_t rom[] = {
	PRG_MAGIC_HEADER,	/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(126 << 0),		/* program size  */

	/* set default outport state */
	(mov << 0) | (R0 << 8)  | OP_DST_REG | (0x8001 << 16),			/* binary ones on each side */
	(movi << 0) | (R0 << 8)  | OP_DST_MEM | (MEM_START_IO_OUTPUT << 16),

	(dimd << 0) | (mode_40x12 << 8),
	(diwtrt << 0),
	(diclr << 0),
	(disetxy << 0) | 10  << 8 | (1 << 20),
	(dichar << 0) | ('e' << 8),
	(disetxy << 0) | 11  << 8 | (1 << 20),
	(dichar << 0) | ('i' << 8),
	(disetxy << 0) | 12  << 8 | (1 << 20),
	(dichar << 0) | ('r' << 8),
	(disetxy << 0) | 13  << 8 | (1 << 20),
	(dichar << 0) | ('a' << 8),
	(disetxy << 0) | 14  << 8 | (1 << 20),
	(dichar << 0) | ('-' << 8),
	(disetxy << 0) | 15  << 8 | (1 << 20),
	(dichar << 0) | ('1' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('|' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('/' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('-' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('\'' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('|' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('/' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('-' << 8),
	(disetxy << 0) | 12  << 8 | (2 << 20),
	(dichar << 0) | ('\'' << 8),

	/* jump to program memory and start any program loaded
	 * if no program is loaded we will end up in MEM_START_ROM again.
	 */
	(jmp << 0) |  (MEM_START_PRG << 8),

	(nop << 0),
	(halt << 0),
};


#endif /* __ROM_H_ */
