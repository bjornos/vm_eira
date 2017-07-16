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

#include "exception.h"
#include "opcodes.h"
#include "memory.h"
#include "display.h"
#include "utils.h"
#include "machine.h"

static struct _dbg dbg_info[DBG_HISTORY];
static int dbg_index = 0;

__inline__ static  void compare(struct _cpu_regs *cpu_regs, uint16_t c1, uint16_t c2)
{
	int comp = c2 - c1;

	debug_args(dbg_info, dbg_index, (uint16_t*)&c1, (uint16_t*)&c1);

	cpu_regs->cr =
		(comp == 0) ? (COND_EQ | COND_ZERO) :
		(comp > 0) ? (COND_GR | COND_NEQ) :
		(comp < 0) ? (COND_LE | COND_NEQ) :
		COND_UNDEF;

	debug_result(dbg_info, dbg_index, (long*)&cpu_regs->cr);
}

__inline__ static void branch(struct _cpu_regs *cpu_regs, enum conditions cond, uint16_t addr)
{
	debug_args(dbg_info, dbg_index, (uint16_t *)&cpu_regs->cr, (uint16_t *)&cond);


	if (cpu_regs->cr & cond) {
		cpu_regs->pc = addr - sizeof(uint32_t);
	}
	debug_result(dbg_info, dbg_index, &cpu_regs->pc);
	cpu_regs->cr = COND_UNDEF;
}


static uint16_t cpu_decode_mnemonic(struct _cpu_regs *cpu_regs, uint8_t *RAM, uint32_t *instr, uint16_t **dst, int opsize)
{
	uint16_t local_src;
	uint16_t local_dst;
	uint16_t src;

	/* value destination general purpose register */
	if (*instr & OP_DST_REG) {
		local_dst = (*instr >> 8) & 0x0f;
		local_src = (*instr >> 16) & 0xffff;

		*(dst) = cpu_regs->GP_REG + local_dst;

		if (*instr & OP_SRC_REG) {
			/* value from register */
			if (local_src > GP_REG_MAX) {
				cpu_regs->exception = EXC_REG;
				goto mnemonic_out;
			}
			src = cpu_regs->GP_REG[local_src];
			goto mnemonic_out;
		}
		if (*instr & OP_SRC_MEM) {
			/* copy from memory */
			if (local_src > RAM_SIZE) {
				cpu_regs->exception = EXC_MEM;
				goto mnemonic_out;
			}
			if (opsize == SIZE_INT) {
				src = RAM[local_src];
				src |= RAM[local_src + 1] << 8;
			} else
				src = RAM[local_src];

			goto mnemonic_out;
		}
		/* immediate value */
		src = local_src;
		goto mnemonic_out;
	}

	/* value destination memory */
	if (*instr & OP_DST_MEM) {
		local_src = (*instr >> 8) & 0x0f;
		local_dst = (*instr >> 16) & 0xffff;

		if (local_dst > RAM_SIZE) {
			cpu_regs->exception = EXC_MEM;
			goto mnemonic_out;
		}

		*(dst) = (void*)RAM + local_dst;
		if (opsize == SIZE_INT)
			src = cpu_regs->GP_REG[local_src] & 0xffff;
		else
			src = cpu_regs->GP_REG[local_src] & 0xff;
	}

mnemonic_out:
		debug_args(dbg_info, dbg_index, &local_dst, &src);

		return src;
}

void cpu_decode_instruction(struct _cpu_regs *cpu_regs, uint8_t *RAM, struct _display_adapter *display)
{
	uint8_t opcode;
	uint32_t *instr;
	uint16_t src;
	uint16_t *dst;
	uint16_t addr;

	if (cpu_regs->exception)
		return;

	instr = (uint32_t *)&RAM[cpu_regs->pc];

	debug_instr(dbg_info, dbg_index, instr);

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop:
			debug_opcode(dbg_info, dbg_index, "nop");
			break;
		case halt:
			debug_opcode(dbg_info, dbg_index, "halt");
			cpu_regs->panic = 1;
			break;
		case mov:
			debug_opcode(dbg_info, dbg_index, "mov");
			src = cpu_decode_mnemonic(cpu_regs, RAM, instr, &dst, SIZE_BYTE);
			if (!cpu_regs->exception)
				*dst = src;
			break;
		case movi:
			debug_opcode(dbg_info, dbg_index, "movi");
			src = cpu_decode_mnemonic(cpu_regs, RAM, instr, &dst, SIZE_INT);
			if (!cpu_regs->exception)
				*dst = src;
			break;
		case add:
			debug_opcode(dbg_info, dbg_index, "add");
			src = cpu_decode_mnemonic(cpu_regs, RAM, instr, &dst,SIZE_BYTE);
			if (!cpu_regs->exception)
				*dst += src;
			break;
		case sub:
			debug_opcode(dbg_info, dbg_index, "sub");
			src = cpu_decode_mnemonic(cpu_regs, RAM, instr, &dst,SIZE_BYTE);
			if (!cpu_regs->exception)
				*dst -= src;
			break;
		case jmp:
			debug_opcode(dbg_info, dbg_index, "jmp");
			cpu_regs->pc = (*instr >> 8) - sizeof(uint32_t); /* compensate for pc++ */
			debug_result(dbg_info, dbg_index, &cpu_regs->pc);
			break;
		case cmp:
			debug_opcode(dbg_info, dbg_index, "cmp");
			cpu_regs->cr &= COND_UNDEF;
			src = cpu_decode_mnemonic(cpu_regs, RAM, instr, &dst, SIZE_INT);
			compare(cpu_regs, src, *dst);
			debug_args(dbg_info, dbg_index, &src, dst);
			debug_result(dbg_info, dbg_index, (long *)&cpu_regs->cr);
			break;
		case breq:
			debug_opcode(dbg_info, dbg_index, "breq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(cpu_regs, COND_EQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					cpu_regs->exception = EXC_INSTR;
				else
					branch(cpu_regs, COND_EQ, cpu_regs->GP_REG[addr]);
			}
			break;
		case brneq:
			debug_opcode(dbg_info, dbg_index, "brneq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(cpu_regs, COND_NEQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					cpu_regs->exception = EXC_INSTR;
				else
					branch(cpu_regs, COND_NEQ, cpu_regs->GP_REG[addr]);
			}
			break;
		case stopc:
			debug_opcode(dbg_info, dbg_index, "stopc");
			addr = (*instr >> 8);
			debug_args(dbg_info, dbg_index, &addr, NULL);
			debug_result(dbg_info, dbg_index, &cpu_regs->pc);
			cpu_regs->GP_REG[(*instr >> 8)] = cpu_regs->pc;
			break;
		case dimd:
		case diclr:
		case diwtrt:
		case setposxy:
		case pchar:
			debug_opcode(dbg_info, dbg_index, "<gpu code>");
			cpu_regs->gpu_request = 1;
			break;
		default: cpu_regs->exception = EXC_INSTR;
			break;
	}

	if (cpu_regs->dbg) {
		//display_wait_retrace(display);
		//display->enabled = 0;
		gotoxy(1,15);
		dump_instr(dbg_info, dbg_index);
		gotoxy(1,15 + DBG_HISTORY + 4);
		dump_regs(cpu_regs->GP_REG);
		//display->enabled = 1; /* fixme: bug*/
	}

}

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
	case EXC_GPU:
			printf("display error ");
			break;
	case EXC_IOPORT:
			printf("I/O error ");
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

long cpu_fetch_instruction(struct _cpu_regs *cpu_regs) {
	/* each instruction is 4 bytes */
	cpu_regs->pc += sizeof(uint32_t);
	if (cpu_regs->pc >= (RAM_SIZE - 3))
		cpu_regs->exception = EXC_PRG;

	dbg_index = (dbg_index + 1) % DBG_HISTORY;
	memset(dbg_info + dbg_index, 0x00, sizeof(struct _dbg));

	cpu_regs->gpu_request = 0;

	return cpu_regs->pc;
}

void cpu_reset(struct _cpu_regs *cpu_regs, uint32_t reset_vector) {
	memset(cpu_regs->GP_REG, 0x00, GP_REG_MAX);

	cpu_regs->reset = 1;
	cpu_regs->sp = 0;
	cpu_regs->exception = EXC_NONE;
	cpu_regs->panic = 0;
	cpu_regs->cr = COND_UNDEF;
	cpu_regs->dbg = 0;
	cpu_regs->gpu_request = 0;

	cpu_regs->pc = reset_vector;

	// (if regs->dbg)
	for (int d=0; d < DBG_HISTORY; d++)
		memset(dbg_info + d, 0x00, sizeof(struct _dbg));

	dbg_index = 0;

}


void *cpu_machine(void *mach)
{
	struct _machine *machine = mach;
	struct timespec cpu_clk_freq;
	long instr_p;
	int hz = 10; /* 10 Hz */

	cpu_clk_freq.tv_nsec = 1000000000 / hz;
	cpu_clk_freq.tv_sec = 0;

	while(!machine->cpu_regs.panic) {
		while(machine->cpu_regs.reset);

		instr_p = cpu_fetch_instruction(&machine->cpu_regs);
		cpu_decode_instruction(&machine->cpu_regs, machine->RAM, &machine->display);

		if (machine->cpu_regs.gpu_request)  {
			gpu_add_instr(&machine->gpu, (uint32_t *)&machine->RAM[instr_p]);
		}

		if (machine->cpu_regs.exception) {
			display_wait_retrace(&machine->display);
			cpu_handle_exception(&machine->cpu_regs, (uint32_t *)&machine->RAM[instr_p]);
		}

		nanosleep(&cpu_clk_freq, NULL);
	}

	pthread_exit(NULL);
}


