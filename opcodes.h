#ifndef OPCODES_H_
#define OPCODES_H_

#define PROGRAM_MAGIC 0xdcbafedc

#define MEM_START_SCREEN	0x7fff
#define MEM_START_PROGRAM	0x1000
#define MEM_START_ROM		0x200

/**
 * instruction: halt
 *
 * | 0000 0000 | 0000 0000 0000 0000 0000 0000 |
 * 0           8                               31
 *                         reserved
 */
#define halt	0x00

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
 *  mov r9,@4096  - r9 = (uint16_t)&ram[4096]
 *  movi r10,8192
 *  movi @128, r10 - ram[128] = 0 (r10 & 0xff), ram[129] = 32 (r10 >> 16)
 *
 */
#define movi	0x05

/**
 * instruction: jmp
 *
 * syntax: jmp [label]
 *
 * the assembler/compiler need to determine the conversion from
 * [label] to address.
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
 * | 1001 1000 |  0000  | 00 | 00 | 0000 0000 0000 0000 |
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
 *
 * | 0001 0010 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16
 *    instr       reserved           addr
*/
#define breq	0x12

/**
 * instruction: branch not equal
 *
 * syntax: brneq [address]
 *
 * jump to address if compare result in conditional register is COND_NEQ
 *
 * | 0001 0010 | 0000 0000 | 0000 0000 0000 0000 |
 * 0           8           16
 *    instr       reserved           addr
*/
#define brneq	0x13



/*
 * instructions below are experimental
 * -----------------------------------
 *
 */

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

#endif /* OPCODES_H_ */
