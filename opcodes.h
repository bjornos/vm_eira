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

#ifndef __OPCODES_H__
#define __OPCODES_H__

#define OPCODE_NAME_MAX	16

/* compiler magic for comment and labels - not really a valid opcode */
#define OPCODE_ENCODE_SKIP	0xffff

/* compiler magic for complications - not really a valid opcode */
#define OPCODE_ENCODE_ERROR		0xfffe

/**
 * instruction: halt
 *
 * | 0000 0000 | 0000 0000 0000 0000 0000 0000 |
 * 0           8                               31
 *                         reserved
 */
#define halt		0x00

/**
 * instruction: nop
 *
 * | 1000 0000 | 0000 0000 0000 0000 0000 0000 |
 * 0           8                               31
 *                        reserved
 */
#define nop		0x01

/**
 * instruction: add
 *
 * syntax: add dest, source
 *
 * | 0100 0000 | 0000 | 00  | 00 | 0000 0000 0000 0000 |
 * 0           8      12    14   16                    31
 *               reg  src    dst    #val | @mem | reg
 *                    type   type
 *
 * if destination is memory, the address is stored in the upper 16 bits and
 * the value is taken from registerX in bits 8-12. It is NOT possible to add
 * immediate values directly to memory.
 *
 * example usage:
 *  add r0,16   - r0 = r0 + 16
 *
 *  mov r1, 16
 *  add @128, r1 - ram[128] = ram[128] + 16
 *
 */
#define add		0x02


/**
 * instruction: sub
 *
 * syntax: sub dest, source
 *
 * | 1100 0000 | 0000 | 00  | 00 | 0000 0000 0000 0000 |
 * 0           8      12    14   16                    31
 *                reg  src    dst   #val | @mem | reg
 *                     type   type
 *
 * if destination is memory, the address is stored in the upper 16 bits and
 * the value is taken from registerX in bits 8-12. It is NOT possible to sub
 * immediate values directly to memory.
 *
 * example usage:
 *  sub r0,16   - r0 = r0 + 16
 *
 *  mov r1, 28
 *  sub @128, r1 - ram[128] = ram[128] - 28
 *
 */
#define sub		0x03

/**
 * instruction: mov
 *
 * syntax: mov dest, source
 *
 *  | 0010 0000 | 0000 | 00  | 00 | 0000 0000 0000 0000 |
 *  0           8      12    14   16                    31
 *                 reg  src    dst   #val | @mem | reg
 *                      type   type
 *
 * if destination is memory, the address is stored in the upper 16 bits and
 * the value is taken from registerX in bits 8-12. It is NOT possible to move
 * immediate values directly to memory.
 *
 * example usage:
 *  mov r0,16   - general purpose register 0 = 16
 *
 *  mov r10, 60
 *  mov @128, r10 - ram[128] = 60
 *
 */
#define mov		0x04

/**
 * instruction: movi - move integer (16-bit)
 *
 * syntax: movi dest, source
 *
 *  | 1010 0000 | 0000 | 00  | 00 | 0000 0000 0000 0000 |
 *  0           8      12    14   16                    31
 *                 reg  src    dst   #val | @mem | reg
 *                      type   type
 *
 * if destination is memory, the address is stored in the upper 16 bits and
 * the value is taken from registerX in bits 8-12. It is NOT possible to move
 * immediate values directly to memory.
 *
 * example usage:
 *  mov r9,@4096  - r9 = (uint16_t)&ram[4096]
 *  movi r10,8192
 *  movi @128, r10 - ram[128] = 0 (r10 & 0xff), ram[129] = 32 (r10 >> 16)
 *
 */
#define movi		0x05

/**
 * instruction: jmp
 *
 * syntax: jmp [label]
 *
 * the assembler/compiler need to determine the conversion from
 * [label] to address.
 * if address is less than ROM start, the value in of GP REG [register]
 * is used instead. if the address value is above GP REG MAX, an exception
 * will occur.
 *
 *
 *  | 0000 1000 | 0000 0000 0000 0000 | 0000 0000 |
 *  0           8                     24          31
 *                     pc address        reserved
*/
#define jmp		0x10

/**
 * instruction: cmp
 *
 * syntax: cmp reg, #value | reg | @mem
 *
 * compare the value in general purpose register reg with immediate
 * value. the result is placed in the cpu conditional register (cr)
 *
 * | 100o 1000 |  0000  | 00 | 00 | 0000 0000 0000 0000 |
 * 0           8          12      16
 *    instr       reg     src  MBZ         value
*/
#define cmp		0x11

/**
 * instruction: branch equal
 *
 * syntax: breq [address]
 *
 * jump to address if compare result in conditional register is COND_EQ
 * if address is less than ROM start, the value in of GP REG [register]
 * is used instead. if the address value is above GP REG MAX, an exception
 * will occur.
 *
 * | 0100 1000 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16
 *    instr       reserved           addr
*/
#define breq		0x12

/**
 * instruction: branch not equal
 *
 * syntax: brneq [address]
 *
 * jump to address if compare result in conditional register is COND_NEQ.
 * if address is less than ROM start, the value in of GP REG [register]
 * is used instead. if the address value is above GP REG MAX, an exception
 * will occur.
 *
 * | 1100 1000 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16
 *    instr       reserved       addr | register
*/
#define brneq		0x13

/**
 * instruction: stopc
 *
 * syntax: stopc [register]
 *
 * store current value of program counter into general
 * purpose register
 *
 * | 0010 1000 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16
 *    instr       register       reserved
*/
#define stopc		0x14

/**
 * instruction: rst
 *
 * syntax: rst
 *
 * force a software reset of the machine
 *
 *
 * | 1010 1000 | 0000 0000 0000 0000 0000 0000 |
 * 0           8           24
 *    instr             reserved
*/
#define rst		0x15

/**
 * instruction: move mem
 *
 * syntax: movmr dest reg, ram[src reg value]
 *
 * copy a value from mem address contained in src register
 * into dest register
 *
 *
 * | 0110 1000 | 0000 | 0000 | 0000 0000 0000 0000 |
 * 0           8  dest   src 16
 *    instr       registers        reserved
*/
#define movmr	0x16



/**
 * instruction: display wait
 *
 * syntax: diwait
 *
 * Basically a nop for the GPU
 *
 * | 0000 1100 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16                   31
 *    instr        mode         reserved
 */
#define diwait		0x30

/**
 * instruction: display mode
 *
 * syntax: dimd [mode]
 *
 * mode:
 * 0x01: 40x12
 * 0x02: 80x25
 * 0x03: 640x480 (VGA)
 *
 *
 * | 1000 1100 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16                   31
 *    instr        mode         reserved
 */
#define dimd		0x31

/**
 * instruction: display wait retrace
 *
 * syntax: diwtrt
 *
 * wait for display retrace
 *
 * | 0100 1100 | 0000 0000 0000 0000 0000 0000 |
 * 0           8                               31
 *    instr               reserved
 */
#define diwtrt		0x32

/**
 * instruction: display clear
 *
 * syntax: diclr
 *
 * clear display adapter memory
 *
 * | 1100 1100 | 0000 0000 0000 0000 0000 0000 |
 * 0           8                              31
 *    instr               reserved
 */
#define diclr		0x33

/**
 * instruction: disetxy
 *
 * set cursor/pixelpos at x,y
 *
 * | 0010 1100 | 0000  0000 0000 | 0000 0000 0000 |
 *   0         8       xpos      20     ypos
 *
 *   xpos and ypos are general purpose registers containing
 *   x and y pixel position.
 *
 */
#define disetxy	0x34

/**
 * Instruction dichar
 *
 * produces a charachter output in screen display at position
 * px py.
 *
 * | 1010 1100  | 00 | 00 0000  | 0000 0000 0000 0000 |
 *   0          8 src  reserved 15 #val | @mem | reg
 *                type                 ASCII Code
 *
 * If SRC is MEM and value is < MEM_START_RW the value of MEM [ GP REG #value ]
 * will be used. If value in ths case is more than GP REG MAX an exception
 * will occur.
 *
 */
#define dichar		0x35

/**
 * Instruction diputpixel
 *
 * produces a pixel output in screen display at position
 * px py.
 *
 * | 1010 1100  |  0000 0000 | 0000 0000 0000 0000 |
 *   0          8  color     16   reserved         
 *
 *
 */
#define diputpixel		0x36


#endif /* __OPCODES_H__ */
