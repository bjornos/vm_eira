# vm_eira
My own virtual machine. Just for Fun.


A hobby project in C which implements a virtual machine with very loose specifications.
Current state is a RISC-inspired little endian story with 64kb memory address limit.

The plan is to make it cross-platform compatible, but as it is developed under Linux
current implementation relies on ANSI escape sequences for "screen output".

GPU
---
The GPU is a co-processor to the main CPU which handles all the display related opcodes.
It works on an instruction list which the main CPU populates whenever it hit a GPU related
opcode. The default instruction is "wait" which of course means do nothing.

I/O PORT
--------
The Eira machine has an I/O port consisting of 16 inputs and 16 outputs.
It is possible to set the input port by writing to the file IO_INPUT_PORT (defined in ioport.h)
and it is possible to read the output port by reading the file IO_OUTPUT_PORT.

Should the virtual machine unexpectedly crash, it will be neccessary to manually remove
these files before the machine can be restarted (as you will notice by the error messages).

Example usage:
ech0 2140 > <IO_INPUT_PORT>
Set input pins 2,3,4,6 and 11 high.

cat <IO_OUPUT_PORT>
Returns numeric value of 16 bit output port.

LOADING PROGRAMS
----------------
The program memory can be loaded when the machine is started using command
line options or at a later stage by using the internal program loader.

The program loader is defined in prg.h as PRG_LOADER_FIFO and is used:
echo <program name> > <PRG_LOADER_FIFO>
e.g
echo bin/eira_test.bin > /tmp/eira_prg

MEMORY MAP
----------

0xffff	|-------------------
		|
		|
		|
		|
		| SCREEN MEM
		|
		|
		|
07fff	|-------------------
		|
		|
		| PROGRAM MEM
		|
		|
		|
		|
0x1000	|-------------------
		|
		|
		| I/O MEM
0x0400	|---------------- addresses below 0x0400 are Read Only
		|
		| ROM CODE
0x0200	|-------------------
		|
		|
		| MACHINE REGISTERS
		|
		|
0x0000	|-------------------
