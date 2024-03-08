#ifndef WAYLAND_GFX_CORE_H
#define WAYLAND_GFX_CORE_H

#undef global
#include <wayland-client.h>
#include "wayland_xdg_shell.generated.h"
#define global static

typedef struct Gfx_Wayland_Context Gfx_Wayland_Context;
struct Gfx_Wayland_Context
{
    // Globals
    struct wl_display    *display;
    struct wl_registry   *registry;
    struct wl_compositor *compositor;
    struct wl_shm        *shm;
    struct xdg_wm_base   *xdg_wm_base;

    // Per window
    struct wl_surface   *surface;
    struct xdg_surface  *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;
};

#endif // WAYLAND_GFX_CORE_H
