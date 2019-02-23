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
#include "vdc.h"

const uint32_t program_reset[] = {
	/* jump back to start of ROM */
	(jmp << 0) | ((MEM_START_ROM + (sizeof(struct _prg_header) / sizeof(uint32_t))) << 8),
	(nop << 0),
	(halt << 0),
};

/*
 * ROM Register Usage
 *
 * R1: Holds marker for end of text -- fixme: should not be needed.
 * R4: Pos X
 * R5: Pos Y
 * R6: Character index pointer
 * R7: Program loading flag
 * R10: Label
*/
const uint32_t rom[] = {
	PRG_MAGIC_HEADER,	/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(160 << 0),		/* program size  */

	/* set default outport state */
	(mov << 0) | (R0 << 8)  | OP_DST_REG | (0x8001 << 16),			/* binary ones on each side */
	(movi << 0) | (R0 << 8)  | OP_DST_MEM | (MEM_IO_OUTPUT << 16),

	(dimd << 0) | (mode_40x12 << 8),
	(diwtrt << 0),

	(diclr << 0),

	(diwtrt << 0),

	(mov << 0) | (R1 << 8)  | OP_DST_REG | (0x00 << 16), 			/* r1 = value of "end" marker */
	(mov << 0) | (R4 << 8)  | OP_DST_REG | (0x0A << 16), 			/* r4 = pos x = 10*/
	(mov << 0) | (R5 << 8)  | OP_DST_REG | (0x01 << 16), 			/* r5 = pos y = 1*/

	(mov << 0) | (R6 << 8)  | OP_DST_REG | (MEM_ROM_BOOT_MSG << 16), 	/* r6 = @ mem boot msg */

	/* store pc. effectively creating a label that can be used with jmp */
	(stopc << 0) | (R10 << 8),

	(disetxy << 0) | R4  << 8 | (R5 << 20),			/* print chars from R6*/
	(dichar << 0) | OP_SRC_MEM | (R6 << 16),

	(add << 0) | (R4 << 8)  | OP_DST_REG | (0x01 << 16),			/* r4++ - x pos ++ */
	(add << 0) | (R6 << 8)  | OP_DST_REG | (0x01 << 16),			/* r6++ - next char */

	/* check string is \0 */
	(movmr << 0) | (R0 << 8)  | R6 << 12,  /* r0 = RAM[ r6 ]*/
	(cmp << 0) | (R0 << 8) | OP_SRC_REG | OP_DST_REG | (R1 << 16),
	(brneq << 0) | (R10 << 16),						/* print next char if not at end */

	(mov << 0) | (R4 << 8)  | OP_DST_REG | (0x0C << 16), 			/* r4 = pos x = 12 */
	(mov << 0) | (R5 << 8)  | OP_DST_REG | (0x02 << 16), 			/* r5 = pos y = 2 */

	(mov << 0) | (R6 << 8)  | OP_DST_REG | (MEM_ROM_BOOT_ANIM << 16), 	/* r6 = @ mem boot anim */

	(stopc << 0) | (R10 << 8),						/* store pc for new label */

	(disetxy << 0) | R4  << 8 | (R5 << 20),
	(dichar << 0) | OP_SRC_MEM | (R6 << 16),

	(add << 0) | (R6 << 8)  | OP_DST_REG | (0x01 << 16),			/* r6++ - next char */

	/* check string is \0 */
	(movmr << 0) | (R0 << 8)  | R6 << 12,
	(cmp << 0) | (R0 << 8) | OP_SRC_REG | OP_DST_REG | (R1 << 16),
	(brneq << 0) | (R10 << 16),

	/* check if a program is being loaded */
	(mov << 0) | (R7 << 8)  | OP_DST_REG | (PRG_LOADING << 16),
	(cmp << 0) | (R7 << 8) | OP_DST_REG | OP_SRC_MEM | (MEM_PRG_LOADING << 16),
	/* if no program is being loaded, jump to program memory and start
	 * any program previously loaded. if no program has been loaded
	 * we will end up in MEM_START_ROM again.
	 */
	(brneq << 0) | (MEM_START_PRG << 16),

	/* mark that a program currently is being loaded */
	(mov << 0) | (R4 << 8)  | OP_DST_REG | (0x0F << 16), 	/* xpos = 15 */
	(mov << 0) | (R5 << 8)  | OP_DST_REG | (0x02 << 16), 	/* ypos = 2 */
	(disetxy << 0) | R4  << 8 | (R5 << 20),
	(dichar << 0) | ('$' << 8),

	/* keep spinning the wheel */
	(jmp << 0) | R10 << 8,

	(nop << 0),
	(halt << 0),
};


#endif /* __ROM_H_ */
