#ifndef LINUX_GFX_CORE_H
#define LINUX_GFX_CORE_H

#include <SDL2/SDL.h>
#include "glad/glad.c"

struct Gfx_Context
{
	B32 is_fullscreen;
	SDL_Window *window;
	SDL_GLContext gl_context;
};

#endif // LINUX_GFX_CORE_H
