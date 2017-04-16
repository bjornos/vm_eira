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
#include <argp.h>
#include <signal.h>
#include <unistd.h>

#include "opcodes.h"
#include "memory.h"
#include "prg.h"
#include "rom.h"
#include "testprogram.h"


typedef struct {
	char *instr;
	int val;
} t_opcode;

static t_opcode instr_table[] = {
		{ "halt", halt },
		{ "nop", nop },
		{ "add", add },
		{ "sub", sub },
		{ "mov", mov },
};

#define NUM_INSTRUCTIONS (sizeof(instr_table) / sizeof(t_opcode))

int which_instr(char *opcode)
{
	int i;
	t_opcode *sym;

	for (i=0; i < NUM_INSTRUCTIONS; i++) {
		sym = instr_table + i;
		if (strcmp(sym->instr, opcode) == 0)
			return sym->val;
	}

	return -1;
}

typedef struct  {
	int instr;
	int args;
} machine_code;


uint32_t encode_instr(const char line[])
{
	int c;
	char instr[16], a1[16],a2[16];
	uint32_t arg1,arg2;
	int src,dst;
	machine_code code;
	uint32_t mnemonic = 0;

	int dst_reg, dst_mem;
	int src_reg, src_mem;

	// parse opcode
	c = 0;
	while ((line[c] !=' ') && (line[c] != ';')) {
		instr[c] = line[c];
		c++;
	}
	instr[c] = '\0';

	code.instr = which_instr(instr);
	code.args = 0;

	switch(code.instr) {
		case mov: printf("decode as move\n");
				code.args = 2;
			break;
		case halt:
				printf("decoded as halt\n");
				code.args = 0;
			break;
		default:
			printf("unknown instruction %s\n",instr);
			return -1;
		break;
	}

	src_reg = src_mem = dst_reg = dst_mem = 0;

	if (code.args > 0) {
		int c2 = 0;

		while ((line[c] ==',') || (line[c] == ';') || (line[c] == ' '))
			c++;

		while ((line[c] !=',') && (line[c] != ';') && (line[c] != ' ') && (c < 80)) {
			a1[c2] = line[c];
			c++;
			c2++;
		}
		if (c > 79) {
			printf("too long arg1.\n");
			return -1;
		}
		a1[c2] = '\0';
		printf("arg1: %s\n", a1);

		if (a1[0] == 'r') {
			dst_reg = 1;
			arg1 = atoi(&a1[1]);
			printf("src = reg  %d\n",arg1);
		} else if (a1[0] == '@') {
			dst_mem = 1;
			arg1 = atoi(&a1[1]);
			printf("src = mem @ %d\n",arg1);
		} else
			printf("dst = ???\n");

	}


	if (code.args > 1) {
		int c3 = 0;

		while ((line[c] ==',') || (line[c] == ';') || (line[c] == ' '))
			c++;

		while ((line[c] !=',') && (line[c] != ';') && (line[c] != ' ') && (line[c] != '\0') && (c < 80)) {
			a2[c3] = line[c];
			c++;
			c3++;
		}
		if ((c > 79) || (line[c] == '\0')) {
			printf("too long arg2 (%d).\n",c);
			return -1;
		}
		a2[c3] = '\0';
		printf("arg2: %s\n", a2);

		if (a2[0] == 'r') {
			src_reg = 1;
			arg2 = atoi(&a2[1]);
			printf("src = reg  %d\n",arg2);
		} else if (a2[0] == '@') {
			src_mem = 1;
			arg2 = atoi(&a2[1]);
			printf("src = mem @ %d\n",arg2);
		} else {
			arg2 = atoi(&a2[0]);
			printf("src =value - %d\n",arg2);
		}
	}


	mnemonic = code.instr;
	if (code.args) {
		if (code.args == 1)
			mnemonic |= (arg2 << 16);
		else {
			if (dst_reg)
				mnemonic |= OP_DST_REG;
			if (src_reg)
				mnemonic |= OP_SRC_REG;
			if (src_mem)
				mnemonic |= OP_SRC_MEM;
			if (dst_mem)
				mnemonic |= OP_DST_MEM;
			if (dst_mem)
				mnemonic |= (arg2 << 8) | (arg1 << 16);
			else
				mnemonic |= (arg1 << 8) | (arg2 << 16);

		}
	}

	return mnemonic;
}


int main(int argc,char *argv[])
{
	char const* const filename = argv[1];
	FILE *prg = fopen(filename, "r");
	char line[0xff];
	char instr[0xff];
	int abort = 0;
	uint32_t enc[255];
	int e;
	struct _prg_format program;

	e = 0;

	while (fgets(line, sizeof(line), prg) && !abort) {
		printf("%s", line);
		enc[e] = encode_instr(line);
		if (enc[e] == -1)
			abort = 1;
		else
			printf("0x%x\n", enc[e]);
		e++;
	}
	fclose(prg);

	program.header.code_size = e * sizeof(uint32_t);
	program.header.magic = PRG_MAGIC_HEADER;
	program.header.reserved1 = 0;
	program.header.reserved2 = 0;
	printf("code size = %d\n",program.header.code_size);
	program.code_segment = enc;

	printf("code: 0x%x  0x%x\n",*program.code_segment,enc[0]);
	printf("code: 0x%x  0x%x\n",*(program.code_segment + 1),enc[1]);
	prg = fopen("sample.bin", "w+");
	fwrite(&program.header, sizeof(struct _prg_header), 1, prg);
	fwrite(&enc, e * sizeof(uint32_t), 1, prg);
	fclose(prg);

#if 0
	FILE *fd;
	fd = fopen("bin/eira_rom.bin","wb");
	fwrite(rom, sizeof(rom),1,fd);
	fclose(fd);
	fd = fopen("bin/eira_test.bin","wb");
	fwrite(program_regression_test, sizeof(program_regression_test),1,fd);
	fclose(fd);
#endif

	return 0;
}
