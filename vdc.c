/*
 * Eira Virtual Machine
 *
 * Copyright (C) 2018
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

#include "vdc.h"
#include "opcodes.h"
#include "memory.h"
#include "machine.h"
#include "utils.h"

#define VDC_DBG(x) x

enum {
	VDC_LOCKED,
	VDC_UNLOCKED
};

struct _adapter_mode {
	display_mode name;
	uint16_t vertical;
	uint16_t horizontal;
	uint32_t resolution;
};

static const struct _adapter_mode adapter_mode[] = {
	{mode_40x12, 40, 12, 40*12},
	{mode_80x25, 80, 25, 80*25},
};

static void display_retrace(struct _vdc_regs *vdc)
{
	int cx,cy,addr;

	if (!vdc->display.enabled)
		return;

	if (!vdc->frame_buffer) {
		fprintf(stderr, "No display mem!\n");
		return;
	}

	vdc->display.refresh = 1;

	for(cy=0; cy < adapter_mode[vdc->display.mode].horizontal; cy++) {
		addr = (cy * adapter_mode[vdc->display.mode].vertical);
		for (cx=0; cx < adapter_mode[vdc->display.mode].vertical; cx++) {
			vdc_gotoxy(cx,cy);
			putchar((char)*(vdc->frame_buffer + addr) & 0xff);
			addr++;
		}
	}

	vdc->display.refresh = 0;
}

static void display_wait_retrace(struct _vdc_regs *vdc)
{
	while(vdc->display.refresh);
}


static exception_t vdc_set_mode(struct _vdc_regs *vdc, display_mode this_mode)
{
	vdc->display.cursor_data.x = 0;
	vdc->display.cursor_data.y = 0;
	vdc->display.cursor_data.face = '\0';

	if (this_mode < mode_unknown)
		vdc->display.mode = this_mode;
	else
		return EXC_DISP;

	memset(vdc->frame_buffer,' ', adapter_mode[vdc->display.mode].resolution);
	vdc->display.refresh = 0;
	vdc->display.enabled = 1;

	return EXC_NONE;
}


void vdc_decode_instr(struct _vdc_regs *vdc, struct _display_adapter *display,struct _machine *machine)
{
	uint8_t opcode;
	int vdc_exception = EXC_NONE;

	opcode = vdc->curr_instr & 0xff;

	/*if (!machine->display.enabled && (request != DISPLAY_INIT))
		return EXC_DISP;*/

	VDC_DBG(vdc_gotoxy(1,21));
	VDC_DBG(printf("vdc instr:\t0x%x\t\tip: %d  \n",opcode, vdc->instr_ptr));

	switch(opcode) {
		case diwait:
			break;
		case diwtrt:
			display_wait_retrace(vdc);
			break;
		case dimd:
			vdc_exception = vdc_set_mode(vdc, (vdc->curr_instr >> 8));
			break;
 		case diclr:
			memset(&vdc->frame_buffer[0], ' ', adapter_mode[vdc->display.mode].resolution);
			break;
		case disetxy:
			vdc->display.cursor_data.x = machine->cpu_regs.GP_REG[ (vdc->curr_instr >> 16) & 0xff ];
			vdc->display.cursor_data.y = machine->cpu_regs.GP_REG[ (vdc->curr_instr >> 24) & 0xff ];
			break;
		case dichar: {
			char c;
			int addr;
			if (vdc->curr_instr & OP_SRC_MEM) {
				if  ((vdc->curr_instr >> 16) > MEM_START_RW) {
					c = machine->RAM[(vdc->curr_instr >> 16)];
				} else {
					c = machine->RAM[ machine->cpu_regs.GP_REG [ (vdc->curr_instr >> 16)] ];
				}
			} else if (vdc->curr_instr & OP_SRC_REG) {
				c = machine->cpu_regs.GP_REG[(vdc->curr_instr >> 16) & 0xff ];
			} else
				c = (vdc->curr_instr >> 16) & 0xff;
			addr = (vdc->display.cursor_data.y * adapter_mode[vdc->display.mode].vertical) + vdc->display.cursor_data.x;
			*(vdc->frame_buffer + addr) = c;
			}
			break;
		default:
			printf("vdc error unknown. instr: 0x%x ip: %d\n", opcode, vdc->instr_ptr);
			vdc_exception = EXC_VDC;
			break;
	}

	vdc->exception = vdc_exception;
}

exception_t vdc_add_instr(struct _vdc_regs *vdc, uint32_t *instr)
{
	if (vdc->instr_ptr >= INSTR_LIST_SIZE)
		return EXC_VDC;

	pthread_mutex_lock(&vdc->instr_lock);

	VDC_DBG(vdc_gotoxy(1,20));
	VDC_DBG(printf("vdc adding instr:\t0x%x\t ip: %d \t\t \n",*instr, vdc->instr_ptr));

	vdc->instr_list[vdc->instr_ptr++] = *instr;

	pthread_mutex_unlock(&vdc->instr_lock);

	return EXC_NONE;
}

void vdc_fetch_instr(struct _vdc_regs *vdc)
{
	pthread_mutex_lock(&vdc->instr_lock);

	if (vdc->instr_ptr) {
		vdc->curr_instr = vdc->instr_list[0];

		for (int i = 0; i < vdc->instr_ptr; i++)
			vdc->instr_list[i] = vdc->instr_list[i + 1];

		vdc->instr_ptr--;
	} else {
		vdc->curr_instr = diwait;
	}

	pthread_mutex_unlock(&vdc->instr_lock);
}


void vdc_reset(void *mach)
{
	struct _machine *machine = mach;

	machine->vdc_regs.reset = 1;
	machine->vdc_regs.display.enabled = 0;
	machine->vdc_regs.display.refresh = 0;

	for (int i=0; i < INSTR_LIST_SIZE; i++)
		machine->vdc_regs.instr_list[i] = diwait;

	machine->vdc_regs.instr_ptr = 0;
	machine->vdc_regs.exception = EXC_NONE;

	pthread_mutex_init(&machine->vdc_regs.instr_lock, NULL);
}

void *vdc_machine(void *mach)
{
	struct _machine *machine = mach;
	struct timespec vdc_clk_freq;

	vdc_clk_freq.tv_nsec = 1000000000 / DISPLAY_FRAME_RATE;  
	vdc_clk_freq.tv_sec = 0;

	while(!machine->cpu_regs.panic) {
		while(machine->vdc_regs.reset);

		vdc_fetch_instr(&machine->vdc_regs);

		vdc_decode_instr(&machine->vdc_regs, &machine->display, machine);

		if (machine->vdc_regs.display.enabled)
			display_retrace(&machine->vdc_regs);
		
		machine->cpu_regs.exception |= machine->vdc_regs.exception;

		nanosleep(&vdc_clk_freq, NULL);
	}

	pthread_exit(NULL);
}