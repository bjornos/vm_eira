#ifndef TESTPROGRAM_H_
#define TESTPROGRAM_H_

#include <stdint.h>
#include "opcodes.h"
#include "registers.h"

uint32_t program_unit_test_basic[] = {
	PROGRAM_MAGIC,
	(1 << 0),					/* binary format version */
	(1 << 0),					/* program start offset */
	(1024 << 0),				/* program size  */
	(nop << 0),
	(setposxy << 0) | 10  << 8 | (1 << 20),
	(screenout << 0) | ('T' << 8),
	(setposxy << 0) | 11  << 8 | (1 << 20),
	(screenout << 0) | ('S' << 8),
	(setposxy << 0) | 12  << 8 | (1 << 20),
	(screenout << 0) | ('T' << 8),
	(nop << 0),
	(halt << 0),
};

uint32_t program_unit_test[] = {
	PROGRAM_MAGIC,
	(1 << 0),		/* binary format version */
	(1 << 0),  		/* program start offset */
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
	PROGRAM_MAGIC,
	(1 << 0),		/* binary format version */
	(1 << 0),		/* program start offset */
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
	PROGRAM_MAGIC,
	(1 << 0),		/* binary format version */
	(1 << 0),		/* program start offset */
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
	(movi << 0) | (R1 << 8) | OP_SRC_MEM | OP_DST_REG | (MEM_START_PROGRAM << 16), 			/* mov r1, @mem_start_prog[0]+[2] */
	(movi << 0) | (R2 << 8) | OP_SRC_MEM | OP_DST_REG | ((MEM_START_PROGRAM + 2) << 16), 	/* mov r2, @mem_start_prog[3]+[4] */

	(mov << 0) | (R3 << 8) | OP_DST_REG | (PROGRAM_MAGIC << 16),							/* mov r3, prog magic low bits */
	(mov << 0) | (R4 << 8) | OP_DST_REG | ((PROGRAM_MAGIC >> 16) << 16),					/* mov r4. prog magic high bits */

	(cmp << 0) | (R1 << 8) | OP_DST_REG | OP_SRC_REG | (3 << 16),							/* r1 == r3? */
	(brneq << 0) | ((MEM_START_ROM + 16) << 16),											/* not eq. jump to start of ROM */

	(cmp << 0) | (R2 << 8) | OP_DST_REG | OP_SRC_REG | (4 << 16),							/* r2 == r4? */
	(brneq << 0) | ((MEM_START_ROM + 16) << 16),											/* not eq. jump to start of ROM */
	(jmp << 0) | ((MEM_START_PROGRAM + 16) << 8),											/* run program */

	(nop << 0),
	(halt << 0),
};

#endif /* TESTPROGRAM_H_ */
