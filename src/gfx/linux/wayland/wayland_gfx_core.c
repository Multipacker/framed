#include "wayland_xdg_shell.generated.c"

internal Void
wayland_xdg_wm_base_handle_ping(Void *data, struct xdg_wm_base *xdg_wm_base, U32 serial)
{
    xdg_wm_base_pong(xdg_wm_base, serial);
}

global struct xdg_wm_base_listener wayland_xdg_wm_base_listener = {
    .ping = wayland_xdg_wm_base_handle_ping,
};

internal Void
wayland_registry_handle_global(Void *data,
                               struct wl_registry *registry,
                               U32 name,
                               const char *interface,
                               U32 version)
{
    Gfx_Wayland_Context *gfx = (Gfx_Wayland_Context *) data;
    if (strcmp(interface, wl_compositor_interface.name) == 0)
    {
        gfx->compositor = wl_registry_bind(gfx->registry, name, &wl_compositor_interface, 4);
    }
    else if (strcmp(interface, wl_shm_interface.name) == 0)
    {
        gfx->shm = wl_registry_bind(gfx->registry, name, &wl_shm_interface, 1);
    }
    else if (strcmp(interface, xdg_wm_base_interface.name) == 0)
    {
        gfx->xdg_wm_base = wl_registry_bind(gfx->registry, name, &xdg_wm_base_interface, 1);
        xdg_wm_base_add_listener(gfx->xdg_wm_base, &wayland_xdg_wm_base_listener, gfx);
    }
    else
    {
        log_info("Register %"PRIU32": %s v%"PRIU32, name, interface, version);
    }
}

internal Void
wayland_registry_handle_global_remove(Void *data,
                                      struct wl_registry *registry,
                                      U32 name)
{
    Gfx_Wayland_Context *gfx = (Gfx_Wayland_Context *) data;
    log_info("Remove %"PRIU32, name);
}


global struct wl_registry_listener wayland_registry_listener = {
#undef global
    .global = wayland_registry_handle_global,
#define global static
    .global_remove = wayland_registry_handle_global_remove,
};

internal Void
wayland_xdg_surface_handle_configure(Void *data, struct xdg_surface *xdg_surface, U32 serial)
{
    Gfx_Wayland_Context *gfx = (Gfx_Wayland_Context *) data;
    xdg_surface_ack_configure(xdg_surface, serial);

    U64 buffer_size = 2 * 1920 * 1080 * sizeof(U32);

    // NOTE(simon): For now we allocate a pixel buffer for CPU rendering just for testing.
    int shared_memory = memfd_create("framed-wayland-buffers", 0);
    if (shared_memory != -1)
    {
        int ret = 0;
        do
        {
            ret = ftruncate(shared_memory, (off_t) buffer_size);
        } while (ret == -1 && errno == EINTR);

        if (ret != -1)
        {
            U8 *buffer_data = mmap(0, buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED, shared_memory, 0);
            struct wl_shm_pool *pool = wl_shm_create_pool(gfx->shm, shared_memory, (S32) buffer_size);
            struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool, 0, 1920, 1080, 1920 * sizeof(U32), WL_SHM_FORMAT_XRGB8888);

            memory_set(buffer_data, 0, buffer_size);

            wl_surface_attach(gfx->surface, buffer, 0, 0);
            wl_surface_damage(gfx->surface, 0, 0, (S32) U32_MAX, (S32) U32_MAX);
            wl_surface_commit(gfx->surface);
        }
        else
        {
            log_error("Could not create resize shared memory");
        }
    }
    else
    {
        log_error("Could not create shared memory");
    }
}

global struct xdg_surface_listener wayland_xdg_surface_listener = {
    .configure = wayland_xdg_surface_handle_configure,
};

internal Gfx_Wayland_Context
wayland_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    Gfx_Wayland_Context result = { 0 };

    result.display = wl_display_connect(0);
    if (!result.display)
    {
        log_error("Could not connect to display");
    }
    else
    {
        result.registry = wl_display_get_registry(result.display);
        wl_registry_add_listener(result.registry, &wayland_registry_listener, &result);

        // NOTE(simon): Get all current globals.
        wl_display_roundtrip(result.display);

        // NOTE(simon): Ensure that we could bind to all globals we need.
        Void *objects[] = {
            result.compositor,
            result.shm,
            result.xdg_wm_base,
        };
        CStr messages[] = {
            "Could not bind to wl_compositor",
            "Could not bind to wl_shm",
            "Could not bind to xdg_wm_base",
        };

        B32 has_all_globals = true;
        for (U32 i = 0; i < array_count(objects); ++i)
        {
            if (!objects[i])
            {
                log_error(messages[i]);
                has_all_globals = false;
            }
        }

        if (has_all_globals)
        {
            result.surface = wl_compositor_create_surface(result.compositor);

            if (result.surface)
            {
                result.xdg_surface = xdg_wm_base_get_xdg_surface(result.xdg_wm_base, result.surface);

                if (result.xdg_surface)
                {
                    xdg_surface_add_listener(result.xdg_surface, &wayland_xdg_surface_listener, &result);
                    result.xdg_toplevel = xdg_surface_get_toplevel(result.xdg_surface);
                    if (result.xdg_toplevel)
                    {
                        arena_scratch(0, 0)
                        {
                            CStr cstr_title = cstr_from_str8(scratch, title);
                            xdg_toplevel_set_title(result.xdg_toplevel, cstr_title);
                        }

                        wl_surface_commit(result.surface);

                        while (wl_display_dispatch(result.display))
                        {
                        }
                    }
                    else
                    {
                        log_error("Could not create XDG toplevel");
                    }
                }
                else
                {
                    log_error("Could not create XDG surface");
                }
            }
            else
            {
                log_error("Could not create surface");
            }
        }
    }

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
