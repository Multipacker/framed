#ifndef WAYLAND_GFX_CORE_H
#define WAYLAND_GFX_CORE_H

#undef global
#include <wayland-client.h>
#define global static

typedef struct Gfx_Wayland_Context Gfx_Wayland_Context;
struct Gfx_Wayland_Context
{
    // Globals
    struct wl_display    *display;
    struct wl_registry   *registry;
    struct wl_compositor *compositor;
    struct wl_shm        *shm;

    // Per window
    struct wl_surface    *surface;
};

#endif // WAYLAND_GFX_CORE_H
