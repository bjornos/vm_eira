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

#include "prg.h"
#include "exception.h"

#define PRG_DEBUG(x) x

void program_load(struct _machine *machine, const char filename[], uint16_t addr) {
	FILE *prog;
	struct _prg_format program;


	prog = fopen(filename,"rb");
	if (prog == NULL) {
		PRG_DEBUG(printf("cannot open program %s\n", filename));
		return;
	}
	PRG_DEBUG(printf("loading %s\n", filename));

	fread(&program.header,sizeof(struct _prg_header), 1, prog);

	if (program.header.magic == PRG_MAGIC_HEADER) {
		program.code_segment = malloc(program.header.code_size * sizeof(uint8_t));

		if ((program.header.code_size > (RAM_SIZE - MEM_START_PRG)) ||
			!program.code_segment) {
			machine->cpu_regs.exception = EXC_PRG;
		} else {
			fread(program.code_segment,program.header.code_size, 1, prog);
	
			*machine->mach_regs.prg_loading = PRG_LOADING;
	
			memcpy(&machine->RAM[addr], program.code_segment, program.header.code_size);

			*machine->mach_regs.prg_loading = PRG_LOADING_DONE;
	
			free(program.code_segment);
		}
	}

	fclose(prog);
}

void program_load_direct(struct _machine *machine, const uint32_t *prg, uint16_t addr, int prg_size) {
	*machine->mach_regs.prg_loading = PRG_LOADING;

	memcpy(&machine->RAM[addr], prg + 4, prg_size);

	*machine->mach_regs.prg_loading = PRG_LOADING_DONE;
}

void program_load_cleanup(void)
{
	int fd;

	fd = open(DEV_PRG_LOAD, O_WRONLY);
	if (fd == -1)
		return;
	else
		write(fd, NULL, 1);

	close(fd);
}

void *program_loader(void *mach)
{
	struct _machine *machine = mach;
	char prg_name[PRG_NAME_MAX] = { 0 };

	while(!machine->cpu_regs.panic) {
		while(machine->cpu_regs.reset);

		int fd = open(DEV_PRG_LOAD, O_RDONLY);

		if (fd < 0) {
			perror("unable to setup program loader");
			pthread_exit(NULL);
		}
		read(fd, prg_name, sizeof(prg_name));

		close(fd);

		/* remove traling newline */
		char *save_ptr;
		strtok_r(prg_name, "\n", &save_ptr);

		if (!machine->cpu_regs.panic)
			program_load(machine, prg_name, MEM_START_PRG);
	}

	pthread_exit(NULL);
}
