#include <stdio.h>
#include <string.h>

#include "cpu.h"
#include "exception.h"


void cpu_handle_exception(struct _cpu_regs *cpu_regs, uint32_t *instr) {
	printf("!! %s: ",__func__);

	switch(cpu_regs->exception) {
	case EXC_INSTR:
			printf("illegal instruction 0x%X ",	*instr);//machine.RAM[pc]
			break;
	case EXC_MEM:
			printf("cannot access memory ");
			break;
	case EXC_REG:
			printf("cannot access register ");
			break;
	case EXC_PRG:
			printf("stray program ");
			break;
	case EXC_DISP:
			printf("display error ");
			break;
	case EXC_SHUTDOWN:
			printf("machine shutdown ");
			break;
	default:
			printf("unknown exception %d ",
				cpu_regs->exception);
			break;
	}

	printf("[pc: %ld]\n", cpu_regs->pc);

	cpu_regs->panic = 1;
}

void cpu_reset(struct _cpu_regs *cpu_regs, uint32_t reset_vector) {
	memset(cpu_regs->GP_REG, 0x00, GP_REG_MAX);

	cpu_regs->reset = 1;
	cpu_regs->sp = 0;
	cpu_regs->exception = EXC_NONE;
	cpu_regs->panic = 0;
	cpu_regs->cr = COND_UNDEF;
	cpu_regs->dbg = 0;

	cpu_regs->pc = reset_vector;
}

