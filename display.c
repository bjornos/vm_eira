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


static void display_retrace(struct _display_adapter *display)
{
	int cx,cy,addr;

	if (!display->frame_buffer) {
		fprintf(stderr, "No display mem!\n");
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

static void display_wait_retrace(struct _display_adapter *display)
{
	while(display->refresh);
}

static int display_set_mode(struct _display_adapter *display, uint8_t *frame_buffer, display_mode this_mode)
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

void display_reset(void *mach)
{
	struct _machine *machine = mach;

	machine->display.enabled = 0;
	machine->display.refresh = 0;
}

int display_request(void *mach, int request)

{
	struct _machine *machine = mach;
	int addr = 0;
	int ret = EXC_NONE;
	uint32_t *instr;

	if ((machine == NULL) || (&machine->display == NULL))
		return EXC_DISP;

	if (!machine->display.enabled && (request != DISPLAY_INIT))
		return EXC_DISP;

	instr = (uint32_t *)&machine->RAM[machine->cpu_regs.pc];

	switch(request) {
		case DISPLAY_WAIT_RETRACE:
			display_wait_retrace(&machine->display);
		break;
		case DISPLAY_INIT:
			ret =
				display_set_mode(&machine->display, machine->display.frame_buffer, (*instr >> 8));
		break;
		case DISPLAY_SETXY:
			machine->display.x = machine->cpu_regs.GP_REG[ (*instr >> 16) & 0xff ];
			machine->display.y = machine->cpu_regs.GP_REG[ (*instr >> 24) & 0xff ];
			break;
		case DISPLAY_SETC: {
			char c;
			if (*instr & OP_SRC_MEM) {
				/* above */
				if  ((*instr >> 16) > MEM_START_RW) {
					c = machine->RAM[(*instr >> 16)];
				} else {
					c = machine->RAM[ machine->cpu_regs.GP_REG [ (*instr >> 16)] ]; /* fixme: clamp at reg max */
				}
			} else if (*instr & OP_SRC_REG) {
				c = machine->cpu_regs.GP_REG[(*instr >> 16) & 0xff ];
			} else
				c = (*instr >> 16) & 0xff;
			addr = (machine->display.y * adapter_mode[machine->display.mode].vertical) + machine->display.x;
			machine->display.c = c;
			*(machine->display.frame_buffer + addr) = machine->display.c;
		}
		break;
		case DISPLAY_CLR:
			memset(&machine->display.frame_buffer[0], ' ', adapter_mode[machine->display.mode].resolution);
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

