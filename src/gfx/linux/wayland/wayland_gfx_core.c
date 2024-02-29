internal Gfx_Wayland_Context
wayland_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    Gfx_Wayland_Context result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Void
wayland_show_window(Gfx_Wayland_Context *gfx)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Gfx_EventList
wayland_get_events(Arena *arena, Gfx_Wayland_Context *gfx)
{
    Gfx_EventList result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Vec2F32
wayland_get_mouse_pos(Gfx_Wayland_Context *gfx)
{
    Vec2F32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Vec2U32
wayland_get_window_area(Gfx_Wayland_Context *gfx)
{
    Vec2U32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Vec2U32
wayland_get_window_client_area(Gfx_Wayland_Context *gfx)
{
    Vec2U32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Void
wayland_toggle_fullscreen(Gfx_Wayland_Context *gfx)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Void
wayland_swap_buffers(Gfx_Wayland_Context *gfx)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Vec2F32
wayland_get_dpi(Gfx_Wayland_Context *gfx)
{
    Vec2F32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Void
wayland_set_cursor(Gfx_Wayland_Context *gfx, Gfx_Cursor cursor)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Void
wayland_set_window_maximized(Gfx_Wayland_Context *gfx)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Gfx_Monitor
wayland_monitor_from_window(Gfx_Wayland_Context *gfx)
{
    Gfx_Monitor result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Vec2F32
wayland_dim_from_monitor(Gfx_Monitor monitor)
{
    Vec2F32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Void
wayland_set_clipboard(Str8 data)
{
    log_error("%s is not yet implemented for Wayland", __func__);
}

internal Str8
wayland_push_clipboard(Arena *arena)
{
    Str8 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}

internal Vec2F32
wayland_scale_from_window(Gfx_Wayland_Context *gfx)
{
    Vec2F32 result = { 0 };

    log_error("%s is not yet implemented for Wayland", __func__);

    return(result);
}
