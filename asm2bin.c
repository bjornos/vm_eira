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

enum {
	DST_MEM,
	DST_REG,
	SRC_REG,
	SRC_MEM,
};

typedef struct  {
	int instr;
	int args;
} machine_code;

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

#define DBG(x) x

#define NUM_INSTRUCTIONS (sizeof(instr_table) / sizeof(t_opcode))

#define OPCODE_LEN_MAX	16

#define ARG_LEN_MAX 16

#define LINE_LENGTH_MAX 64


const char *char_type_instruction = "abcdefghijklmnopqrstuvwxyz";

const char *char_type_argument = "1234567890rRxX@";

const char *char_type_spaces = " \t\n";

const char char_type_argstop[] = ",;\t\n ";


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

static __inline__ int is_comment(const char *c)
{
	return (*c == ';');
}

int char_is_type(const char *c, const char *match_type)
{
	for (int d = 0; d < strlen(match_type); d++) {
			DBG(printf("**found %c \n",*(match_type + d)));
		if (*c == *(match_type + d)) {
			DBG(printf("found %c (%d) \n",*(match_type + d), *(match_type + d)));
			return 1;
		}
	}

	return 0;
}

static __inline__ void get_argument(char **c, char *arg, int line, int *col)
{
	int pos = 0;

	while (char_is_type(*c, char_type_argument) && (pos < ARG_LEN_MAX)) {
		*(arg + pos) = *(*c);
		*(*c)++;
		if (*col > LINE_LENGTH_MAX) {
			printf("%s: Line %d: To long (> %d chars).", __func__, line, *col);
			exit(EXIT_FAILURE);
		}
		(*col)++;
		pos++;
	}
	*(arg + pos) = '\0';
}

static __inline__ void skip_spaces(char **c, int line, int *col)
{
	while (char_is_type(*c, char_type_spaces)) {

		*(*c)++;
		if (*col > LINE_LENGTH_MAX) {
			printf("%s: Line %d: To long (> %d chars).", __func__, line, *col);
			exit(EXIT_FAILURE);
		}
		(*col)++;
	}
}

static __inline__ uint32_t decode_mov(uint32_t mnemonic, char *c, int line, int *col)
{
	char arg1[16];
	char arg2[16];
	int reg_src, mem_src, reg_dst, mem_dst;

	DBG(printf("decode mov\n"));

	skip_spaces(&c, line, col);

	get_argument(&c, arg1, line, col);
	skip_spaces(&c, line, col);

	DBG(printf("arg1: %s \n", arg1));

	switch (arg1[0]) {
		case 'R':
		case 'r':
			reg_dst = atoi(&arg1[1]);
			DBG(printf("regdst: %d\n", reg_dst));
			if (reg_dst < 0 || reg_dst > GP_REG_MAX) {
				printf("Error (%d %d): register %s out of bounds.\n", line, *col, arg1);
				return OPCODE_ENCODE_ERROR;
			}
			mnemonic |= OP_DST_REG;
			mnemonic |= (reg_dst << 8);
			break;
		case '@':
			mem_dst = atoi(&arg1[1]);
			DBG(printf("memdst: %d\n", mem_dst));
			if ((mem_dst < MEM_START_RW) || (mem_dst > RAM_SIZE)) {
				printf("Error (%d %d): address %s out of bounds.\n", line, *col, arg1);
				return OPCODE_ENCODE_ERROR;
			}
			mnemonic |= OP_DST_MEM;
			mnemonic |= (mem_dst << 16);
			break;
	}

	if (*c != ',') {
		printf("Syntax Error at row %d column %d): Exptected ',' - Don't know what to do with %c\n", line, *col, *c);
		exit(EXIT_FAILURE);
	} else
		*c++;

	skip_spaces(&c, line, col);

	get_argument(&c, arg2, line, col);

	DBG(printf("arg2: %s\n", arg2));

	switch (arg2[0]) {
		case 'R':
		case 'r':
			reg_src = atoi(&arg2[1]);
			DBG(printf("regsrc: %d\n", reg_src));
			if (reg_src < 0 || reg_src > 15) { // GP_REG_MAX
				printf("Error (%d %d): register %s out of bounds.\n", line, *col, arg2);
				return OPCODE_ENCODE_ERROR;
			}
			mnemonic |= OP_SRC_REG;
			if (mnemonic & OP_DST_REG)
				mnemonic |= (reg_src << 16);
			else
				mnemonic |= (reg_src << 8);
			break;
		case '@':
			mem_src = atoi(&arg2[1]);
			DBG(printf("memsrc: %d\n", mem_src));
			if ((mem_src < 0) || (mem_src > 65000)) {  // RAM_SIZE
				printf("Error (%d %d): address %s out of bounds.\n", line, *col, arg2);
				return OPCODE_ENCODE_ERROR;
			}
			mnemonic |= OP_SRC_MEM;
			mnemonic |= (mem_src << 16);
			break;
		case  '0':
		case  '1':
		case  '2':
		case  '3':
		case  '4':
		case  '5':
		case  '6':
		case  '7':
		case  '8':
		case  '9':
			if (!(mnemonic & OP_DST_MEM)) {
				reg_src = atoi(&arg2[0]);
				DBG(printf("immval: %d\n", reg_src));
				mnemonic |= (reg_src << 16);
				break;
			}
		default:
				printf("Error (%d %d): Don't know what to do with %s\n", line, *col, arg2);
				mnemonic = OPCODE_ENCODE_ERROR;

	}


	return mnemonic;
}

uint32_t encode_instr(char *code_line, int line_nbr)
{
	machine_code code;
	uint32_t mnemonic = 0;
	char *c;
	char instr[OPCODE_LEN_MAX];
	int pos = 0;
	int col = 0;

	c = code_line;

	if (is_comment(c)) {
		DBG(printf("decode as comment.\n"));
		return OPCODE_ENCODE_COMMENT;
	}

	while (char_is_type(c, char_type_instruction) && (pos < OPCODE_LEN_MAX)) {
		instr[pos++] = *c++;
		col++;
	}
	instr[pos] = '\0';

	code.instr = which_instr(instr);
	code.args = 0;

	printf("line = %d\n", line_nbr);

	switch(code.instr) {
		case mov: mnemonic = decode_mov(code.instr, c, line_nbr, &pos);
			break;
		case halt:
				DBG(printf("decoded as halt\n"));
				mnemonic = halt;
			break;
		default:
			printf("%s: unknown instruction %s @ line %d\n",__func__, instr, line_nbr);
			return OPCODE_ENCODE_ERROR;
		break;
	}

	return mnemonic;
}

int main(int argc,char *argv[])
{
	char const* const filename = argv[1];
	FILE *prg = fopen(filename, "r");
	char line[0xff];
	int abort = 0;
	uint32_t enc[255];
	int e;
	unsigned int line_nbr;
	struct _prg_format program;

	e = line_nbr = 0;

	while (fgets(line, sizeof(line), prg) && !abort) {
		printf("%s", line);
		enc[e] = encode_instr(line, line_nbr++);
		if (enc[e] == OPCODE_ENCODE_ERROR) {
			abort = 1;
		}
		else if (enc[e] != OPCODE_ENCODE_COMMENT) {
			DBG(printf("0x%x\n", enc[e]));
			e++;
		}
	}
	fclose(prg);

	if (abort)
		return EXIT_FAILURE;

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
