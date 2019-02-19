#include "vdc_vga.h"

extern const struct _adapter_mode const adapter_mode[];

exception_t display_init_vga(struct _display_adapter *disp, display_mode *mode)
{
	if (SDL_WasInit(SDL_INIT_EVERYTHING) & SDL_INIT_VIDEO) {
		/* already initialized */
		return EXC_NONE;
	}

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("error: %s\n", SDL_GetError());
		return EXC_VDC;
	}

	disp->screen = SDL_CreateWindow("vm_eira",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			adapter_mode[*mode].vertical,
			adapter_mode[*mode].horizontal,
			SDL_WINDOW_SHOWN);

	if (disp->screen == NULL) {
			printf("err:%s\n", SDL_GetError());
			return EXC_VDC;
	}

	disp->screen_surface = SDL_GetWindowSurface(disp->screen);
	if (disp->screen_surface == NULL) {
		printf("err:%s\n", SDL_GetError());
		return EXC_VDC;
	}

	return EXC_NONE;
}

void display_retrace_mode_vga(struct _vdc_regs *vdc)
{
	SDL_UpdateWindowSurface(vdc->display.screen);
}

void display_clear_mode_vga(struct _vdc_regs *vdc)
{
	SDL_FillRect(vdc->display.screen_surface, NULL,
		SDL_MapRGB(vdc->display.screen_surface->format, 0x00, 0x00, 0x00));
}
