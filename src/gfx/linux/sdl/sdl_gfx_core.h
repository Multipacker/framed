#ifndef SDL_GFX_CORE_H
#define SDL_GFX_CORE_H

#include <SDL2/SDL.h>

typedef struct Gfx_SDL_Context Gfx_SDL_Context;
struct Gfx_SDL_Context
{
    B32 is_fullscreen;
    SDL_Window *window;
    SDL_GLContext gl_context;
    SDL_Cursor *cursors[Gfx_Cursor_COUNT];
};

internal Gfx_SDL_Context sdl_init(U32 x, U32 y, U32 width, U32 height, Str8 title);
internal Void            sdl_show_window(Gfx_SDL_Context *gfx);
internal Gfx_EventList   sdl_get_events(Arena *arena, Gfx_SDL_Context *gfx);
internal Vec2F32         sdl_get_mouse_pos(Gfx_SDL_Context *gfx);
internal Vec2U32         sdl_get_window_area(Gfx_SDL_Context *gfx);
internal Vec2U32         sdl_get_window_client_area(Gfx_SDL_Context *gfx);
internal Void            sdl_toggle_fullscreen(Gfx_SDL_Context *gfx);
internal Void            sdl_swap_buffers(Gfx_SDL_Context *gfx);
internal Vec2F32         sdl_get_dpi(Gfx_SDL_Context *ctx);
internal Void            sdl_set_cursor(Gfx_SDL_Context *ctx, Gfx_Cursor cursor);
internal Void            sdl_set_window_maximized(Gfx_SDL_Context *ctx);
internal Gfx_Monitor     sdl_monitor_from_window(Gfx_SDL_Context *ctx);
internal Vec2F32         sdl_dim_from_monitor(Gfx_Monitor monitor);
internal Vec2F32         sdl_scale_from_window(Gfx_SDL_Context *gfx);

internal Void sdl_set_clipboard(Str8 data);
internal Str8 sdl_push_clipboard(Arena *arena);

#endif // SDL_GFX_CORE_H
