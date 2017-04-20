#include <string.h>
#include <time.h>
#include "display.h"
#include "memory.h"
#include "opcodes.h"

struct _adapter_mode {
	display_mode name;
	uint16_t vertical;
	uint16_t horizontal;
	uint32_t resolution;
};

static const struct _adapter_mode adapter_mode[] = {
	{mode_40x12, 40, 12, 40*12},
	{mode_80x25, 80, 25, 80*25},
	{mode_320x240, 320, 240, 320*240},
};


void display_retrace(struct _display_adapter *display)
{
	int cx,cy,addr;

	for(cy=0; cy < adapter_mode[display->mode].horizontal; cy++) {
		addr = (cy * adapter_mode[display->mode].vertical);
		for (cx=0; cx < adapter_mode[display->mode].vertical; cx++) {
			gotoxy(cx,cy);
			printf("%c\n",(char)*(display->mem + addr) & 0xff);
			addr++;
		}
	}
	display->refresh = 0;
;
}

int display_request(struct _display_adapter *display, uint32_t *instr,
		int request)
{
	int addr = 0;
	int ret = EXC_NONE;

	switch(request) {
		case display_setxy:
			display->x = (*instr >> 8) & 0xfff;
			display->y = (*instr >> 20) & 0xfff;
			break;
		case display_setc:
			addr = (display->y * adapter_mode[display->mode].vertical) + display->x;
			display->c = (*instr >> 8) & 0xff;
			*(display->mem + addr) = display->c;
			display->refresh = 1;
			break;
		case display_clr:
			memset(&display->mem[0], ' ', adapter_mode[display->mode].resolution);
			display->refresh = 1;
			break;
		default:
			ret = EXC_DISP;
	}

	return ret;
}

void display_init(struct _display_adapter *display, uint8_t *machine_ram, display_mode this_mode)
{
	display->x = 0;
	display->y = 0;
	display->c = '\0';
	display->mode = this_mode;

	display->mem = machine_ram + MEM_START_DISPLAY;
	memset(display->mem, 0x00, adapter_mode[display->mode].resolution);
	display->refresh = 0;

}
