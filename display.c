#include <string.h>
#include <time.h>
#include "display.h"
#include "memory.h"
#include "exception.h"
#include "machine.h"

struct _adapter_mode {
	display_mode name;
	uint16_t vertical;
	uint16_t horizontal;
	uint32_t resolution;
};

static const struct _adapter_mode adapter_mode[] = {
	{mode_40x12, 40, 12, 40*12},
	{mode_80x25, 80, 25, 80*25},
};


void display_retrace(struct _display_adapter *display)
{
	int cx,cy,addr;

	if (!display->frame_buffer) {
		printf("No display mem!\n");
		return;
	}

	display->refresh = 1;

	for(cy=0; cy < adapter_mode[display->mode].horizontal; cy++) {
		addr = (cy * adapter_mode[display->mode].vertical);
		for (cx=0; cx < adapter_mode[display->mode].vertical; cx++) {
			gotoxy(cx,cy);
			putchar((char)*(display->frame_buffer + addr) & 0xff);
			addr++;
		}
	}

	display->refresh = 0;
}

void display_wait_retrace(struct _display_adapter *display)
{
	while(display->refresh);
}

void display_reset(struct _display_adapter *display)
{
	display->enabled = 0;
}

int display_set_mode(struct _display_adapter *display, uint8_t *frame_buffer, display_mode this_mode)
{
	display->x = 0;
	display->y = 0;
	display->c = '\0';

	if (this_mode < mode_unknown)
		display->mode = this_mode;
	else
		return EXC_DISP;

	memset(display->frame_buffer,' ', adapter_mode[display->mode].resolution);
	display->refresh = 0;
	display->enabled = 1;

	return EXC_NONE;
}

int display_request(struct _display_adapter *display, uint32_t *instr,
	uint8_t *frame_buffer, int request)
{
	int addr = 0;
	int ret = EXC_NONE;

	if (!display->enabled && (request != DISPLAY_INIT))
		return EXC_DISP;

	display_wait_retrace(display);

	switch(request) {
		case DISPLAY_INIT:
			ret =
				display_set_mode(display, frame_buffer, (*instr >> 8));
			break;
		case DISPLAY_SETXY:
			display->x = (*instr >> 8) & 0xfff;
			display->y = (*instr >> 20) & 0xfff;
			break;
		case DISPLAY_SETC:
			addr = (display->y * adapter_mode[display->mode].vertical) + display->x;
			display->c = (*instr >> 8) & 0xff;
			*(display->frame_buffer + addr) = display->c;
			break;
		case DISPLAY_CLR:
			memset(&display->frame_buffer[0], ' ', adapter_mode[display->mode].resolution);
			break;
		default:
			ret = EXC_DISP;
	}

	return ret;
}


void *display_machine(void *mach)
{
	struct _machine *machine = mach;
	struct timespec frame_rate;

	frame_rate.tv_nsec = 1000000000 / DISPLAY_FRAME_RATE;
	frame_rate.tv_sec = 0;

	cursor_off();

	while(!machine->cpu_regs.panic) {
		if (machine->display.enabled)
			display_retrace(&machine->display);
		nanosleep(&frame_rate, NULL);
	}

	cursor_on();

	pthread_exit(NULL);
}

