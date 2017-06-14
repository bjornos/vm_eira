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
#include "gpu.h"
#include "display.h"
#include "memory.h"
#include "exception.h"
#include "testprogram.h"
#include "prg.h"
#include "rom.h"
#include "utils.h"

#define MACHINE_RESET_VECTOR	(MEM_START_ROM - sizeof(uint32_t))

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
	struct _gpu gpu;
	struct _display_adapter display;
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



static void machine_reset(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);

	cpu_reset(&machine.cpu_regs, MACHINE_RESET_VECTOR);

	gpu_reset(&machine.gpu, machine.RAM);

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

__inline__ static void machine_load_program_local(const uint32_t *prg, uint16_t addr) {
	memcpy(&machine.RAM[addr], prg + 4, 1024);
}


void *machine_display(void *arg)
{
	struct timespec frame_rate;
	int fps = DISPLAY_FRAME_RATE;

	frame_rate.tv_nsec = 1000000000 / fps;
	frame_rate.tv_sec = 0;

	while(!machine.cpu_regs.panic) {
		if (machine.display.enabled)
			display_retrace(&machine.display, machine.gpu.frame_buffer);
		nanosleep(&frame_rate, NULL);
	}

	pthread_exit(NULL);
}


void *machine_gpu(void *arg)
{
	struct timespec gpu_clk_freq;
	int hz = 10; /* 10 Hz */

	gpu_clk_freq.tv_nsec = 1000000000 / hz;
	gpu_clk_freq.tv_sec = 0;

	while(!machine.cpu_regs.panic) {

		while(machine.gpu.reset);

		gpu_fetch_instr(&machine.gpu);

		gpu_decode_instr(&machine.gpu, &machine.display);

		machine.cpu_regs.exception = machine.gpu.exception;

		nanosleep(&gpu_clk_freq, NULL);
	}

	//for (int i=0; i<16;i++);
	//	printf("gpu instr list %d: 0x%x\n",i,machine.gpu.instr_list[i]);

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

		instr_p = cpu_fetch_instruction(&machine.cpu_regs);
		cpu_decode_instruction(&machine.cpu_regs, machine.RAM, &machine.display);

		if (machine.cpu_regs.gpu_request)  {
			gpu_add_instr(&machine.gpu, (uint32_t *)&machine.RAM[instr_p]);
		}

		if (machine.cpu_regs.exception) {
			display_wait_retrace(&machine.display);
			cpu_handle_exception(&machine.cpu_regs, (uint32_t *)&machine.RAM[instr_p]);
		}

		nanosleep(&cpu_clk_freq, NULL);
	}

	pthread_exit(NULL);
}

int main(int argc,char *argv[])
{
	struct argp argp = {opts, parse_opt, args_doc, doc};
	pthread_t display, cpu, gpu;
	void *status;

	signal(SIGINT, sig_handler);

	args.debug = args.machine_check = args.dump_ram = 0;
	args.load_program = NULL;
	args.dump_size = DUMP_RAM_SIZE_DEFAULT;

	argp_parse(&argp,argc,argv,0,0,&args);

	machine_reset();

	pthread_create(&cpu, NULL, machine_cpu, NULL);
	pthread_create(&gpu, NULL, machine_gpu, NULL);
	pthread_create(&display, NULL, machine_display, NULL);

	machine_load_program_local(rom, MEM_START_ROM);

	if (args.machine_check)
		machine_load_program_local(program_regression_test, MEM_START_PRG);
	else if (args.load_program)
		machine_load_program(args.load_program, MEM_START_PRG);

	if (args.debug)
		machine.cpu_regs.dbg = 1;

	/* release CPU & GPU */
	machine.gpu.reset = 0;
	machine.cpu_regs.reset = 0;

	pthread_join(cpu, &status);
	pthread_join(gpu, &status);
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
