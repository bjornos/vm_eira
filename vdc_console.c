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

exception_t display_put_char(struct _machine *machine)
{
	struct _vdc_regs *vdc = &machine->vdc_regs;
	char c;
	int addr;

	if (vdc->display.mode == mode_640x480)
		return EXC_NONE;

	if ((vdc->display.mode != mode_80x25) && (vdc->display.mode != mode_40x12))
		return EXC_VDC;

	if (!vdc->display.enabled)
			return EXC_DISP;

	if (vdc->curr_instr & OP_SRC_MEM) {
		if  ((vdc->curr_instr >> 16) > MEM_START_RW) {
			c = machine->RAM[(vdc->curr_instr >> 16)];
		} else {
			c = machine->RAM[ machine->cpu_regs.GP_REG [ (vdc->curr_instr >> 16)] ];
		}
	} else if (vdc->curr_instr & OP_SRC_REG) {
			c = machine->cpu_regs.GP_REG[(vdc->curr_instr >> 16) & 0xff ];
	} else {
		c = (vdc->curr_instr >> 16) & 0xff;
	}

	addr = (vdc->display.cursor_data.y * adapter_mode[vdc->display.mode].vertical) + vdc->display.cursor_data.x;

	if ((addr + MEM_START_VDC_FB) > RAM_SIZE)
		return EXC_VDC;

	*(vdc->frame_buffer + addr) = c;

	return EXC_NONE;
}
