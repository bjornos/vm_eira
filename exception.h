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

#ifndef __EXCEPTION_H_
#define __EXCEPTION_H_

typedef enum exceptions {
	EXC_NONE		= (0 << 0),
	EXC_INSTR		= (1 << 1),
	EXC_MEM			= (1 << 2),
	EXC_REG			= (1 << 3),
	EXC_PRG			= (1 << 4),
	EXC_DISP		= (1 << 5),
	EXC_GPU			= (1 << 6),
	EXC_IOPORT		= (1 << 7),
	EXC_SHUTDOWN	= (1 << 8),
	EXC_VDC			= (1 << 9),
	EXC_END			= (1 << 31),
} exception_t;

#endif /*__EXCEPTION_H_  */
