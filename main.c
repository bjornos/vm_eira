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

#include "opcodes.h"
#include "registers.h"
#include "exception.h"
#include "testprogram.h"
#include "prg.h"
#include "rom.h"
#include "utils.h"
#include "machine.h"

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

struct _machine machine;

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
	if (signo == SIGPIPE) {
		machine.cpu_regs.exception = EXC_IOPORT;
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


static void mem_setup(void)
{
	memset(&machine.RAM, 0x00, RAM_SIZE);

	machine.mach_regs.prg_loading = (uint8_t *)&machine.RAM[MEM_PRG_LOADING];
	machine.mach_regs.boot_code = (uint8_t *)&machine.RAM[MEM_BOOT_STATUS];

	*machine.mach_regs.prg_loading = PRG_LOADING_DONE;
	*machine.mach_regs.boot_code = BOOT_OK;
}


static void machine_reset(void) {
	mem_setup();

	cpu_reset(&machine.cpu_regs, MACHINE_RESET_VECTOR);

	gpu_reset(&machine.gpu, machine.RAM);

	display_reset(&machine.display);

	if (!ioport_reset(&machine))
		*machine.mach_regs.boot_code |= BOOT_ERR_IO;

	memcpy(&machine.RAM[MEM_START_PRG], program_reset, sizeof(program_reset));

	if (mkfifo(PRG_lOAD_FIFO, S_IRUSR| S_IWUSR) < 0) {
		perror("failed to create program loader");
		*machine.mach_regs.boot_code |= BOOT_ERR_PRG;
	}

}



int main(int argc,char *argv[])
{
	struct argp argp = {opts, parse_opt, args_doc, doc};
	pthread_t display, cpu, gpu, io_in, io_out, prg;
	void *status;

	signal(SIGINT, sig_handler);
	signal(SIGPIPE, sig_handler);

	args.debug = args.machine_check = args.dump_ram = 0;
	args.load_program = NULL;
	args.dump_size = DUMP_RAM_SIZE_DEFAULT;

	argp_parse(&argp,argc,argv,0,0,&args);

machine_soft_reset:

	machine_reset();

	pthread_create(&cpu, NULL, cpu_machine, &machine);
	pthread_create(&gpu, NULL, gpu_machine, &machine);
	pthread_create(&display, NULL, display_machine, &machine);

	if (!(*machine.mach_regs.boot_code & BOOT_ERR_PRG))
		pthread_create(&prg, NULL, program_loader, &machine);

	if (!(*machine.mach_regs.boot_code & BOOT_ERR_IO)) {
		pthread_create(&io_in, NULL, ioport_machine_input, &machine);
		pthread_create(&io_out, NULL, ioport_machine_output, &machine);
	}

	program_load_direct(&machine, rom, MEM_START_ROM, sizeof(rom));

	if (args.machine_check) {
		program_load_direct(&machine, program_regression_test,
			MEM_START_PRG, sizeof(program_regression_test));
	}
	else if (args.load_program) {
		program_load(&machine, args.load_program, MEM_START_PRG);
	}

	if (args.debug)
		machine.cpu_regs.dbg = 1;

	/* release CPU & GPU */
	machine.gpu.reset = 0;
	machine.cpu_regs.reset = 0;

	pthread_join(cpu, &status);
	pthread_join(gpu, &status);
	pthread_join(display, &status);

	if (!(*machine.mach_regs.boot_code & BOOT_ERR_IO)) {
		machine.ioport->active = 0;
		ioport_shutdown((int)machine.ioport->input);
		pthread_join(io_in, &status);
		pthread_join(io_out, &status);
		unlink(IO_INPUT_PORT);
		unlink(IO_OUTPUT_PORT);
	}

	if (!(*machine.mach_regs.boot_code & BOOT_ERR_PRG)) {
		program_load_cleanup();
		pthread_join(prg, &status);
		unlink(PRG_lOAD_FIFO);
	}

	/* soft reboot @ cpu exception */
	if ((machine.cpu_regs.exception != EXC_SHUTDOWN) &&
		(machine.cpu_regs.exception != EXC_NONE)) {
		args.load_program = NULL;
		args.debug = 0;
		goto machine_soft_reset;
	}

	if (args.machine_check) {
		machine.ioport->input = IO_IN_TST_VAL;
		machine.ioport->output = IO_OUT_TST_VAL;

		test_result(machine.cpu_regs.GP_REG, machine.RAM);

		printf("\n%s: all tests OK.\n",__func__);
	}

	if (args.dump_ram) {
		dump_ram(machine.RAM, args.dump_ram, args.dump_ram + args.dump_size);
		dump_io(machine.ioport->input, machine.ioport->output);
	}

	pthread_exit(NULL);

	return 0;
}
