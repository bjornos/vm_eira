#ifndef __CPU_H__
#define __CPU_H_

#include <stdint.h>

#include "registers.h"

struct _cpu_regs {
	uint16_t GP_REG[GP_REG_MAX];	/* general purpose registers */
	long pc;			/* program counter */
	int sp;				/* stack pointer */
	int cr;				/* conditional register */
	char exception;
	unsigned char reset;
	unsigned char dbg;	/* enable debug mode */
	unsigned char panic;		/* halt cpu */
};

void cpu_handle_exception(struct _cpu_regs *cpu_regs, uint32_t *instr);


#endif /* __CPU_H__ */
