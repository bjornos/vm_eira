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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "opcodes.h"
#include "testprogram.h"
#include "prg.h"

#undef NDEBUG
#include <assert.h>

#define UNIT_TEST 0

/* fixme: make cross platform compatible */
#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

#define DBG(x)
#define CPU_VERSION	0x1

#define RAM_SIZE	0xffff
#define BYTE_SIZE 0
#define INT_SIZE

enum op_size {
	SIZE_BYTE,
	SIZE_INT,
};

enum conditions {
	COND_EQ = 1,
	COND_NEQ = 2,
	COND_ZERO = 4,
	COND_NZERO = 8,
	COND_GR = 16,
	COND_LE = 32,
	COND_UNDEFINED = 64,
};

enum exceptions {
	EXC_INSTR,
	EXC_MEM,
	EXC_REG,
	EXC_PRG,
};

enum screen_adapter_capabilities {
	screen_setxy,
	screen_getxy,
	screen_setc,
	screen_getc,
	screen_clr,
};


struct _cpu_regs {
	long pc;
	int sp;
	int cr;
	char exception;
	int panic;
};

int screen_modes[] = {
	40  * 12,
	80  * 25,
	320 * 240,
	640 * 480,
};

struct _screen_adapter {
	uint8_t *screen_mem;
	uint16_t x;
	uint16_t y;
	char c;
	int refresh;
	int mode;
};

static struct _machine {
	uint8_t RAM[RAM_SIZE];
	uint16_t GP_REG[16]; /* General Purpose Registers - fixme: move into cpu regs */
	struct _cpu_regs cpu_regs;
	struct _screen_adapter screen_adapter;
} machine;


void dump_ram(int from, int to)
{
	int r,v;
	int p=0;
	int rows=0;
	int grid = 8;
	int range;

	printf("RAM  %d -> %d:\n----\n", from, to);

	/* round up range to nearest grid size */
	range = ((to - from) + grid)  & ~(grid -1);

	for (r=0; r<(range / grid); r++) {
		printf("0x%.2x:\t",rows+from);
		for (v=0; v<grid; v++) {
			assert(from + p <= RAM_SIZE);
			printf("%d\t", machine.RAM[from + p] & 0xff);
			p++;
		}
		rows +=grid;
		printf("\n");
	}
	printf("\n");
}

void dump_regs(void)
{
	int i,o;
	int grid = 4;
	int r = 0;

	printf("General Purpose Registers:\n--------------------------\n");
	for (i=0; i <= (GP_REG_MAX / grid); i++) {
		for (o=0; o < grid; o++) {
			printf("R%d:\t%d\t",r,machine.GP_REG[r]);
			r++;
		}
		printf("\n");
	}
	printf("\n");
	printf("CR: %d PC: %ld\n",machine.cpu_regs.cr,machine.cpu_regs.pc);
}



/* fixme: func name */
uint16_t mnemonic(uint32_t *instr, uint16_t **dst, int opsize, const char dbg_instr[])
{
	uint16_t local_src;
	uint16_t local_dst;
	uint16_t src;

	printf("%s %s --- ",__func__, dbg_instr);

	/* value destination general purpose register */
	if (*instr & OP_DST_REG) {
		local_dst = (*instr >> 8) & 0x0f;
		local_src = (*instr >> 16) & 0xffff;

		*(dst) = machine.GP_REG + local_dst;

		if (*instr & OP_SRC_REG) {
			/* value from register */
			DBG(printf("GP_REG%d -> GP_REG%d\n",local_src, local_dst));
			if (local_src > GP_REG_MAX) {
				machine.cpu_regs.exception = EXC_REG;
				goto out;
			}
			src = machine.GP_REG[local_src];
			goto out;
		}
		if (*instr & OP_SRC_MEM) {
			/* copy from memory */
			DBG(printf("0x%x -> GP_REG%d\n",local_src,local_dst));
			if (local_src > RAM_SIZE) {
				machine.cpu_regs.exception = EXC_MEM;
				goto out;
			}
			if (opsize == SIZE_INT) {
				src = machine.RAM[local_src];
				src |= machine.RAM[local_src + 1] << 8;
			} else
				src = machine.RAM[local_src];

			printf("RAM SRC = 0x%x  opsize=%d\n",src,opsize);
			goto out;
		}
		/* immediate value */
		DBG(printf("%d -> GP_REG%d\n",local_src,local_dst));
		src = local_src;
		goto out;
	}

	/* value destination memory */
	if (*instr & OP_DST_MEM) {
		local_src = (*instr >> 8) & 0x0f;
		local_dst = (*instr >> 16) & 0xffff;
		DBG(printf("GP_REG%d -> @%d\n",local_src,local_dst));

		if (local_dst > RAM_SIZE) {
			machine.cpu_regs.exception = EXC_MEM;
			goto out;
		}

		if (opsize == SIZE_INT)
			*(dst) = (uint16_t *)machine.RAM + local_dst; // FIXME: verify
		else
			*(dst) = (void *)machine.RAM + local_dst;

		src = machine.GP_REG[local_src] & 0xff;
	}

out:
		return src;
}

void compare(uint16_t t1, uint16_t t2)
{
	int d = t2 - t1;

	printf("%s --- ",__func__);

	if (d == 0)
		machine.cpu_regs.cr = COND_EQ | COND_ZERO;
	if (d > 0)
		machine.cpu_regs.cr = COND_GR | COND_NEQ;
	if (d < 0)
		machine.cpu_regs.cr = COND_LE | COND_NEQ;

	printf("t1:%d t2:%d d:%d desc: %d\n",t1, t2,d,machine.cpu_regs.cr);
}


void screen_retrace(void)
{
	int screen_addr;

	gotoxy(machine.screen_adapter.x,machine.screen_adapter.y);

	machine.screen_adapter.screen_mem = machine.RAM + MEM_START_SCREEN;
	screen_addr =	/* FIXME: respect screen mode */
		(machine.screen_adapter.y * 40) + machine.screen_adapter.x;
	machine.screen_adapter.screen_mem = machine.RAM + MEM_START_SCREEN + screen_addr;

	printf("%c",(char)*machine.screen_adapter.screen_mem & 0xff);

	machine.screen_adapter.refresh = 0;
}

void screen_request(uint32_t *instr, int request)
{
	int screen_addr = 0;

	DBG(printf("%s 0x%x --- ",__func__,request));

	switch(request) {
		case screen_setxy:
			machine.screen_adapter.x = (*instr >> 8) & 0xfff;
			machine.screen_adapter.y = (*instr >> 20) & 0xfff;
			gotoxy(5,20);
			printf("x:%d y:%d \n",machine.screen_adapter.x ,machine.screen_adapter.y);
			DBG(printf("x:%d y:%d \n",machine.screen_adapter.x ,machine.screen_adapter.y));
			break;
		case screen_setc:
			screen_addr =	/* FIXME: screen mode */
				(machine.screen_adapter.y * 40) + machine.screen_adapter.x;
			machine.screen_adapter.c = (*instr >> 8) & 0xff;
			machine.screen_adapter.screen_mem =
				machine.RAM + MEM_START_SCREEN + screen_addr;
			*machine.screen_adapter.screen_mem = machine.screen_adapter.c;
			machine.screen_adapter.refresh = 1;
			DBG(printf("c:%c \n", (char)machine.screen_adapter.c));
			break;
		default:
			machine.cpu_regs.exception = 2;
	}
}

void decode_instruction(uint32_t *instr)
{
	uint8_t opcode;
	uint16_t src;
	uint16_t *dst;

	if (machine.cpu_regs.exception)
		return;

	DBG(printf("%s 0x%x\n",__func__, *instr));

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop: DBG(printf("nop\n"));
			break;
		case halt: DBG(printf("halt\n"));
			machine.cpu_regs.panic = 1;
			break;
		case mov:
			src = mnemonic(instr, &dst, SIZE_BYTE, "mov");
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case movi:
			src = mnemonic(instr, &dst, SIZE_INT,"movi");
			if (!machine.cpu_regs.exception)
				*dst = src;
			break;
		case add:
			src = mnemonic(instr, &dst,SIZE_BYTE, "add");
			if (!machine.cpu_regs.exception)
				*dst += src;
			break;
		case sub: DBG(printf("sub\n"));
			src = mnemonic(instr, &dst,SIZE_BYTE,"sub");
			if (!machine.cpu_regs.exception)
				*dst -= src;
			break;
		case jmp: DBG(printf("jmp "));
			machine.cpu_regs.pc = (*instr >> 8) - 4; /* compensate for pc++ */
			DBG(printf("%ld\n", machine.cpu_regs.pc));
			break;
		case cmp: DBG(printf("cmp "));
			machine.cpu_regs.cr &= COND_UNDEFINED;
			src = mnemonic(instr, &dst, SIZE_INT, "cmp");
			compare(src, *dst);
			DBG(printf("cr: 0x%x\n", machine.cpu_regs.cr));
			break;
		case breq: DBG(printf("breq "));
			if (machine.cpu_regs.cr &= COND_EQ) {// todo: make a jump func
				machine.cpu_regs.pc = (*instr >> 16);
				printf("jump to %ld \n",machine.cpu_regs.pc);
				machine.cpu_regs.pc -= 4; /* compensate for pc + 4 */
			}
			break;
		case brneq: DBG(printf("brneq "));
			if (machine.cpu_regs.cr &= COND_NEQ) {// todo: make a jump func
				printf("pc = %ld \n",machine.cpu_regs.pc);
				machine.cpu_regs.pc = (*instr >> 16);
				printf("jump to %ld \n",machine.cpu_regs.pc);
				machine.cpu_regs.pc -= 4; /* compensate for pc + 4 */
				//exit(0);
			}
			break;
		case clrscr: DBG(printf("clrscr\n"));
			clear();
			break;
		case setposxy: DBG(printf("setposxy\n"));
			screen_request(instr,screen_setxy);
			break;
		case screenout: DBG(printf("outscreen\n"));
			screen_request(instr,screen_setc);
			break;
		default: machine.cpu_regs.exception = EXC_INSTR;
			break;
	}
}


void reset_cpu(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);
	memset(&machine.GP_REG, 0x00, GP_REG_MAX);

	machine.cpu_regs.pc = 0;
	machine.cpu_regs.sp = 0;
	machine.cpu_regs.exception = 0;
	machine.cpu_regs.panic = 0;
	machine.cpu_regs.cr = COND_UNDEFINED;

	machine.cpu_regs.pc = MEM_START_ROM + PRG_HEADER_SIZE;
}

void init_screen(void) {
	machine.screen_adapter.x = 0;
	machine.screen_adapter.y = 0;
	machine.screen_adapter.c = '\0';
	machine.screen_adapter.mode = 0;

	machine.screen_adapter.screen_mem = machine.RAM + MEM_START_SCREEN;
	memset(machine.screen_adapter.screen_mem, 0x00, screen_modes[machine.screen_adapter.mode]);
	machine.screen_adapter.refresh = 0;
}

void load_program(uint32_t *prg, uint16_t addr) {
	int prg_size = prg[PRG_SIZE_OFFSET];

	if (prg_size > (RAM_SIZE - MEM_START_PRG))
		machine.cpu_regs.exception = EXC_PRG;
	else
		memcpy(&machine.RAM[addr], prg, prg_size);
}

void cpu_exception(long pc) {
	printf("!! %s: ",__func__);

	switch(machine.cpu_regs.exception) {
	case EXC_INSTR:
			printf("illegal instruction 0x%X ",
				machine.RAM[pc]);
			break;
	case EXC_MEM:
			printf("cannot access memory ");
			break;
	case EXC_REG:
			printf("cannot access register ");
			break;
	case EXC_PRG:
			printf("stray program ");
			break;
	default:
			printf("unknown exception %d",
				machine.cpu_regs.exception);
			break;
	}

	printf("[pc: %ld]\n", machine.cpu_regs.pc);

	machine.cpu_regs.panic = 1;
}

__inline static long cpu_fetch_instruction(void){
	machine.cpu_regs.exception = 0;

	/* each instruction is 4 bytes */
	machine.cpu_regs.pc += 4;
	if (machine.cpu_regs.pc >= (RAM_SIZE - 3))
		machine.cpu_regs.exception = EXC_PRG;

	return machine.cpu_regs.pc;
}

int main(int argc,char *argv[])
{
	long instr_p;
	int run_test_prg = 0;

	if ((argc == 2) && (strcmp(argv[1],"--test") == 0))
		run_test_prg = 1;

	reset_cpu();
	init_screen();

	load_program(rom, MEM_START_ROM);

	if (run_test_prg)
		load_program(program_unit_test_basic, MEM_START_PRG);

	while(!machine.cpu_regs.panic) {
		instr_p = cpu_fetch_instruction();
		decode_instruction((uint32_t *)&machine.RAM[instr_p]);

		if (machine.cpu_regs.exception)
			cpu_exception(instr_p);

		if (machine.screen_adapter.refresh) {
			screen_retrace();
			usleep(100 * 1000);
		}
	}
	dump_ram(MEM_START_PRG,MEM_START_PRG + PRG_HEADER_SIZE);
	dump_regs();
	dump_ram(8192,8192+8);

	return 0;
}

