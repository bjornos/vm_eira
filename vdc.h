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

#ifndef __VDC_H__
#define __VDC_H__

#include <stdint.h>
#include <pthread.h>

#include "exception.h"

/* fixme: make cross platform compatible */
#define vdc_display_clear() printf("\033[H\033[J")
#define vdc_gotoxy(x,y) 	printf("\033[%d;%dH", (y), (x))
#define vdc_cursor_on()		printf("\x1B[?25h")
#define vdc_cursor_off()	printf("\x1B[?25l")

#define INSTR_LIST_SIZE 32

#define DISPLAY_FRAME_RATE	100	/* 100 Hz*/

typedef enum {
	mode_40x12,
	mode_80x25,
	mode_unknown,
} display_mode;

struct _cursor_data {
	uint16_t x;
	uint16_t y;
	char face;
};

struct _display_adapter {
	struct _cursor_data cursor_data;
	int refresh;
	display_mode mode;
	int enabled;
};

struct _vdc_regs {
	uint8_t *frame_buffer;
	uint8_t *text_buffer;
	uint32_t instr_list[INSTR_LIST_SIZE];
	uint32_t curr_instr;
	unsigned char instr_ptr;
	pthread_mutex_t instr_lock;
	int exception;
	unsigned char reset;
	struct _display_adapter display;
};


exception_t vdc_add_instr(struct _vdc_regs *vdc, uint32_t *instr);

void vdc_reset(void *mach);

void *vdc_machine(void *mach);

#endif /* __VDC_H__ */