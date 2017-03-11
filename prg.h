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

#ifndef __PRG_H_
#define __PRG_H_

#include <stdint.h>

#define PRG_MAGIC_HEADER	0xe113a100

struct _prg_header {
	uint32_t magic;
	uint32_t reserved1;
	uint32_t reserved2;
	uint32_t code_size;
};

struct _prg_format {
	struct _prg_header header;
	uint32_t *code_segment;
};

#endif /* __PRG_H_ */
