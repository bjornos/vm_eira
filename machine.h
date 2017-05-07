#ifndef __MACHINE_H__
#define __MACHINE_H__

#include "display.h"
#include "memory.h"
#include "utils.h"
#include "cpu.h"

struct _machine {
	uint8_t RAM[RAM_SIZE];
	struct _cpu_regs cpu_regs;
	struct _display_adapter display;
	struct _dbg dbg_info[DBG_HISTORY];
};

#endif /* __MACHINE_H__ */
