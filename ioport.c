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

#include "ioport.h"
#include "exception.h"
#include "machine.h"
#include "utils.h"

void ioport_reset(void *mach)
{
	struct _machine *machine = mach;

	machine->ioport = (struct _io_regs *)&machine->RAM[MEM_START_IOPORT];
	memset(machine->ioport, 0x00, sizeof(struct _io_regs));
}

void ioport_shutdown(int input_state)
{
	char *in_port;
	int fd;
	int t;

	fd = open(DEV_IO_OUTPUT, O_RDONLY);
	t = read(fd, NULL, 4); /* t silence compiler warning */
	close(fd);

	in_port = int_to_str(input_state);

	fd = open(DEV_IO_INPUT, O_WRONLY);
	t = write(fd, in_port, strlen(in_port));
	close(fd);

	free(in_port);
}

void *ioport_machine_output(void *mach)
{
	struct _machine *machine = mach;
	struct timespec io_clk_freq;
	int hz = 100; /* 100 Hz */

	io_clk_freq.tv_nsec = 1000000000 / hz;
	io_clk_freq.tv_sec = 0;

	while(!machine->cpu_regs.panic) {
		while(machine->cpu_regs.reset);

		int fd = open(DEV_IO_OUTPUT, O_WRONLY);
		if (fd < 0) {
			perror("unable to access output port");
			machine->cpu_regs.exception = EXC_IOPORT;
			pthread_exit(NULL);
		}
		char *outval = int_to_str((int)machine->ioport->output);

		if (write(fd, outval, strlen(outval)) == -1) {
			if (errno == EPIPE)
				perror("read side closed pipe");
		}
		close(fd);

		free(outval);

		nanosleep(&io_clk_freq, NULL);
	}

	pthread_exit(NULL);
}

void *ioport_machine_input(void *mach)
{
	struct _machine *machine = mach;
	char inval[4] = { 0 };

	while(!machine->cpu_regs.panic) {
		while(machine->cpu_regs.reset);

		int fd = open(DEV_IO_INPUT, O_RDONLY);
		if (fd < 0) {
			perror("unable to access input port");
			machine->cpu_regs.exception = EXC_IOPORT;
			pthread_exit(NULL);
		}
		int t = read(fd, inval, sizeof(inval)); /* t silence compiler warning */
		close(fd);

		machine->ioport->input = atoi(inval);
	}

	pthread_exit(NULL);
}
