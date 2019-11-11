#include "vdc_console.h"

extern const struct _adapter_mode const adapter_mode[];

exception_t display_retrace_mode_console(struct _vdc_regs *vdc)
{
	int cx,cy;

	if (!vdc->display.enabled)
		return EXC_VDC;

	if (!vdc->frame_buffer) {
		fprintf(stderr, "No display mem!\n");
		return EXC_VDC;
	}

	vdc->display.refresh = 1;

	for(cy=0; cy < adapter_mode[vdc->display.mode].horizontal; cy++) {
		int addr = (cy * adapter_mode[vdc->display.mode].vertical);
		for (cx=0; cx < adapter_mode[vdc->display.mode].vertical; cx++) {
			vdc_gotoxy(cx,cy);
			putchar((char)*(vdc->frame_buffer + addr) & 0xff);
			addr++;
		}
	}

	vdc->display.refresh = 0;

	return EXC_NONE;
}

void display_clear_mode_console(struct _vdc_regs *vdc)
{
	if (!vdc->display.enabled) {
			vdc->exception = EXC_DISP;
			return;
	}

	memset(&vdc->frame_buffer[0], ' ', adapter_mode[vdc->display.mode].resolution);
}
