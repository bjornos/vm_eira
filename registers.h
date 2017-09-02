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

#ifndef REGISTERS_H_
#define REGISTERS_H_


#define GP_REG_MAX 	0xf

#define R0		0x00
#define R1		0x01
#define R2		0x02
#define R3		0x03
#define R4		0x04
#define R5		0x05
#define R6		0x06
#define R7		0x07
#define R8		0x08
#define R9		0x09
#define R10		0x0a
#define R11		0x0b
#define R12		0x0c
#define R13		0x0d
#define R14		0x0e
#define	R15		0x0f

/* mov */
#define	OP_SRC_REG	(1 << 12)
#define OP_SRC_MEM	(1 << 13)
#define OP_DST_REG	(1 << 14)
#define OP_DST_MEM	(1 << 15)


#endif /* REGISTERS_H_ */
