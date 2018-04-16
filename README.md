# VM_EIRA
My own virtual machine. Just for Fun.


A hobby project in C which implements a virtual machine with very loose specifications. Current state is a RISC-inspired little endian story with 64kb memory address limit.

The plan is to make it cross-platform compatible, but as it is developed under Linux current implementation relies on ANSI escape sequences for "screen output".

### Compiling

vm_eira builds with cmake

>cmake .  
>make  

Executables are placed in the "build" sub directory

## Hardware Description

### I/O PORT

The Eira machine has an I/O port consisting of 16 inputs and 16 outputs.
It is possible to set the input port by writing to the file `DEV_IO_INPUT` (defined in ioport.h)
and it is possible to read the output port by reading the file `DEV_IO_OUTPUT`.

Should the virtual machine unexpectedly crash, it will be neccessary to manually remove
these files before the machine can be restarted (as you will notice by the error messages).

Example usage:   
`>ech0 2140 > <DEV_IO_INPUT>`  

Set input pins 2,3,4,6 and 11 high.

`>cat <IO_OUPUT_PORT>`  

Returns numeric value of 16 bit output port.

### LOADING PROGRAMS

The program memory can be loaded when the machine is started using command
line options or at a later stage by using the internal program loader.

The program loader is defined in prg.h as DEV_PRG_LOADER and is used:  
`echo <program name> > <DEV_PRG_LOADER>`  
e.g  
`echo bin/eira_test.bin > machine/prg_load`

### MEMORY MAP

```text

0xFFFFF
       |-------------------
       |
       |
       |
       | PROGRAM MEMORY
       |
       |
       |
0x1000 |-------------------
       |  
       |
       |  I/O MEM
0x0400 |------------------ Addresses below 0x0400 are Read Only
       |
       | ROM CODE
0x0200 | ------------------
       |
       | MACHINE REGISTERS
       |
0x0000 |------------------

```
