#include "vdc_vga.h"

extern const struct _adapter_mode const adapter_mode[];

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
void putpixel(SDL_Surface *surface, int x, int y, uint32_t pixel)
{
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    uint8_t *p = (uint8_t *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(uint16_t *)p = pixel;
        break;

    case 3:
        if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(uint32_t *)p = pixel;
        break;
    }
}

exception_t display_put_pixel(struct _machine *machine)
{
	struct _vdc_regs *vdc = &machine->vdc_regs;

	if (vdc->display.mode != mode_640x480)
		return EXC_VDC;

	uint32_t yellow = SDL_MapRGB(vdc->display.screen_surface->format, 0xff, 0xff, 0x00);

	int x = vdc->display.cursor_data.x;
	int y = vdc->display.cursor_data.y;

	/* Lock the screen for direct access to the pixels */
	if (SDL_LockSurface(vdc->display.screen_surface) < 0 ) {
		fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
		return EXC_VDC;
	}

	putpixel(vdc->display.screen_surface, x, y, yellow);

	SDL_UnlockSurface(vdc->display.screen_surface);

	return EXC_NONE;
}

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

exception_t display_retrace_mode_vga(struct _vdc_regs *vdc)
{
	int cx,cy;
	uint32_t color = SDL_MapRGB(vdc->display.screen_surface->format, 0xff, 0xff, 0x00);
	uint16_t px,py;

	if (!vdc->display.enabled)
		return EXC_VDC;;

	if (!vdc->frame_buffer) {
		fprintf(stderr, "No display mem!\n");
		return EXC_VDC;
	}

	vdc->display.refresh = 1;

    /* Lock the screen for direct access to the pixels */
	if (SDL_LockSurface(vdc->display.screen_surface) < 0 ) {
		fprintf(stderr, "Can't lock screen: %s\n", SDL_GetError());
		return EXC_VDC;
	}

	for(cy=0; cy < adapter_mode[vdc->display.mode].horizontal; cy++) {
		int addr = (cy * adapter_mode[vdc->display.mode].vertical);
		for (cx=0; cx < adapter_mode[vdc->display.mode].vertical; cx++) {
			if (*(vdc->frame_buffer + addr) != 0x00) {
				color = SDL_MapRGB(vdc->display.screen_surface->format, 0xff, 0xff, 0xff);
				px = cx;
				py = cy;
				putpixel(vdc->display.screen_surface, px, py, color);
			}
			addr++;
		}
	}

	SDL_UnlockSurface(vdc->display.screen_surface);

	vdc->display.refresh = 0;

	SDL_UpdateWindowSurface(vdc->display.screen);

	return EXC_NONE;
}

void display_clear_mode_vga(struct _vdc_regs *vdc)
{
	SDL_FillRect(vdc->display.screen_surface, NULL,
		SDL_MapRGB(vdc->display.screen_surface->format, 0x00, 0x00, 0x00));

	memset(&vdc->frame_buffer[0], 0x00, adapter_mode[vdc->display.mode].resolution);
}
