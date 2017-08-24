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

#include <string.h>

#include "gpu.h"
#include "opcodes.h"
#include "exception.h"
#include "memory.h"
#include "machine.h"
#include "utils.h"

#define GPU_DBG(x)

enum {
	GPU_LOCKED,
	GPU_UNLOCKED
};

static struct _dbg dbg_info[DBG_HISTORY];
static int dbg_index;

__inline__ static void gpu_lock(struct _gpu *gpu)
{
	while(gpu->instr_lock == GPU_LOCKED)
		if (gpu->reset)
			return;
	gpu->instr_lock = GPU_LOCKED;
}

__inline__ static void gpu_unlock(struct _gpu *gpu)
{
	gpu->instr_lock = GPU_UNLOCKED;
}

void gpu_fetch_instr(struct _gpu *gpu)
{
	gpu_lock(gpu);

	gpu->instr_ptr = (gpu->instr_ptr + 1) % GPU_INSTR_BUFFER_SIZE;

	gpu_unlock(gpu);
}


void gpu_decode_instr(struct _gpu *gpu, struct _display_adapter *display)
{
	uint32_t instr;
	uint8_t opcode;
	int gpu_exception = EXC_NONE;

	gpu_lock(gpu);

	instr = gpu->instr_list[gpu->instr_ptr];
	gpu->instr_list[gpu->instr_ptr] = diwait;

	gpu_unlock(gpu);

	opcode = instr & 0xff;

	switch(opcode) {
		case diwait:
			break;
		case dimd:
			gpu_exception =
				display_request(display, &instr, gpu->frame_buffer, DISPLAY_INIT);
			//debug_opcode(dbg_info, dbg_index, "dimd");
			break;
 		case diclr:
			//debug_opcode(dbg_info, dbg_index, "diclr");
			gpu_exception =
				display_request(display, &instr, gpu->frame_buffer, DISPLAY_CLR);
			break;
		case diwtrt:
			//debug_opcode(dbg_info, dbg_index, "diwtrt");
			display_wait_retrace(display);
			break;
		case disetxy:
			//debug_opcode(dbg_info, dbg_index, "setposxy");
			display_request(display, &instr, gpu->frame_buffer, DISPLAY_SETXY);
			break;
		case dichar:
			//debug_opcode(dbg_info, dbg_index, "putchar");
			display_wait_retrace(display);
			display_request(display, &instr, gpu->frame_buffer, DISPLAY_SETC);
			break;

		default:
			printf("gpu error unknown. instr: 0x%x ip: %d\n",opcode, gpu->instr_ptr);
			gpu_exception = EXC_GPU;
			break;
	}

	GPU_DBG(gotoxy(1,21));
	GPU_DBG(printf("gpu instr:\t0x%x\t\tip: %d \tlistpos %d  \n",opcode, gpu->instr_ptr,gpu->instr_list_pos));

	gpu->exception = gpu_exception;
}

void gpu_add_instr(struct _gpu *gpu, uint32_t *instr)
{
	gpu_lock(gpu);

	GPU_DBG(gotoxy(1,20));
	GPU_DBG(printf("gpu adding instr:\t0x%x\t ip: %d \t\t listpos %d  \n",*instr, gpu->instr_ptr,gpu->instr_list_pos));

	gpu->instr_list[gpu->instr_list_pos] = *instr;
	gpu->instr_list_pos = (gpu->instr_list_pos + 1) % GPU_INSTR_BUFFER_SIZE;

	gpu_unlock(gpu);
}

void gpu_reset(struct _gpu *gpu, uint8_t *RAM)
{
	gpu->reset = 1;

	gpu_lock(gpu);

	gpu->frame_buffer = RAM + MEM_START_GPU_FB;

	for (int i=0; i<GPU_INSTR_BUFFER_SIZE; i++)
		gpu->instr_list[i] = diwait;

	gpu->instr_ptr = 0;
	gpu->instr_list_pos = 0;

	gpu->exception = EXC_NONE;

	for (int d=0; d < DBG_HISTORY; d++)
		memset(dbg_info + d, 0x00, sizeof(struct _dbg));

	dbg_index = 0;

	gpu_unlock(gpu);
}

void gpu_debug_out(void *mach)
{
	struct _machine *machine = mach;

	printf("GPU Registers\n============\nIP: %d\t list_pos %d\n",
		machine->gpu.instr_ptr, machine->gpu.instr_list_pos);

	for (int i=0; i<GPU_INSTR_BUFFER_SIZE;i++)
		printf("instr list %d: 0x%x\n",
			i,machine->gpu.instr_list[i]);

}

void *gpu_machine(void *mach)
{
	struct _machine *machine = mach;
	struct timespec gpu_clk_freq;
	int hz = 10; /* 10 Hz */

	gpu_clk_freq.tv_nsec = 1000000000 / hz;
	gpu_clk_freq.tv_sec = 0;

	while(!machine->cpu_regs.panic) {
		while(machine->gpu.reset);

		gpu_fetch_instr(&machine->gpu);

		gpu_decode_instr(&machine->gpu, &machine->display);

		if (machine->gpu.exception != EXC_NONE)
			machine->cpu_regs.exception = machine->gpu.exception;

		nanosleep(&gpu_clk_freq, NULL);
	}
	//gpu_debug_out(mach);

	pthread_exit(NULL);
}