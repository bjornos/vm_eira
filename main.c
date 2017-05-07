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
#include <time.h>
#include <pthread.h>

#include "opcodes.h"
#include "cpu.h"
#include "display.h"
#include "memory.h"
#include "exception.h"
#include "testprogram.h"
#include "prg.h"
#include "rom.h"
#include "utils.h"

#define DBG(x)
#define MACHINE_RESET_VECTOR	(MEM_START_ROM - sizeof(uint32_t))

enum op_size {
	SIZE_BYTE,
	SIZE_INT,
};

typedef enum {
	RESET_HARD,
	RESET_SOFT
} reset_t;


typedef struct {
	int debug;
	int machine_check;
	char *load_program;
	int dump_ram;
	int dump_size;
} args_t;


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
	{"dump-ram", 'r', "RAM ADDRESS", OPTION_ARG_OPTIONAL,
		"Dump RAM at machine shutdown"},
	{"dump-size", 's', "RAM DUMP SIZE", OPTION_ARG_OPTIONAL,
		"Number of RAM Bytes to dump (default 32)"},
	{0}
};


static char* doc = "";
static char* args_doc = "";
static args_t args;
static int dbg_index = 0;

void sig_handler(int signo)
{
	if (signo == SIGINT) {
		machine.cpu_regs.exception = EXC_SHUTDOWN;
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
		case 'r':
			args->dump_ram = atoi(arg);
			break;
		case 's':
			args->dump_size = atoi(arg);
			break;
		default:
			return ARGP_ERR_UNKNOWN;
	}

	return 0;
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
		debug_args(machine.dbg_info, dbg_index, &local_dst, &src);

		return src;
}

static __inline__ void compare(uint16_t c1, uint16_t c2)
{
	int d = c2 - c1;

	DBG(printf("%s --- ",__func__));

	debug_args(machine.dbg_info, dbg_index, (uint16_t*)&c1, (uint16_t*)&c1);

	machine.cpu_regs.cr =
			(d == 0) ? (COND_EQ | COND_ZERO) :
			(d > 0) ? (COND_GR | COND_NEQ) :
			(d < 0) ? (COND_LE | COND_NEQ) :
			COND_UNDEF;

	debug_result(machine.dbg_info, dbg_index, (long*)&machine.cpu_regs.cr);
	DBG(printf("c1:%d c2:%d d:%d desc: %d\n",c1,c2,d,machine.cpu_regs.cr));
}


static __inline__ void branch(enum conditions cond, uint16_t addr)
{
	DBG(printf("%s ?: ",__func__));

	debug_args(machine.dbg_info, dbg_index, (uint16_t*)&machine.cpu_regs.cr, (uint16_t*)&cond);


	if (machine.cpu_regs.cr & cond) {
		machine.cpu_regs.pc = addr - sizeof(uint32_t);
	}
	debug_result(machine.dbg_info, dbg_index, &machine.cpu_regs.pc);
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

	debug_instr(machine.dbg_info, dbg_index, instr);

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop:
			debug_opcode(machine.dbg_info, dbg_index, "nop");
			break;
		case halt:
			debug_opcode(machine.dbg_info, dbg_index, "halt");
			machine.cpu_regs.panic = 1;
			break;
		case mov:
			debug_opcode(machine.dbg_info, dbg_index, "mov");
			src = decode_mnemonic(instr, &dst, SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case movi:
			debug_opcode(machine.dbg_info, dbg_index, "movi");
			src = decode_mnemonic(instr, &dst, SIZE_INT);
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case add:
			debug_opcode(machine.dbg_info, dbg_index, "add");
			src = decode_mnemonic(instr, &dst,SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst += src;
			break;
		case sub:
			debug_opcode(machine.dbg_info, dbg_index, "sub");
			src = decode_mnemonic(instr, &dst,SIZE_BYTE);
			if (!machine.cpu_regs.exception)
				*dst -= src;
			break;
		case jmp:
			debug_opcode(machine.dbg_info, dbg_index, "jmp");
			machine.cpu_regs.pc = (*instr >> 8) - sizeof(uint32_t); /* compensate for pc++ */
			debug_result(machine.dbg_info, dbg_index, &machine.cpu_regs.pc);
			break;
		case cmp:
			debug_opcode(machine.dbg_info, dbg_index, "cmp");
			machine.cpu_regs.cr &= COND_UNDEF;
			src = decode_mnemonic(instr, &dst, SIZE_INT);
			compare(src, *dst);
			debug_args(machine.dbg_info, dbg_index, &src, dst);
			debug_result(machine.dbg_info, dbg_index, (long *)&machine.cpu_regs.cr);
			break;
		case breq:
			debug_opcode(machine.dbg_info, dbg_index, "breq");
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
			debug_opcode(machine.dbg_info, dbg_index, "brneq");
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
			debug_opcode(machine.dbg_info, dbg_index, "stopc");
			addr = (*instr >> 8);
			debug_args(machine.dbg_info, dbg_index, &addr, NULL);
			debug_result(machine.dbg_info, dbg_index, &machine.cpu_regs.pc);
			machine.cpu_regs.GP_REG[(*instr >> 8)] = machine.cpu_regs.pc;
			break;
		case dimd:
			debug_opcode(machine.dbg_info, dbg_index, "dimd");
			display_wait_retrace(&machine.display);
			machine.cpu_regs.exception =
				display_init(&machine.display, machine.RAM, (*instr >> 8));
			break;
		case diclr:
			debug_opcode(machine.dbg_info, dbg_index, "diclr");
			machine.cpu_regs.exception =
				display_request(&machine.display, instr, display_clr);
			break;
		case diwtrt:
			debug_opcode(machine.dbg_info, dbg_index, "diwtrt");
			display_wait_retrace(&machine.display);
			break;
		case setposxy:
			debug_opcode(machine.dbg_info, dbg_index, "setposxy");
			display_request(&machine.display, instr, display_setxy);
			break;
		case putchar:
			debug_opcode(machine.dbg_info, dbg_index, "putchar");
			display_wait_retrace(&machine.display);
			display_request(&machine.display, instr, display_setc);
			break;
		default: machine.cpu_regs.exception = EXC_INSTR;
			break;
	}
}


static void machine_reset(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);

	for (int d=0; d < DBG_HISTORY; d++)
		memset(&machine.dbg_info[d], 0x00, sizeof(struct _dbg));
	dbg_index = 0;

	cpu_reset(&machine.cpu_regs, MACHINE_RESET_VECTOR);

	display_reset(&machine.display);

	memcpy(&machine.RAM[MEM_START_PRG], program_reset, sizeof(program_reset));
}


static void machine_load_program(const char filename[], uint16_t addr) {
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

static __inline__ void machine_load_program_local(const uint32_t *prg, uint16_t addr) {
		memcpy(&machine.RAM[addr], prg + 4, 1024);
}


__inline__ static long cpu_fetch_instruction(void){
	/* each instruction is 4 bytes */
	machine.cpu_regs.pc += sizeof(uint32_t);
	if (machine.cpu_regs.pc >= (RAM_SIZE - 3))
		machine.cpu_regs.exception = EXC_PRG;

	dbg_index = (dbg_index + 1) % DBG_HISTORY;
	memset(&machine.dbg_info[dbg_index], 0x00, sizeof(struct _dbg));

	return machine.cpu_regs.pc;
}

void *machine_display(void *arg)
{
	struct timespec frame_rate;
	int fps = DISPLAY_FRAME_RATE;

	frame_rate.tv_nsec = 1000000000 / fps;
	frame_rate.tv_sec = 0;

	while(!machine.cpu_regs.panic) {
		if (machine.display.enabled)
			display_retrace(&machine.display);
		nanosleep(&frame_rate, NULL);
	}

	pthread_exit(NULL);
}

void *machine_cpu(void *arg)
{
	struct timespec cpu_clk_freq;
	long instr_p;
	int hz = 10; /* 10 Hz */

	cpu_clk_freq.tv_nsec = 1000000000 / hz;
	cpu_clk_freq.tv_sec = 0;

	while(!machine.cpu_regs.panic) {
		while(machine.cpu_regs.reset);

		instr_p = cpu_fetch_instruction();
		cpu_decode_instruction((uint32_t *)&machine.RAM[instr_p]);

		if (machine.cpu_regs.exception) {
			display_wait_retrace(&machine.display);
			cpu_handle_exception(&machine.cpu_regs, (uint32_t *)&machine.RAM[instr_p]);
		}

		if (machine.cpu_regs.dbg) {
			display_wait_retrace(&machine.display);
			machine.display.enabled = 0;
			gotoxy(1,15);
			dump_instr(machine.dbg_info, dbg_index);
			gotoxy(1,15 + DBG_HISTORY + 4);
			dump_regs(machine.cpu_regs.GP_REG);
			machine.display.enabled = 1;
		}

		nanosleep(&cpu_clk_freq, NULL);
	}

	pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
	struct argp argp = {opts, parse_opt, args_doc, doc};
	pthread_t display, cpu;
	void *status;

	signal(SIGINT, sig_handler);

	args.debug = args.machine_check = args.dump_ram = 0;
	args.load_program = NULL;
	args.dump_size = DUMP_RAM_SIZE_DEFAULT;

	argp_parse(&argp,argc,argv,0,0,&args);

	machine_reset();

	pthread_create(&cpu, NULL, machine_cpu, NULL);
	pthread_create(&display, NULL, machine_display, NULL);

	machine_load_program_local(rom, MEM_START_ROM);

	if (args.machine_check)
		machine_load_program_local(program_regression_test, MEM_START_PRG);
	else if (args.load_program)
		machine_load_program(args.load_program, MEM_START_PRG);

	if (args.debug)
		machine.cpu_regs.dbg = 1;

	/* release CPU */
	machine.cpu_regs.reset = 0;

	pthread_join(cpu, &status);
	pthread_join(display, &status);

	if (args.dump_ram)
		dump_ram(machine.RAM, args.dump_ram, args.dump_ram + args.dump_size);

	if (args.machine_check) {
		test_result(machine.cpu_regs.GP_REG, machine.RAM);
		printf("\n%s: all tests OK.\n",__func__);
	}

	pthread_exit(NULL);

	return 0;
}
