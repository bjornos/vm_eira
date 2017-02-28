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
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

enum display_adapter_capabilities {
	display_setxy,
	display_getxy,
	display_setc,
	display_getc,
	display_clr,
};
/*
enum display_modes {
	display_40x12,
	display_80x25,
	display_320x240,
	display_640x480
};
*/
struct _display_adapter {
 	uint8_t *mem;
 	uint16_t x;
	uint16_t y;
	char c;
	int refresh;
	int mode;
};

void display_retrace(struct _display_adapter *display,  uint8_t machine_ram[]);

int display_request(struct _display_adapter *display, uint32_t *instr,
		int request, uint8_t machine_ram[]);

void display_init(struct _display_adapter *display, uint8_t machine_ram[]);



#endif /* __DISPLAY_H_ */