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
#include "vdc.h"
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

	debug_result(dbg_info, dbg_index, (unsigned long)cpu_regs->cr);
}

__inline__ static void branch(struct _cpu_regs *cpu_regs, enum conditions cond, uint16_t addr)
{
	debug_args(dbg_info, dbg_index, (uint16_t *)&cpu_regs->cr, (uint16_t *)&cond);


	if (cpu_regs->cr & cond) {
		cpu_regs->pc = addr - sizeof(uint32_t);
	}
	debug_result(dbg_info, dbg_index, cpu_regs->pc);
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

		if ((local_dst > RAM_SIZE) || (local_dst < MEM_START_RW)) {
			cpu_regs->exception = EXC_MEM;
			goto mnemonic_out;
		}
		*(dst) = (uint16_t *)((uint8_t *)RAM + local_dst);
		if (opsize == SIZE_INT)
			src = cpu_regs->GP_REG[local_src] & 0xffff;
		else
			src = cpu_regs->GP_REG[local_src] & 0xff;
	}

mnemonic_out:
		debug_args(dbg_info, dbg_index, &local_dst, &src);

		return src;
}

static void cpu_decode_instruction(void *mach)
{
	struct _machine *machine = mach;
	uint8_t opcode;
	uint32_t *instr;
	uint16_t src;
	uint16_t *dst;
	uint16_t addr;
	uint16_t arg1;
	uint16_t arg2;

	if (machine->cpu_regs.exception)
		return;

	instr = (uint32_t *)&machine->RAM[machine->cpu_regs.pc];

	debug_instr(dbg_info, dbg_index, instr);

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop:
			debug_opcode(dbg_info, dbg_index, "nop");
			break;
		case halt:
			debug_opcode(dbg_info, dbg_index, "halt");
			machine->cpu_regs.panic = 1;
			break;
		case mov:
			debug_opcode(dbg_info, dbg_index, "mov");
			src = cpu_decode_mnemonic(&machine->cpu_regs, machine->RAM, instr, &dst, SIZE_BYTE);
			if (!machine->cpu_regs.exception);
				*dst = src;
			break;
		case movi:
			debug_opcode(dbg_info, dbg_index, "movi");
			src = cpu_decode_mnemonic(&machine->cpu_regs, machine->RAM, instr, &dst, SIZE_INT);
			if (!machine->cpu_regs.exception)
				*dst = src;
			break;
		case add:
			debug_opcode(dbg_info, dbg_index, "add");
			src = cpu_decode_mnemonic(&machine->cpu_regs, machine->RAM, instr, &dst,SIZE_BYTE);
			if (!machine->cpu_regs.exception)
				*dst += src;
			break;
		case sub:
			debug_opcode(dbg_info, dbg_index, "sub");
			src = cpu_decode_mnemonic(&machine->cpu_regs, machine->RAM, instr, &dst,SIZE_BYTE);
			if (!machine->cpu_regs.exception)
				*dst -= src;
			break;
		case jmp:
			debug_opcode(dbg_info, dbg_index, "jmp");
			if ((*instr >> 8) > MEM_START_ROM) {
				machine->cpu_regs.pc = (*instr >> 8) - sizeof(uint32_t); /* compensate for pc++ */
			} else {
				if ((*instr >> 8) > GP_REG_MAX)
					machine->cpu_regs.exception |= EXC_MEM;
				else
					machine->cpu_regs.pc = machine->cpu_regs.GP_REG[(*instr >> 8)] - sizeof(uint32_t);
			}

			debug_result(dbg_info, dbg_index, machine->cpu_regs.pc);
			break;
		case cmp:
			debug_opcode(dbg_info, dbg_index, "cmp");
			machine->cpu_regs.cr &= COND_UNDEF;
			src = cpu_decode_mnemonic(&machine->cpu_regs, machine->RAM, instr, &dst, SIZE_INT);
			compare(&machine->cpu_regs, src, *dst);
			debug_args(dbg_info, dbg_index, &src, dst);
			debug_result(dbg_info, dbg_index, (unsigned long)machine->cpu_regs.cr);
			break;
		case breq:
			debug_opcode(dbg_info, dbg_index, "breq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(&machine->cpu_regs, COND_EQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					machine->cpu_regs.exception |= EXC_MEM;
				else
					branch(&machine->cpu_regs, COND_EQ, machine->cpu_regs.GP_REG[addr]);
			}
			break;
		case brneq:
			debug_opcode(dbg_info, dbg_index, "brneq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(&machine->cpu_regs, COND_NEQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					machine->cpu_regs.exception |= EXC_MEM;
				else
					branch(&machine->cpu_regs, COND_NEQ, machine->cpu_regs.GP_REG[addr]);
			}
			break;
		case stopc:
			debug_opcode(dbg_info, dbg_index, "stopc");
			addr = (*instr >> 8);
			debug_args(dbg_info, dbg_index, &addr, NULL);
			debug_result(dbg_info, dbg_index, machine->cpu_regs.pc);
			machine->cpu_regs.GP_REG[(*instr >> 8)] = machine->cpu_regs.pc;
			break;
		case rst:
			debug_opcode(dbg_info, dbg_index, "rst");
			machine->cpu_regs.exception |= EXC_PRG;
			break;
		case movmr:
			arg1 = (*instr >> 8) & 0xf;
			arg2 = machine->cpu_regs.GP_REG[(*instr >> 12) & 0xf];
			debug_opcode(dbg_info, dbg_index, "movmr");
			debug_args(dbg_info, dbg_index, &arg1, &arg2);
			if ((arg1 > GP_REG_MAX))
				machine->cpu_regs.exception |= EXC_MEM;
			else {
				machine->cpu_regs.GP_REG[arg1] = machine->RAM[arg2];
				debug_result(dbg_info, dbg_index,  machine->cpu_regs.GP_REG[arg1]);
			}
			break;
		case diwait:
			debug_opcode(dbg_info, dbg_index, "diwait");
			machine->cpu_regs.vdc_request = 1;
			break;
		case dimd:
			debug_opcode(dbg_info, dbg_index, "dimd");
			machine->cpu_regs.vdc_request = 1;
			break;
		case diclr:
			debug_opcode(dbg_info, dbg_index, "diclr");
			machine->cpu_regs.vdc_request = 1;
			break;
		case diwtrt:
			debug_opcode(dbg_info, dbg_index, "diwtrt");
			machine->cpu_regs.vdc_request = 1;
			break;
		case disetxy:
			debug_opcode(dbg_info, dbg_index, "setposxy");
			machine->cpu_regs.vdc_request = 1;
			break;
		case dichar:
			debug_opcode(dbg_info, dbg_index, "putchar");
			machine->cpu_regs.vdc_request = 1;
			break;
		case diputpixel:
			debug_opcode(dbg_info, dbg_index, "putpixel");
			machine->cpu_regs.vdc_request = 1;
			break;
		default: machine->cpu_regs.exception |= EXC_INSTR;
			break;
	}

	if (machine->cpu_regs.dbg) { /* consider move this into debug_opcode so that it will show in case of hang */
		while(machine->vdc_regs.display.refresh)
			usleep(1000);
		machine->vdc_regs.display.enabled = 0;
		vdc_gotoxy(1,15);
		dump_instr(dbg_info, dbg_index);
		vdc_gotoxy(1,15 + DBG_HISTORY + 4);
		dump_regs(machine->cpu_regs.GP_REG);
		machine->vdc_regs.display.enabled = 1; /* fixme: bug*/
	}

}

static void cpu_handle_exception(void *mach)
{
	struct _machine *machine = mach;
	int exception = 0;

	printf("\n!! %s: 0x%x\n",__func__, machine->cpu_regs.exception);

	machine->cpu_regs.panic = 1;

	for (int e = 0; e <= 31; e++) {
		exception = (1 << e);
		if (machine->cpu_regs.exception & exception) {
			switch(exception) {
			case EXC_INSTR:
				printf("* Illegal instruction@0x%X \n",
					(uint32_t)machine->RAM[machine->cpu_regs.pc]);
			break;
			case EXC_MEM:
				printf("* Cannot access memory\n");
			break;
			case EXC_REG:
				printf("* Cannot access register\n");
			break;
			case EXC_PRG:
				printf("* Stray program\n");
			break;
			case EXC_VDC:
			case EXC_DISP:
				printf("* VDC error\n");
			break;
			case EXC_IOPORT:
				printf("* I/O error\n");
			break;
			case EXC_SHUTDOWN:
				printf("* Machine shutdown\n");
			break;
			default:
				printf("? Unknown exception\n");
			break;
			}
		}
	}

	printf("[pc: %lu]\n", machine->cpu_regs.pc);
}

static void cpu_fetch_instruction(struct _cpu_regs *cpu_regs)
{
	/* each instruction is 4 bytes */
	cpu_regs->pc += sizeof(uint32_t);
	if (cpu_regs->pc >= (RAM_SIZE - 3))
		cpu_regs->exception = EXC_PRG;

	dbg_index = (dbg_index + 1) % DBG_HISTORY;
	memset(dbg_info + dbg_index, 0x00, sizeof(struct _dbg));

	cpu_regs->vdc_request = 0;
}

void cpu_reset(void *mach)
{
	struct _machine *machine = mach;

	memset(machine->cpu_regs.GP_REG, 0x00, GP_REG_MAX);

	machine->cpu_regs.reset = 1;
	machine->cpu_regs.sp = 0;
	machine->cpu_regs.exception = EXC_NONE;
	machine->cpu_regs.panic = 0;
	machine->cpu_regs.cr = COND_UNDEF;
	machine->cpu_regs.dbg = 0;
	machine->cpu_regs.vdc_request = 0;
	machine->cpu_regs.pc = MACHINE_RESET_VECTOR;
	machine->cpu_regs.mclk = MACHINE_MASTER_CLOCK / 20; /* 70 Hz */

	// (if regs->dbg)
	for (int d=0; d < DBG_HISTORY; d++)
		memset(dbg_info + d, 0x00, sizeof(struct _dbg));

	dbg_index = 0;

}


void *cpu_machine(void *mach)
{
	struct _machine *machine = mach;
	struct timespec cpu_clk_freq;

	cpu_clk_freq.tv_sec = 0;

	while(!machine->cpu_regs.panic) {

		cpu_clk_freq.tv_nsec = 1000000000 / machine->cpu_regs.mclk;

		while(machine->cpu_regs.reset) {
			nanosleep(&cpu_clk_freq, NULL);
		}

		cpu_fetch_instruction(&machine->cpu_regs);
		cpu_decode_instruction(machine);

		if (machine->cpu_regs.vdc_request)
			machine->cpu_regs.exception |=
				vdc_add_instr(&machine->vdc_regs,
					(uint32_t *)&machine->RAM[machine->cpu_regs.pc]);

		if (machine->cpu_regs.exception)
			cpu_handle_exception(machine);

		nanosleep(&cpu_clk_freq, NULL);
	}

	pthread_exit(NULL);
}


