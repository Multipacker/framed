#ifndef LINUX_GFX_CORE_H
#define LINUX_GFX_CORE_H

#include "sdl/sdl_gfx_core.h"
#include "wayland/wayland_gfx_core.h"

typedef enum Gfx_Linux_ContextKind Gfx_Linux_ContextKind;
enum Gfx_Linux_ContextKind
{
    Gfx_Linux_ContextKind_Wayland,
    Gfx_Linux_ContextKind_SDL,
};

struct Gfx_Context
{
    Gfx_Linux_ContextKind kind;
    union {
        Gfx_Wayland_Context wayland;
        Gfx_SDL_Context     sdl;
    };
};

#endif // LINUX_GFX_CORE_H
