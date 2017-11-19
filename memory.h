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

#ifndef __MEMORY_H_
#define __MEMORY_H_

#define RAM_SIZE		0xffff

#define MEM_START_GPU_FB	MEM_START_GPU
#define MEM_START_GPU		0x7fff

#define MEM_START_PRG		0x1000

#define MEM_IO_OUTPUT		(MEM_IO_INPUT + sizeof(uint16_t))
#define MEM_IO_INPUT		MEM_START_IOPORT
#define MEM_START_IOPORT	0x0400

/* first kb of RAM is read only and
 * reserved for the machine
 */
#define MEM_START_RW		0x0400

#define MEM_START_ROM		0x0200

#define MEM_BOOT_STATUS		0x0101
#define MEM_PRG_LOADING		0x0100

#define MEM_ROM_BOOT_MSG	0x0020 /* 16 chars of boot text segment */
#define MEM_ROM_BOOT_ANIM	0x0030 /* 16 chars */

#endif /* __MEMORY_H_ */
