#include "vdc.h"
#include "machine.h"

exception_t display_put_pixel(struct _machine *machine);

exception_t display_init_vga(struct _display_adapter *disp, display_mode *mode);

exception_t display_retrace_mode_vga(struct _vdc_regs *vdc);

void display_clear_mode_vga(struct _vdc_regs *vdc);

