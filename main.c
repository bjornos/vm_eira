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
#include <stdlib.h>
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
	int machine_check;
	char *load_program;
} args_t;

struct _cpu_regs {
	uint16_t GP_REG[GP_REG_MAX];	/* general purpose registers */
	long pc;			/* program counter */
	int sp;				/* stack pointer */
	int cr;				/* conditional register */
	char exception;
	unsigned char panic;		/* halt cpu */
};

static struct _machine {
	uint8_t RAM[RAM_SIZE];
	struct _cpu_regs cpu_regs;
	struct _display_adapter display;
	struct _dbg dbg_info[DBG_HISTORY];
} machine;

static struct argp_option opts[] = {
	{"debug", 'd', 0, OPTION_ARG_OPTIONAL, "Enable debug"},
	{"check", 'c', 0, OPTION_ARG_OPTIONAL, "Run machine check"},
	{"program", 'p', "FILE", OPTION_ARG_OPTIONAL, "Load program"},
	{0}
};


static char* doc = "";
static char* args_doc = "";
static args_t args;
static int dbg_index = 0;

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
		case 'c':
			args->machine_check = 1;
			break;
		case 'p':
			args->load_program = arg;
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static __inline__ void debug_opcode(const char op[])
{
	memset(machine.dbg_info[dbg_index].opcode, '\0', 16);
	strncpy(machine.dbg_info[dbg_index].opcode, op, strlen(op));
}
static __inline__ void debug_instr(uint32_t *instr)
{
	machine.dbg_info[dbg_index].instr = *instr;
}
static __inline__ void debug_result(long *res)
{
	machine.dbg_info[dbg_index].op_result = *res;
}
static __inline__ void debug_args(uint16_t *arg1,uint16_t *arg2)
{
	if (arg1)
		machine.dbg_info[dbg_index].op_arg1 = *arg1;
	if (arg2)
		machine.dbg_info[dbg_index].op_arg2 = *arg2;
}


uint16_t decode_mnemonic(uint32_t *instr, uint16_t **dst, int opsize)
{
	uint16_t local_src;
	uint16_t local_dst;
	uint16_t src;

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
		debug_args(&local_dst, &src);

		return src;
}


static __inline__ void compare(uint16_t c1, uint16_t c2)
{
	int d = c2 - c1;

	DBG(printf("%s --- ",__func__));

	debug_args((uint16_t*)&c1, (uint16_t*)&c1);

	machine.cpu_regs.cr =
			(d == 0) ? (COND_EQ | COND_ZERO) :
			(d > 0) ? (COND_GR | COND_NEQ) :
			(d < 0) ? (COND_LE | COND_NEQ) :
			COND_UNDEF;

	debug_result((long*)&machine.cpu_regs.cr);
	DBG(printf("c1:%d c2:%d d:%d desc: %d\n",c1,c2,d,machine.cpu_regs.cr));
}


static __inline__ void branch(enum conditions cond, uint16_t addr)
{
	DBG(printf("%s ?: ",__func__));

	debug_args((uint16_t*)&machine.cpu_regs.cr, (uint16_t*)&cond);


	if (machine.cpu_regs.cr & cond) {
		machine.cpu_regs.pc = addr - sizeof(uint32_t);
	}
	debug_result(&machine.cpu_regs.pc);
	machine.cpu_regs.cr = COND_UNDEF;
}

static void cpu_decode_instruction(uint32_t *instr)
{
	uint8_t opcode;
	uint16_t src;
	uint16_t *dst;
	uint16_t addr;

	if (machine.cpu_regs.exception)
		return;

	DBG(printf("%s 0x%x\n",__func__, *instr));

	debug_instr(instr);

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop:
			debug_opcode("nop");
			break;
		case halt:
			debug_opcode("halt");
			machine.cpu_regs.panic = 1;
			break;
		case mov:
			debug_opcode("mov");
			src = decode_mnemonic(instr, &dst, SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case movi:
			debug_opcode("movi");
			src = decode_mnemonic(instr, &dst, SIZE_INT);
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case add:
			debug_opcode("add");
			src = decode_mnemonic(instr, &dst,SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst += src;
			break;
		case sub:
			debug_opcode("sub");
			src = decode_mnemonic(instr, &dst,SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst -= src;
			break;
		case jmp:
			debug_opcode("jmp");
			machine.cpu_regs.pc = (*instr >> 8) - sizeof(uint32_t); /* compensate for pc++ */
			debug_result(&machine.cpu_regs.pc);
			break;
		case cmp:
			debug_opcode("cmp");
			machine.cpu_regs.cr &= COND_UNDEF;
			src = decode_mnemonic(instr, &dst, SIZE_INT);
			compare(src, *dst);
			debug_args(&src, dst);
			debug_result((long *)&machine.cpu_regs.cr);
			break;
		case breq:
			debug_opcode("breq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(COND_EQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					machine.cpu_regs.exception = EXC_INSTR;
				else
					branch(COND_EQ, machine.cpu_regs.GP_REG[addr]);
			}
			break;
		case brneq:
			debug_opcode("brneq");
			addr = (*instr >> 16);
			if (addr > MEM_START_ROM)
				branch(COND_NEQ, (*instr >> 16));
			else {
				if (addr > GP_REG_MAX)
					machine.cpu_regs.exception = EXC_INSTR;
				else
					branch(COND_NEQ, machine.cpu_regs.GP_REG[addr]);
			}
			break;
		case stopc:
			debug_opcode("stopc");
			addr = (*instr >> 8);
			debug_args(&addr, NULL);
			debug_result(&machine.cpu_regs.pc);
			machine.cpu_regs.GP_REG[(*instr >> 8)] = machine.cpu_regs.pc;
			break;
		case clrscr:
			debug_opcode("clrscr");
			display_request(&machine.display, instr, display_clr, machine.RAM);
			break;
		case setposxy:
			debug_opcode("setposxy");
			display_request(&machine.display, instr, display_setxy, machine.RAM);
			break;
		case putchar:
			debug_opcode("putchar");
			display_request(&machine.display, instr, display_setc, machine.RAM);
			break;
		default: machine.cpu_regs.exception = EXC_INSTR;
			break;
	}
}


static void cpu_reset(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);
	memset(&machine.cpu_regs.GP_REG, 0x00, GP_REG_MAX);

	for (int d=0; d < DBG_HISTORY; d++)
		memset(&machine.dbg_info[d], 0x00, sizeof(struct _dbg));
	dbg_index = 0;

	machine.cpu_regs.pc = 0;
	machine.cpu_regs.sp = 0;
	machine.cpu_regs.exception = 0;
	machine.cpu_regs.panic = 0;
	machine.cpu_regs.cr = COND_UNDEF;

	machine.cpu_regs.pc = MEM_START_ROM - sizeof(uint32_t);

	memcpy(&machine.RAM[MEM_START_PRG], program_reset, sizeof(program_reset));
}


static void cpu_load_program(const char filename[], uint16_t addr) {
	FILE *prog;
	struct _prg_format program;

	prog = fopen(filename,"rb");
	fread(&program.header,sizeof(struct _prg_header),1,prog);

	if (program.header.magic == PRG_MAGIC_HEADER) {
		program.code_segment = malloc(program.header.code_size * sizeof(uint8_t));

		if ((program.header.code_size > (RAM_SIZE - MEM_START_PRG)) ||
			!program.code_segment) {
			machine.cpu_regs.exception = EXC_PRG;
		} else {
			fread(program.code_segment,program.header.code_size,1,prog);
			memcpy(&machine.RAM[addr], program.code_segment, program.header.code_size);
			free(program.code_segment);
		}
	}

	fclose(prog);
}

static __inline__ void cpu_load_program_local(uint32_t *prg, uint16_t addr) {
		memcpy(&machine.RAM[addr], prg + 4, 1024);
}

static void cpu_exception(long pc) {
	printf("!! %s: ",__func__);

	switch(machine.cpu_regs.exception) {
	case EXC_INSTR:
			printf("illegal instruction 0x%X\n",
				machine.RAM[pc]);
			break;
	case EXC_MEM:
			printf("cannot access memory\n");
			break;
	case EXC_REG:
			printf("cannot access register\n");
			break;
	case EXC_PRG:
			printf("stray program\n");
			break;
	default:
			printf("unknown exception %d\n",
				machine.cpu_regs.exception);
			break;
	}

	printf("[pc: %ld]\n", machine.cpu_regs.pc);

	machine.cpu_regs.panic = 1;
}


__inline__ static long cpu_fetch_instruction(void){
	machine.cpu_regs.exception = 0;

	/* each instruction is 4 bytes */
	machine.cpu_regs.pc += sizeof(uint32_t);
	if (machine.cpu_regs.pc >= (RAM_SIZE - 3))
		machine.cpu_regs.exception = EXC_PRG;

	dbg_index = (dbg_index + 1) % DBG_HISTORY;
	memset(&machine.dbg_info[dbg_index], 0x00, sizeof(struct _dbg));

	return machine.cpu_regs.pc;
}


int main(int argc,char *argv[])
{
	long instr_p;
	struct argp argp = {opts, parse_opt, args_doc, doc};

	signal(SIGINT, sig_handler);

	args.debug = args.machine_check = 0;
	args.load_program = NULL;

	argp_parse(&argp,argc,argv,0,0,&args);

	cpu_reset();
	display_init(&machine.display, machine.RAM);

	cpu_load_program("bin/eira_rom.bin", MEM_START_ROM);

	if (args.machine_check)
		cpu_load_program_local(program_regression_test, MEM_START_PRG);

	if (args.load_program)
		cpu_load_program(args.load_program, MEM_START_PRG);

	while(!machine.cpu_regs.panic) {
		instr_p = cpu_fetch_instruction();
		cpu_decode_instruction((uint32_t *)&machine.RAM[instr_p]);

		if (machine.cpu_regs.exception)
			cpu_exception(instr_p);

		if (machine.display.refresh)
			display_retrace(&machine.display, machine.RAM);

		if (args.debug) {
			gotoxy(1,5);
			dump_instr(machine.dbg_info, dbg_index);
			gotoxy(1,5 + DBG_HISTORY + 4);
			dump_regs(machine.cpu_regs.GP_REG);
		}
	}

	if (args.machine_check) {
		test_result(machine.cpu_regs.GP_REG, machine.RAM);
		printf("\n%s: all tests OK.\n",__func__);
	}

	return 0;
}
