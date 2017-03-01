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
#include <unistd.h>
#include <argp.h>
#include <signal.h>

#include "opcodes.h"
#include "display.h"
#include "memory.h"
#include "testprogram.h"
#include "prg.h"
#include "rom.h"
#include "utils.h"

#define DBG(x)
#define CPU_VERSION	0x1


enum op_size {
	SIZE_BYTE,
	SIZE_INT,
};

enum conditions {
	COND_EQ = 1,
	COND_NEQ = 2,
	COND_ZERO = 4,
	COND_NZERO = 8,
	COND_GR = 16,
	COND_LE = 32,
	COND_UNDEF = 64,
};


typedef struct {
	int debug;
	int regression;
} args_t;

struct _cpu_regs {
	uint16_t GP_REG[GP_REG_MAX]; /* General Purpose Registers */
	long pc;
	int sp;
	int cr;
	char exception;
	int panic;
};

static struct _machine {
	uint8_t RAM[RAM_SIZE];
	struct _cpu_regs cpu_regs;
	struct _display_adapter display;
} machine;

static struct argp_option opts[] = {
	{"debug", 'd', 0, OPTION_ARG_OPTIONAL, "Enable debug"},
	{"regression", 'r', "test", OPTION_ARG_OPTIONAL, "Run regression test program"},
	{0}
};


static char* doc = "";
static char* args_doc = "";
static args_t args;


void sig_handler(int signo)
{
	if (signo == SIGINT) {
		machine.cpu_regs.panic = 1;
		args.debug = 1;
	}
}


error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	args_t* args = state->input;

	switch (key) {
		case 'd':
			args->debug = 1;
			break;
		case 'r':
			args->regression = 1;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}


uint16_t decode_mnemonic(uint32_t *instr, uint16_t **dst, int opsize, const char dbg_instr[])
{
	uint16_t local_src;
	uint16_t local_dst;
	uint16_t src;

	DBG(printf("%s %s --- ",__func__, dbg_instr));

	/* value destination general purpose register */
	if (*instr & OP_DST_REG) {
		local_dst = (*instr >> 8) & 0x0f;
		local_src = (*instr >> 16) & 0xffff;

		*(dst) = machine.cpu_regs.GP_REG + local_dst;

		if (*instr & OP_SRC_REG) {
			/* value from register */
			DBG(printf("GP_REG%d -> GP_REG%d\n",local_src, local_dst));
			if (local_src > GP_REG_MAX) {
				machine.cpu_regs.exception = EXC_REG;
				goto mnemonic_out;
			}
			src = machine.cpu_regs.GP_REG[local_src];
			goto mnemonic_out;
		}
		if (*instr & OP_SRC_MEM) {
			/* copy from memory */
			DBG(printf("0x%x -> GP_REG%d\n",local_src,local_dst));
			if (local_src > RAM_SIZE) {
				machine.cpu_regs.exception = EXC_MEM;
				goto mnemonic_out;
			}
			if (opsize == SIZE_INT) {
				src = machine.RAM[local_src];
				src |= machine.RAM[local_src + 1] << 8;
			} else
				src = machine.RAM[local_src];

			DBG(printf("RAM SRC = 0x%x  opsize=%d\n",src,opsize));
			goto mnemonic_out;
		}
		/* immediate value */
		DBG(printf("%d -> GP_REG%d\n",local_src,local_dst));
		src = local_src;
		goto mnemonic_out;
	}

	/* value destination memory */
	if (*instr & OP_DST_MEM) {
		local_src = (*instr >> 8) & 0x0f;
		local_dst = (*instr >> 16) & 0xffff;
		DBG(printf("GP_REG%d -> @%d\n",local_src,local_dst));

		if (local_dst > RAM_SIZE) {
			machine.cpu_regs.exception = EXC_MEM;
			goto mnemonic_out;
		}

		*(dst) = (void*)machine.RAM + local_dst;
		if (opsize == SIZE_INT)
			src = machine.cpu_regs.GP_REG[local_src] & 0xffff;
		else
			src = machine.cpu_regs.GP_REG[local_src] & 0xff;
	}

mnemonic_out:
		return src;
}


static __inline__ void compare(uint16_t c1, uint16_t c2)
{
	int d = c2 - c1;

	DBG(printf("%s --- ",__func__));

	machine.cpu_regs.cr =
			(d == 0) ? (COND_EQ | COND_ZERO) :
			(d > 0) ? (COND_GR | COND_NEQ) :
			(d < 0) ? (COND_LE | COND_NEQ) :
			COND_UNDEF;

	DBG(printf("c1:%d c2:%d d:%d desc: %d\n",c1,c2,d,machine.cpu_regs.cr));
}


static __inline__ void branch(enum conditions cond, uint16_t addr)
{
	DBG(printf("%s ?: ",__func__));

	if (machine.cpu_regs.cr & cond) {
		machine.cpu_regs.pc = addr - 4; /* compensate for pc + 4 each cycle */
		DBG(printf("jump %ld",machine.cpu_regs.pc));
	}
	DBG(printf("\n"));

	machine.cpu_regs.cr = COND_UNDEF;
}


static void cpu_decode_instruction(uint32_t *instr)
{
	uint8_t opcode;
	uint16_t src;
	uint16_t *dst;

	if (machine.cpu_regs.exception)
		return;

	DBG(printf("%s 0x%x\n",__func__, *instr));

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop: DBG(printf("nop\n"));
			break;
		case halt: DBG(printf("halt\n"));
			machine.cpu_regs.panic = 1;
			break;
		case mov:
			src = decode_mnemonic(instr, &dst, SIZE_BYTE, "mov");
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case movi:
			src = decode_mnemonic(instr, &dst, SIZE_INT,"movi");
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case add:
			src = decode_mnemonic(instr, &dst,SIZE_BYTE, "add");
			if (!machine.cpu_regs.exception)
				*dst += src;
			break;
		case sub: DBG(printf("sub\n"));
			src = decode_mnemonic(instr, &dst,SIZE_BYTE,"sub");
			if (!machine.cpu_regs.exception)
				*dst -= src;
			break;
		case jmp: DBG(printf("jmp "));
			machine.cpu_regs.pc = (*instr >> 8) - 4; /* compensate for pc++ */
			DBG(printf("%ld\n", machine.cpu_regs.pc));
			break;
		case cmp: DBG(printf("cmp "));
			machine.cpu_regs.cr &= COND_UNDEF;
			src = decode_mnemonic(instr, &dst, SIZE_INT, "cmp");
			compare(src, *dst);
			DBG(printf("cr: 0x%x\n", machine.cpu_regs.cr));
			break;
		case breq: DBG(printf("breq "));
			branch(COND_EQ, (*instr >> 16));
			break;
		case brneq: DBG(printf("brneq "));
			branch(COND_NEQ, (*instr >> 16));
			break;
		case clrscr: DBG(printf("clrscr\n"));
			display_request(&machine.display, instr, display_clr, machine.RAM);
			break;
		case setposxy: DBG(printf("setposxy\n"));
			display_request(&machine.display, instr, display_setxy, machine.RAM);
			break;
		case putchar: DBG(printf("putchar\n"));
			display_request(&machine.display, instr, display_setc, machine.RAM);
			break;
		default: machine.cpu_regs.exception = EXC_INSTR;
			break;
	}
}


static void cpu_reset(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);
	memset(&machine.cpu_regs.GP_REG, 0x00, GP_REG_MAX);

	machine.cpu_regs.pc = 0;
	machine.cpu_regs.sp = 0;
	machine.cpu_regs.exception = 0;
	machine.cpu_regs.panic = 0;
	machine.cpu_regs.cr = COND_UNDEF;

	machine.cpu_regs.pc = MEM_START_ROM + PRG_HEADER_SIZE;
}


static void cpu_load_program(uint32_t *prg, uint16_t addr) {
	int prg_size = prg[PRG_SIZE_OFFSET];

	if (prg_size > (RAM_SIZE - MEM_START_PRG))
		machine.cpu_regs.exception = EXC_PRG;
	else
		memcpy(&machine.RAM[addr], prg, prg_size);
}


static void cpu_exception(long pc) {
	printf("!! %s: ",__func__);

	switch(machine.cpu_regs.exception) {
	case EXC_INSTR:
			printf("illegal instruction 0x%X ",
				machine.RAM[pc]);
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
	default:
			printf("unknown exception %d",
				machine.cpu_regs.exception);
			break;
	}

	printf("[pc: %ld]\n", machine.cpu_regs.pc);

	machine.cpu_regs.panic = 1;
}


__inline__ static long cpu_fetch_instruction(void){
	machine.cpu_regs.exception = 0;

	/* each instruction is 4 bytes */
	machine.cpu_regs.pc += 4;
	if (machine.cpu_regs.pc >= (RAM_SIZE - 3))
		machine.cpu_regs.exception = EXC_PRG;

	return machine.cpu_regs.pc;
}


int main(int argc,char *argv[])
{
	long instr_p;
	int *run_test_prg = &args.regression;
	int *debug = &args.debug;
	struct argp argp = {opts, parse_opt, args_doc, doc};

	signal(SIGINT, sig_handler);

	args.debug = args.regression = 0;

	argp_parse(&argp,argc,argv,0,0,&args);

	cpu_reset();
	display_init(&machine.display, machine.RAM);

	cpu_load_program(rom, MEM_START_ROM);

	if (*run_test_prg)
		cpu_load_program(program_regression_test, MEM_START_PRG);

	while(!machine.cpu_regs.panic) {
		instr_p = cpu_fetch_instruction();
		cpu_decode_instruction((uint32_t *)&machine.RAM[instr_p]);

		if (machine.cpu_regs.exception)
			cpu_exception(instr_p);

		if (machine.display.refresh)
			display_retrace(&machine.display, machine.RAM);
	}

	if (*debug) {
		dump_ram(machine.RAM, MEM_START_DISPLAY,MEM_START_DISPLAY + 64);
		dump_ram(machine.RAM, 8192,8192 + 64);
		dump_regs(machine.cpu_regs.GP_REG);
	}

	if (*run_test_prg) {
		test_result(machine.cpu_regs.GP_REG, machine.RAM);
		printf("\n%s: all tests OK.\n",__func__);
	}

	return 0;
}
