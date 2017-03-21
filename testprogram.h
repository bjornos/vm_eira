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

#undef NDEBUG
#include <assert.h>

void test_result(uint16_t *GP_REG, uint8_t *RAM) {
	assert(*(GP_REG + 1) == 0xe4);
	assert(*(GP_REG + 6) == *(GP_REG + 1));
	assert(*(RAM + 8192) == 0xaa);
	assert(*(GP_REG + 0) == 0x44);
	assert(*(RAM + 8194) == 0x44);
	assert(*(GP_REG + 3) == 0x44);
	assert(*(GP_REG + 10) == 196);
	assert(*(GP_REG + 11) == 200);
	assert(*(RAM + 8196) == 0x12);
	assert(*(RAM + 8200) == 0x0f);
	assert(*(RAM + 8201) == 0x07);
	assert(*(GP_REG + 7) == *(GP_REG + 8));
}

uint32_t program_regression_test[] = {
	PRG_MAGIC_HEADER,	/* magic */
	(1 << 0),		/* reserved */
	(1 << 0),		/* reserved */
	(112 << 0),		/* code size  */
	(clrscr << 0),
	(setposxy << 0) | 10  << 8 | (1 << 20),
	(putchar << 0) | ('T' << 8),
	(setposxy << 0) | 11  << 8 | (1 << 20),
	(putchar << 0) | ('S' << 8),
	(setposxy << 0) | 12  << 8 | (1 << 20),
	(putchar << 0) | ('T' << 8),
	(nop << 0),

	(mov << 0) | (R1 << 8)  | OP_DST_REG | (0xe4 << 16), 			/* r1 = 0xe4 */
	(mov << 0) | (R6 << 8) | OP_SRC_REG | OP_DST_REG | (R1 << 16),		/* r10 = r1 */
	(mov << 0) | (R0 << 8)  | OP_DST_REG | (0xaa << 16),
	(mov << 0) | (R0 << 8) | OP_SRC_REG | OP_DST_MEM | (8192 << 16), 	/* RAM[8192] = 0xaa */

	(mov << 0) | (R0 << 8)  | OP_DST_REG | (0x44 << 16),			/* r0 = 0x44 */
	(mov << 0) | (R0 << 8) | OP_SRC_REG | OP_DST_MEM | (8194 << 16),	/* RAM[8194] = 0x44*/
	(mov << 0) | (R3 << 8) | OP_SRC_MEM | OP_DST_REG | (8194 << 16),	/* r3 = 0x44 */


	(mov << 0) | (R10 << 8)  | OP_DST_REG | (0x00 << 16), 			/* r10 = 0x00*/
	(mov << 0) | (R11 << 8)  | OP_DST_REG | (0x00 << 16), 			/* r11 = 0x00*/
	(mov << 0) | (R12 << 8)  | OP_DST_REG | (0x02 << 16), 			/* r12 = 0x02*/

	(add << 0) | (R10 << 8)  | OP_DST_REG | (198 << 16),			/* r10 = r10 + 198 (0 + 198)*/
	(add << 0) | (R11 << 8)  | OP_DST_REG | OP_SRC_REG | (10 << 16),	/* r11 = r11 + r10 (0 + 198)*/
	(add << 0) | (R11 << 8)  | OP_DST_REG | OP_SRC_REG | (12 << 16),	/* r11 = r11 + r12 (198 + 2) */

	(mov << 0) | (R13 << 8)  | OP_DST_REG | (0x10 << 16),
	(mov << 0) | (R13 << 8) | OP_SRC_REG | OP_DST_MEM | (8196 << 16), 	/* RAM[8196] = 0x10 */

	(add << 0) | (R12 << 8)  | OP_DST_MEM | OP_SRC_REG | (8196 << 16),	/* RAM[8196] = RAM[8196] + r12 (16 + 2)*/

	(sub << 0) | (10 << 8)  | OP_DST_REG | OP_SRC_REG | (12 << 16),		/* r10 = r10 - r12 (198 - 2) */

	(mov << 0) | (R15 << 8)  | OP_DST_REG | (1807 << 16),				/* r15 = 1807 */
	(movi << 0) | (R15 << 8)  | OP_DST_MEM | (8200 << 16),				/* RAM[8200] = 1024 */

	/* test cmp and branch instructions. */
	(mov << 0) | (R7 << 8)  | OP_DST_REG | (4 << 16),				/* r7=4  */
	(mov << 0) | (R8 << 8)  | OP_DST_REG | (1 << 16),				/* r8=1  */
//label test_cmp:  [addr 4212]
	(sub << 0) | (R7 << 8)  | OP_DST_REG | (1 << 16),				/* r7 = r7 - 1 */
	(cmp << 0) | (R7 << 8) | OP_DST_REG | OP_SRC_REG | (8 << 16),	/* r7 == r8? */
	(brneq << 0) | ((0x1074) << 16),								/* not eq. jump to @test_cmp */

	(halt << 0),
};

#endif /* TESTPROGRAM_H_ */
