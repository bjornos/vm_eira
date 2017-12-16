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

#ifndef __IOPORT_H__
#define __IOPORT_H__

#include <stdint.h>

#define IO_INPUT_PORT	"machine/eira_input"
#define IO_OUTPUT_PORT	"machine/eira_output"

struct _io_regs {
	uint16_t input;
	uint16_t output;
};



void ioport_reset(void *mach);

void ioport_shutdown(int input_state);

void *ioport_machine_output(void *mach);

void *ioport_machine_input(void *mach);


#endif /* __IOPORT_H__ */