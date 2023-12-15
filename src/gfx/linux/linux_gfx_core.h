#ifndef LINUX_GFX_CORE_H
#define LINUX_GFX_CORE_H

#include <SDL2/SDL.h>

struct Gfx_Context
{
	B32 is_fullscreen;
	SDL_Window *window;
};

#endif // LINUX_GFX_CORE_H
