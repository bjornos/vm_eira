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

#ifndef __GPU_H__
#define __GPU_H__

#include <stdint.h>
#include "display.h"

#define GPU_INSTR_BUFFER_SIZE	16

struct _gpu {
	uint8_t *frame_buffer;
	uint32_t instr_list[GPU_INSTR_BUFFER_SIZE];
	int instr_ptr;
	int exception;
	unsigned char instr_lock;
	unsigned char reset;
};


void gpu_add_instr(struct _gpu *gpu, uint32_t *instr);

void gpu_reset(struct _gpu *gpu, uint8_t *RAM);

void *gpu_machine(void *mach);

#endif /* __GPU_H__ */