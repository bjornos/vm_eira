#include "vdc.h"

exception_t display_init_vga(struct _display_adapter *disp, display_mode *mode);

exception_t display_retrace_mode_vga(struct _vdc_regs *vdc);

void display_clear_mode_vga(struct _vdc_regs *vdc);

