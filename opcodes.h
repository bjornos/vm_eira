/*
 * opcodes.h
 *
 *  Created on: Jan 17, 2017
 *      Author: bjornos
 */

#ifndef OPCODES_H_
#define OPCODES_H_

#define PROGRAM_MAGIC 0xdcbafedc

/* instruction: halt
 * | 0000 0000 | 0000 0000 | 0000 0000 0000 0000 |
 *   0           8           16
 */
#define halt	0x00

/* instruction: nop
 * | 1000 0000 | 0000 0000 | 0000 0000 0000 0000 |
 *   0           8           16
 */
#define nop		0x01

/* instruction: add [dst, src]
 * | 0100 0000 | 0000      | 00    00      |  0000 0000 0000 0000 |
 *   0          8 [DSTREG]  12src 14dst     16 [VALUE | SRC ADDR | REG] | DST ADDR ]
 *                              0: val
 *                              1: reg
 *                              2: mem
 */
#define add		0x02



#define sub		0x03

/** instruction: jmp [label]
 * | 0000 0000 | 0000 0000 0000 0000 0000 0000 |
 *   0          8      address     24| reserved
*/
#define jmp		0x10

/** instruction: cmp
 *
 *  syntax
 *  cmp [gp reg],[value]
 *
 *  The result is placed in the cpu conditional register
 *
 * | 0001 0001 |  0000  | 0000 | 0000 0000 0000 0000 |
 *   0          8       12      16
 *    instr       reg     src          value
*/
#define cmp		0x11

/** instruction: breq
 *
 *  syntax
 *  breq [address]
 *  jump to address if compare result was equal
 *
 *  desc
 *
 * | 0001 0010 | 0000 0000 | 0000 0000 0000 0000 |
 *   0          8            16
 *    instr       reserved   addr
*/
#define breq	0x12



/* instruction: mov [dst, src]
 * | 0000 0000 | 0000      | 00    00      |  0000 0000 0000 0000 |
 *   0          8 [DSTREG]  12src 14dst     16 [VALUE | ADDR | REG]
 *                              0: val
 *                              1: reg
 *                              2: mem
 * add r0,@0xfea (addr)
 * add r1,r2
 * add @1024, 255
 * add @512, r3
 * add r1,0xffa    0x02 00 01 00 0000 ffa0 0000
 * add r1,0xff     0x02 00 01 00 0000 ff00 0000 0xff0102
 *
 * dest           src
 * mem reg val   mem reg val
 *
 *
 * general purpose register = 16 st. -> 4 bitar.
*/
#define mov		0x04

/* no args */
#define clrscr	0xf0

/*
 * instruction: setposxy
 *
 * set cursor/pixelpos at x,y
 * | 0000 0000 | 0000 0000 0000 | 0000 0000 0000 |
 *   0          8    xpos      15      ypos
 *
 *   x- and ypos are general purpose registers
 *   12-bit value may address up to 4096 pixels.
*/
#define setposxy	0xf1

/**
 * instruction: getposxy
 *
 * get cursor/pixelpos at x,y
 * | 0000 0000 | 0000 0000 0000 | 0000 0000 0000 |
 *   0          8    xpos       15     ypos
 *
 *   The result must be placed in general purpose registers
*/
#define getposxy	0xf2

/**
 * Instruction screenout
 *
 * produces a charachter output in screen mem at position
 * px py.
 *
 * | 0000 0000  | 0000  0000  |  0000 0000 0000 0000 |
 *   0          8 ASCII code  15
 *
 *   The result must be placed in general purpose registers
 */
#define screenout	0xf3




/*   |-------| 255 - 8 bitars, 16 bitars maxint.
 0000 0000 0000 0000

mov src, dst

mov 100, r2
mov @100, r4


instr reg  imm val/mem addr
|---|----|----|----|
0000 0000 0000 0000


255 instr.
16 reg
65535 minne

uint32_t
instr      reg reg     imm val memaddr
|---|----|----|----| |---------------|
0000 0000 0000 0000  00000000 00000000



*/

#endif /* OPCODES_H_ */
