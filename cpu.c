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

