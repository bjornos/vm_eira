#include <string.h>
#include <time.h>
#include "display.h"
#include "memory.h"
#include "opcodes.h"


void display_retrace(struct _display_adapter *display, uint8_t machine_ram[])
{
	int display_addr;

	gotoxy(display->x, display->y);

	display->mem = machine_ram + MEM_START_DISPLAY;
	display_addr =	/* FIXME: respect display mode */
		(display->y * 40) + display->x;
	display->mem = machine_ram + MEM_START_DISPLAY + display_addr;

	printf("%c\n",(char)*display->mem & 0xff); /* todo: why is \n needed? */

	nanosleep((const struct timespec[]){{0, 100000000L}}, NULL); /* sleep 100ms */
	display->refresh = 0;
}

int display_request(struct _display_adapter *display, uint32_t *instr,
		int request, uint8_t machine_ram[])
{
	int display_addr = 0;
	int ret = EXC_NONE;

	switch(request) {
		case display_setxy:
			display->x = (*instr >> 8) & 0xfff;
			display->y = (*instr >> 20) & 0xfff;
			break;
		case display_setc:
			display_addr =	/* FIXME: display mode */
				(display->y * 40) + display->x;
			display->c = (*instr >> 8) & 0xff;
			display->mem =
				machine_ram + MEM_START_DISPLAY + display_addr;
			*display->mem = display->c;
			display->refresh = 1;
			break;
		case display_clr:
			display->mem = machine_ram + MEM_START_DISPLAY;
			memset(display->mem, 0x00, 40 * 12);
			display_clear();
			break;
		default:
			ret = EXC_DISP;
	}

	return ret;
}

void display_init(struct _display_adapter *display, uint8_t machine_ram[])
{
	display->x = 0;
	display->y = 0;
	display->c = '\0';
	display->mode = 0;

	display->mem = machine_ram + MEM_START_DISPLAY;
	memset(display->mem, 0x00, 40 * 12);
	display->refresh = 0;
}
