#ifndef TESTPROGRAM_H_
#define TESTPROGRAM_H_

#include <stdint.h>
#include "opcodes.h"
#include "registers.h"


uint32_t program_unit_test[] = {
		64,
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
		64,
		(nop << 0),
		(add << 0) | (R1 << 8)  | OP_DST_REG | (64 << 16),
		(add << 0) | (R0 << 8)  | OP_DST_REG | OP_SRC_REG | (3 << 16),
		(add << 0) | (R7 << 8)  | OP_DST_REG | OP_SRC_REG | (6 << 16),
		(add << 0) | (R2 << 8)  | OP_DST_MEM | OP_SRC_REG | (61 << 16),
		(sub << 0) | (R1 << 8)  | OP_DST_REG | OP_SRC_REG | (0 << 16),
		(halt << 0),
};


uint32_t rom[] = {
		1024,
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
		 * but until those details are worked out, we don't do anything */
		(mov << 0) | (R1 << 8) | OP_SRC_MEM | OP_DST_REG | (8192 << 16),
		//(cmp << 0) | (R1 << 8) | (0xcb << 16),
		//(cmp << 0) | (R1 << 8) | OP_DST_REG | OP_SRC_MEM | (8192 << 16),
		(mov << 0) | (R2 << 8) | OP_DST_REG | (0xcb << 16),
		(cmp << 0) | (R1 << 8) | OP_DST_REG | OP_SRC_REG | (2 << 16),
		(breq << 0) | (0x204 << 16), /* jump to start of ROM */
		(nop << 0),
		(halt << 0),
};

#endif /* TESTPROGRAM_H_ */
