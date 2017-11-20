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

typedef struct {
	int debug;
	int machine_check;
	char *load_program;
	int dump_ram;
	int dump_size;
} args_t;

static const uint8_t rom_txt_segment_boot_head[15] =
	{'e', 'i', 'r', 'a', '-', '1', 0x00, 0x00,
	 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

static const uint8_t rom_txt_segment_boot_anim[15] =
	{'|','/','-','\\','|','/', '-', '\\',
	 '*', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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

static __inline__ int machine_error_is_set(machine_boot_err err_code)
{
	if (*machine.mach_regs.boot_code & err_code)
		return 1;

	return 0;
}
static __inline__ void machine_error_set(machine_boot_err err_code)
{
	*machine.mach_regs.boot_code |= err_code;
}
static __inline__ int machine_error_is_clear(machine_boot_err err_code)
{
	return !machine_error_is_set(err_code);
}

static __inline__ void mem_setup(void)
{
	memset(&machine.RAM, 0x00, RAM_SIZE);

	machine.display.frame_buffer = machine.RAM + MEM_START_GPU_FB;

	machine.mach_regs.prg_loading = (uint8_t *)&machine.RAM[MEM_PRG_LOADING];
	machine.mach_regs.boot_code = (uint8_t *)&machine.RAM[MEM_BOOT_STATUS];

	machine.mach_regs.boot_msg = (uint8_t *)&machine.RAM[MEM_ROM_BOOT_MSG];
	memcpy(machine.mach_regs.boot_msg, rom_txt_segment_boot_head,
		sizeof(rom_txt_segment_boot_head));

	machine.mach_regs.boot_anim = (uint8_t *)&machine.RAM[MEM_ROM_BOOT_ANIM];
	memcpy(machine.mach_regs.boot_anim, rom_txt_segment_boot_anim,
		sizeof(rom_txt_segment_boot_anim));

	*machine.mach_regs.prg_loading = PRG_LOADING_DONE;
	*machine.mach_regs.boot_code = BOOT_OK;
}


static void machine_reset(void) {
	mem_setup();

	cpu_reset(&machine);

	display_reset(&machine);

	if (!ioport_reset(&machine))
		machine_error_set(BOOT_ERR_IO);

	memcpy(&machine.RAM[MEM_START_PRG], program_reset, sizeof(program_reset));

	if (mkfifo(PRG_LOAD_FIFO, S_IRUSR| S_IWUSR) < 0) {
		perror("failed to create program loader");
		machine_error_set(BOOT_ERR_PRG);
	}

}



int main(int argc,char *argv[])
{
	struct argp argp = {opts, parse_opt, args_doc, doc};
	pthread_t display, cpu, io_in, io_out, prg;
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
	pthread_create(&display, NULL, display_machine, &machine);

	if (machine_error_is_clear(BOOT_ERR_PRG))
		pthread_create(&prg, NULL, program_loader, &machine);

	if (machine_error_is_clear(BOOT_ERR_IO)) {
		pthread_create(&io_in, NULL, ioport_machine_input, &machine);
		pthread_create(&io_out, NULL, ioport_machine_output, &machine);
	}

	program_load_direct(&machine, rom, MEM_START_ROM, sizeof(rom));


	if (args.load_program) {
		program_load(&machine, args.load_program, MEM_START_PRG);
	}
	/* checktest override load program */
	if (args.machine_check) {
		program_load_direct(&machine, program_regression_test,
			MEM_START_PRG, sizeof(program_regression_test));
	}

	machine.cpu_regs.dbg = args.debug ? 1 : 0;

	/* release CPU */
	machine.cpu_regs.reset = 0;

	pthread_join(cpu, &status);
	pthread_join(display, &status);

	if (machine_error_is_clear(BOOT_ERR_IO)) {
		machine.ioport->active = 0;
		ioport_shutdown((int)machine.ioport->input);
		pthread_join(io_in, &status);
		pthread_join(io_out, &status);
	}

	if (machine_error_is_clear(BOOT_ERR_PRG)) {
		program_load_cleanup();
		pthread_join(prg, &status);
	}

	/* soft reboot @ cpu exception */
	if ((machine.cpu_regs.exception != EXC_SHUTDOWN) && (machine.cpu_regs.exception != EXC_NONE)) {
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
