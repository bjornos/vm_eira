#include "vdc.h"
#include "machine.h"


exception_t display_retrace_mode_console(struct _vdc_regs *vdc);

void display_clear_mode_console(struct _vdc_regs *vdc);

exception_t display_put_char(struct _machine *machine);
