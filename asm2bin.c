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


int main(int argc,char *argv[])
{
/*	char const* const filename = argv[1];
	FILE *prg = fopen(filename, "r");
	char line[0xff];

	while (fgets(line, sizeof(line), prg)) {
		printf("%s", line);
	}
	fclose(prg);
*/
	FILE *fd;
	fd = fopen("bin/eira_rom.bin","wb");
	fwrite(rom, sizeof(rom),1,fd);
	fclose(fd);
	fd = fopen("bin/eira_test.bin","wb");
	fwrite(program_regression_test, sizeof(program_regression_test),1,fd);
	fclose(fd);

	return 0;
}
