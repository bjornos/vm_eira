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

#ifndef __DISPLAY_H_
#define __DISPLAY_H_

#include <stdint.h>
#include <stdio.h>

/* fixme: make cross platform compatible */
#define display_clear() printf("\033[H\033[J")
#define gotoxy(x,y) 	printf("\033[%d;%dH", (y), (x))
#define cursor_on()		printf("\x1B[?25h")
#define cursor_off()	printf("\x1B[?25l")

#define DISPLAY_FRAME_RATE	50	/* 50Hz*/

struct _cursor_data {
	uint16_t x;
	uint16_t y;
	char face;
};

enum display_adapter_capabilities {
	DISPLAY_INIT,
	DISPLAY_SETXY,
	DISPLAY_GETXY,
	DISPLAY_SETC,
	DISPLAY_GETC,
	DISPLAY_CLR,
};

typedef enum {
	mode_40x12,
	mode_80x25,
	mode_unknown,
} display_mode;


struct _display_adapter {
	uint8_t *frame_buffer;
 	uint16_t x;
	uint16_t y;
	char c;
	int refresh;
	int mode;
	int enabled;
};

void display_retrace(struct _display_adapter *display);

void display_wait_retrace(struct _display_adapter *display);

int display_request(struct _display_adapter *display, uint32_t *instr,
	uint8_t *frame_buffer, int request);

void display_reset(struct _display_adapter *display);

void *display_machine(void *mach);



#endif /* __DISPLAY_H_ */
