#ifndef __CPU_H__
#define __CPU_H_

#include <stdint.h>

#include "registers.h"

enum conditions {
	COND_EQ = 1,
	COND_NEQ = 2,
	COND_ZERO = 4,
	COND_NZERO = 8,
	COND_GR = 16,
	COND_LE = 32,
	COND_UNDEF = 64,
};

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

void cpu_reset(struct _cpu_regs *cpu_regs, uint32_t reset_vector);


#endif /* __CPU_H__ */
