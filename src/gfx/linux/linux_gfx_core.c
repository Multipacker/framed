#include "sdl/sdl_gfx_core.c"
#include "wayland/wayland_gfx_core.c"

internal Gfx_Context
gfx_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    Gfx_Context result = { 0 };

    result.kind = Gfx_Linux_ContextKind_SDL;
    result.sdl  = sdl_init(x, y, width, height, title);
    //result.kind     = Gfx_Linux_ContextKind_Wayland;
    //result.wayland  = wayland_init(x, y, width, height, title);
    wayland_init(x, y, width, height, title);

    return(result);
}

internal Void
gfx_show_window(Gfx_Context *gfx)
{
    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_show_window(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        wayland_show_window(&gfx->wayland);
    }
}

internal Gfx_EventList
gfx_get_events(Arena *arena, Gfx_Context *gfx)
{
    Gfx_EventList result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_get_events(arena, &gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_get_events(arena, &gfx->wayland);
    }

    return(result);
}

internal Vec2F32
gfx_get_mouse_pos(Gfx_Context *gfx)
{
    Vec2F32 result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_get_mouse_pos(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_get_mouse_pos(&gfx->wayland);
    }

    return(result);
}

internal Vec2U32
gfx_get_window_area(Gfx_Context *gfx)
{
    Vec2U32 result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_get_window_area(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_get_window_area(&gfx->wayland);
    }

    return(result);
}

internal Vec2U32
gfx_get_window_client_area(Gfx_Context *gfx)
{
    Vec2U32 result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_get_window_client_area(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_get_window_client_area(&gfx->wayland);
    }

    return(result);
}

internal Void
gfx_toggle_fullscreen(Gfx_Context *gfx)
{
    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_toggle_fullscreen(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        wayland_toggle_fullscreen(&gfx->wayland);
    }
}

internal Void
gfx_swap_buffers(Gfx_Context *gfx)
{
    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_swap_buffers(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        wayland_swap_buffers(&gfx->wayland);
    }
}

internal Vec2F32
gfx_get_dpi(Gfx_Context *gfx)
{
    Vec2F32 result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_get_dpi(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_get_dpi(&gfx->wayland);
    }

    return(result);
}

internal Void
gfx_set_cursor(Gfx_Context *gfx, Gfx_Cursor cursor)
{
    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_set_cursor(&gfx->sdl, cursor);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        wayland_set_cursor(&gfx->wayland, cursor);
    }
}

internal Void
gfx_set_window_maximized(Gfx_Context *gfx)
{
    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_set_window_maximized(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        wayland_set_window_maximized(&gfx->wayland);
    }
}

internal Gfx_Monitor
gfx_monitor_from_window(Gfx_Context *gfx)
{
    Gfx_Monitor result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_monitor_from_window(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_monitor_from_window(&gfx->wayland);
    }

    return(result);
}

internal Vec2F32
gfx_dim_from_monitor(Gfx_Monitor monitor)
{
    Vec2F32 result = { 0 };

    //if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_dim_from_monitor(monitor);
    }
    //else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        //result = wayland_dim_from_monitor(monitor);
    }

    return(result);
}

internal Void
gfx_set_clipboard(Str8 data)
{
    //if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        sdl_set_clipboard(data);
    }
    //else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        //wayland_set_clipboard(data);
    }
}

internal Str8
gfx_push_clipboard(Arena *arena)
{
    Str8 result = { 0 };

    //if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_push_clipboard(arena);
    }
    //else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        //result = wayland_push_clipboard(arena);
    }

    return(result);
}

internal Vec2F32
gfx_scale_from_window(Gfx_Context *gfx)
{
    Vec2F32 result = { 0 };

    if (gfx->kind == Gfx_Linux_ContextKind_SDL)
    {
        result = sdl_scale_from_window(&gfx->sdl);
    }
    else if (gfx->kind == Gfx_Linux_ContextKind_Wayland)
    {
        result = wayland_scale_from_window(&gfx->wayland);
    }

    return(result);
}
