#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "opcodes.h"
#include "testprogram.h"
#undef NDEBUG
#include <assert.h>

#define UNIT_TEST 0

/* fixme: make cross platform compatible */
#define clear() printf("\033[H\033[J")
#define gotoxy(x,y) printf("\033[%d;%dH", (y), (x))

#define DBG(x)
#define CPU_VERSION	0x1

#define RAM_SIZE	0xffff

#define SCREEN_MEM_START 0x7fff
#define PROGRAM_MEM_START 0x1000

enum conditions {
	COND_EQ,
	COND_NEQ,
	COND_ZERO,
	COND_NZERO,
	COND_GR,
	COND_LE,
	COND_UNDEFINED
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
}



/* fixme: func name */
uint16_t mnemonic(uint32_t *instr, uint16_t **dst, const char dbg_instr[])
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
				machine.cpu_regs.exception = 1;
				printf("exception: register %d out of bounds.\n",local_src);
				goto out;
			}
			src = machine.GP_REG[local_src];
			goto out;
		}
		if (*instr & OP_SRC_MEM) {
			/* copy from memory */
			DBG(printf("0x%x -> GP_REG%d\n",local_src,local_dst));
			if (local_src > RAM_SIZE) {
				machine.cpu_regs.exception = 1;
				printf("exception: cannot access ram@0x%x",local_src); /* fixme: MMU? */
				goto out;
			}
			src = machine.RAM[local_src] & 0xff;
			printf("RAM SRC = 0x%x\n",src);
			goto out;
		}
		/* immidiate value */
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
			printf("exception: cannot access ram @ 0x%x\n",local_dst); /* fixme: MMU? */
			machine.cpu_regs.exception = 1;
			goto out;
		}
		*(dst) = (uint8_t *)machine.RAM + local_dst;
		src = machine.GP_REG[local_src] & 0xff;
	}

out:
		return src;
}

void compare(uint32_t *instr)
{
	uint8_t c1;
	uint16_t c2;
	int d;

	printf("%s --- ",__func__);

	c1 = machine.GP_REG[(*instr >> 8) & 0x0f];
	c2 = (*instr >> 16) & 0xffff;

	d = c2 - c1;

	if (d == 0)
		machine.cpu_regs.cr = COND_EQ;
	if (d > 0)
		machine.cpu_regs.cr = COND_GR;
	if (d < 0)
		machine.cpu_regs.cr = COND_LE;

	printf("c1:%d c2:%d d:%d",c1, c2,d);
}


void screen_retrace(void)
{
	int screen_addr;

	gotoxy(machine.screen_adapter.x,machine.screen_adapter.y);

	machine.screen_adapter.screen_mem = machine.RAM + SCREEN_MEM_START;
	screen_addr =	/* FIXME: respect screen mode */
			(machine.screen_adapter.y * 40) + machine.screen_adapter.x;
	machine.screen_adapter.screen_mem = machine.RAM + SCREEN_MEM_START + screen_addr;

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
			screen_addr =		/* FIXME: screen mode */
					(machine.screen_adapter.y * 40) + machine.screen_adapter.x;
			machine.screen_adapter.c = (*instr >> 8) & 0xff;
			machine.screen_adapter.screen_mem =
					machine.RAM + SCREEN_MEM_START + screen_addr;
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

	DBG(printf("%s 0x%x\n",__func__, *instr));

	opcode = *instr & 0xff;

	switch(opcode) {
		case nop: DBG(printf("nop\n"));
				break;
		case halt: DBG(printf("halt\n"));
				machine.cpu_regs.panic = 1;
				break;
		case mov:
				src = mnemonic(instr, &dst, "mov");
				if (!machine.cpu_regs.exception)
					*dst = src;
				break;
		case add:
				src = mnemonic(instr, &dst, "add");
				if (!machine.cpu_regs.exception)
					*dst += src;
				break;
		case sub: DBG(printf("sub\n"));
				src = mnemonic(instr, &dst, "sub");
				if (!machine.cpu_regs.exception)
					*dst -= src;
				break;
		case jmp: DBG(printf("jmp "));
				machine.cpu_regs.pc = (*instr >> 8) - 1; /* compensate for pc++ */
				DBG(printf("0x%x\n", machine.cpu_regs.pc));
				break;
		case cmp: DBG(printf("cmp "));
				machine.cpu_regs.cr = COND_UNDEFINED;
				compare(instr);
				DBG(printf("cr: 0x%x\n", machine.cpu_regs.cr));
				break;
		case breq: DBG(printf("breq "));
				if (machine.cpu_regs.cr == COND_EQ) {// todo: make a jump func
					machine.cpu_regs.pc = (*instr >> 16);
					//machine.cpu_regs.pc--; /* compensate for pc++ */
					printf("jump to %ld \n",machine.cpu_regs.pc + 1);
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
		default: printf("illegal instruction 0x%X  pc=%ld\n",*instr,machine.cpu_regs.pc);
				machine.cpu_regs.panic = 1;
				break;
	}
	machine.cpu_regs.exception = 0;
}


void reset_cpu(void) {
	memset(&machine.RAM, 0x00, RAM_SIZE);
	memset(&machine.GP_REG, 0x00, GP_REG_MAX);

	machine.cpu_regs.pc = 0;
	machine.cpu_regs.sp = 0;
	machine.cpu_regs.exception = 0;
	machine.cpu_regs.panic = 0;
	machine.cpu_regs.cr = COND_UNDEFINED;
}

void init_screen(void) {
	machine.screen_adapter.x = 0;
	machine.screen_adapter.y = 0;
	machine.screen_adapter.c = '\0';
	machine.screen_adapter.mode = 0;

	machine.screen_adapter.screen_mem = machine.RAM + SCREEN_MEM_START;
	memset(machine.screen_adapter.screen_mem, 0x00, screen_modes[machine.screen_adapter.mode]);
	machine.screen_adapter.refresh = 0;
}

void load_program(uint32_t *prg) {
	int prg_size = prg[0];

	memcpy(&machine.RAM[4], prg, prg_size);
	/* pc0 will be program header/size */
	machine.cpu_regs.pc = 2;
}


int main(void)
{

	reset_cpu();
	init_screen();
#if UNIT_TEST
	machine.RAM[58] = 0xba;
	machine.RAM[59] = 0xAA;
	machine.RAM[60] = 0xdd;
	machine.RAM[61] = 0xEE;

	machine.RAM[SCREEN_MEM_START] = 0xff;
	machine.RAM[SCREEN_MEM_START+1] = 6;
	machine.RAM[SCREEN_MEM_START+2] = 2;
	machine.RAM[SCREEN_MEM_START+8] = 8;

	machine.GP_REG[1] = 10;
	machine.GP_REG[2] = 11;
	machine.GP_REG[3] = 12;
	machine.GP_REG[4] = 13;
	machine.GP_REG[5] = 14;
	machine.GP_REG[6] = 15;
	machine.GP_REG[7] = 16;

	load_program(program_unit_test);
#else
	machine.RAM[PROGRAM_MEM_START] = 0xdc;
	machine.RAM[PROGRAM_MEM_START + 1] = 0xba;
	machine.RAM[PROGRAM_MEM_START + 2] = 0xfe;
	machine.RAM[PROGRAM_MEM_START + 3] = 0xdc;
	machine.RAM[8192] = 0xcb;
	load_program(rom);
#endif

	while(!machine.cpu_regs.panic) {
		decode_instruction((uint32_t *)&machine.RAM[machine.cpu_regs.pc * 4]);
		if (machine.screen_adapter.refresh) {
			screen_retrace();
			usleep(100 * 1000);
		}
		machine.cpu_regs.pc++;
		if ( (machine.cpu_regs.pc * 4) >= RAM_SIZE) {
			machine.cpu_regs.panic = 1;
			printf("stray program...\n");
		}
	}

#if UNIT_TEST
	assert(machine.GP_REG[0] == 238);
	assert(machine.GP_REG[1] == 255);
	assert(machine.GP_REG[2] == 11);
	assert(machine.GP_REG[3] == 12);
	assert(machine.GP_REG[4] == 160);
	assert(machine.GP_REG[5] == 14);
	assert(machine.GP_REG[6] == 170);
	assert(machine.GP_REG[7] == 33);
	assert(machine.GP_REG[9] == 12);
	assert(machine.GP_REG[10] == 160);
	assert(machine.GP_REG[12] == 186);
	assert(machine.GP_REG[13] == 255);
	assert(machine.GP_REG[14] == 12);

	assert(machine.RAM[1] == machine.GP_REG[10]);
#endif

	return 0;
}

