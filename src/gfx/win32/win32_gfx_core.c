global volatile Win32_Gfx_State win32_gfx_state;

internal LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    Arena_Temporary scratch = get_scratch(0, 0);
    Gfx_Event event;
    LRESULT result = 0;
    if (message == WM_SIZE ||
        message == WM_KEYDOWN ||
        message == WM_KEYUP ||
        message == WM_SYSKEYDOWN ||
        message == WM_SYSKEYUP ||
        message == WM_QUIT ||
        message == WM_DESTROY ||
        message == WM_CLOSE ||
        message == WM_CHAR ||
        message == WM_MOUSEWHEEL ||
        message == WM_LBUTTONDBLCLK ||
        message == WM_MBUTTONDBLCLK ||
        message == WM_RBUTTONDBLCLK ||
        message == WM_MBUTTONUP ||
        message == WM_MBUTTONDOWN ||
        message == WM_RBUTTONUP ||
        message == WM_RBUTTONDOWN ||
        message == WM_LBUTTONUP ||
        message == WM_LBUTTONDOWN ||
        message == WM_SETCURSOR)
    {
        if (message == WM_SETCURSOR)
        {
            RECT rect = {0};
            GetClientRect(win32_gfx_state.context.hwnd, &rect);
            POINT point = {0};
            GetCursorPos(&point);
            ScreenToClient(win32_gfx_state.context.hwnd, &point);
            B32 mouse_is_hover_window =
                point.x >= rect.left && point.x < rect.right &&
                point.y >= rect.top  && point.y < rect.bottom;

            if (!win32_gfx_state.resizing && mouse_is_hover_window)
            {
                SetCursor(win32_gfx_state.cursor);
            }
            else
            {
                result = DefWindowProc(hwnd, message, wparam, lparam);
            }
        }
        else
        {
            if (!PostThreadMessage(win32_gfx_state.main_thread_id, message, wparam, lparam))
            {
                win32_print_error_message();
            }
            result = true;
        }
    }
    else if (message == WM_ENTERSIZEMOVE)
    {
        win32_gfx_state.resizing = true;
    }
    else if (message == WM_EXITSIZEMOVE)
    {
        win32_gfx_state.resizing = false;
    }
    else
    {
        result = DefWindowProc(hwnd, message, wparam, lparam);
    }
    release_scratch(scratch);
    return(result);
}

typedef struct Win32_WindowCreationData Win32_WindowCreationData;
struct Win32_WindowCreationData
{
    U32 x;
    U32 y;
    U32 width;
    U32 height;
    Str8 title;
};

DWORD
win32_gfx_startup_thread(Void *data)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    Win32_WindowCreationData *window_creation_data = (Win32_WindowCreationData *) data;

    ThreadContext *context = thread_ctx_init(str8_lit("Events"));

    Gfx_Context result = {0};
    Arena_Temporary scratch = get_scratch(0, 0);

    win32_gfx_state.cursors[Gfx_Cursor_Arrow]    = LoadCursor(0, IDC_ARROW);
    win32_gfx_state.cursors[Gfx_Cursor_Hand]     = LoadCursor(0, IDC_HAND);
    win32_gfx_state.cursors[Gfx_Cursor_Beam]     = LoadCursor(0, IDC_IBEAM);
    win32_gfx_state.cursors[Gfx_Cursor_SizeNWSE] = LoadCursor(0, IDC_SIZENWSE);
    win32_gfx_state.cursors[Gfx_Cursor_SizeNESW] = LoadCursor(0, IDC_SIZENESW);
    win32_gfx_state.cursors[Gfx_Cursor_SizeWE]   = LoadCursor(0, IDC_SIZEWE);
    win32_gfx_state.cursors[Gfx_Cursor_SizeNS]   = LoadCursor(0, IDC_SIZENS);
    win32_gfx_state.cursors[Gfx_Cursor_SizeAll]  = LoadCursor(0, IDC_SIZEALL);

    HINSTANCE instance = GetModuleHandle(0);

    CStr16 class_name = cstr16_from_str8(scratch.arena, str8_lit("ApplicationWindowClassName"));

    WNDCLASS window_class = {0};

    window_class.style = 0;
    window_class.lpfnWndProc = win32_window_proc;
    window_class.hInstance = instance;
    window_class.lpszClassName = class_name;

    ATOM register_class_result = RegisterClass(&window_class);
    if (register_class_result)
    {
        DWORD create_window_flags = WS_OVERLAPPEDWINDOW;

        CStr16 title_s16 = cstr16_from_str8(scratch.arena, window_creation_data->title);
        result.hwnd = CreateWindow(
                                   window_class.lpszClassName, (LPCWSTR) title_s16,
                                   create_window_flags,
                                   window_creation_data->x, window_creation_data->y,
                                   window_creation_data->width, window_creation_data->height,
                                   0, 0, instance, 0
                                   );
        if (result.hwnd)
        {
            result.hdc = GetDC(result.hwnd);
        }
        else
        {
            win32_print_error_message();
        }
    }
    else
    {
        win32_print_error_message();
    }

    release_scratch(scratch);

    memory_fence();

    win32_gfx_state.context = result;

    for (MSG message; GetMessage(&message, 0, 0, 0);)
    {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return(0);
}

internal Gfx_Context
gfx_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    win32_gfx_state.main_thread_id = GetThreadId(GetCurrentThread());
    Win32_WindowCreationData data = {x, y, width, height, title};
    CreateThread(0, 0, win32_gfx_startup_thread, &data, 0, 0);
    while (!win32_gfx_state.context.hwnd && !win32_gfx_state.context.hdc);
    Gfx_Context result = win32_gfx_state.context;
#if defined(RENDERER_OPENGL)
    win32_init_opengl(&result);
#endif
    return(result);
}

internal Void
gfx_show_window(Gfx_Context *gfx)
{
    ShowWindow(gfx->hwnd, SW_SHOW);
    UpdateWindow(gfx->hwnd);
}

internal Gfx_EventList
gfx_get_events(Arena *arena, Gfx_Context *gfx)
{
    Gfx_EventList result = {0};

    for (MSG message; PeekMessage(&message, 0, 0, 0, PM_REMOVE);)
    {
        Gfx_Event *event = push_struct_zero(arena, Gfx_Event);
        event->kind = Gfx_EventKind_Null;
        switch (message.message)
        {
            case WM_CLOSE:
            {
                event->kind = Gfx_EventKind_Quit;
            } break;

            case WM_QUIT:
            {
                event->kind = Gfx_EventKind_Quit;
            } break;

            case WM_DESTROY:
            {
                event->kind = Gfx_EventKind_Quit;
            } break;

            case WM_CHAR:
            {
                if (message.wParam >= ' ' && message.wParam <= '~')
                {
                    event->kind = Gfx_EventKind_Char;
                    event->character = (char) message.wParam;
                }
            } break;

            case WM_SIZE:
            {
                event->kind = Gfx_EventKind_Resize;
            } break;

            case WM_MOUSEWHEEL:
            {
                event->kind = Gfx_EventKind_Scroll;
                event->scroll.y = (F32) (GET_WHEEL_DELTA_WPARAM(message.wParam) / WHEEL_DELTA);
            } break;

            case WM_LBUTTONDBLCLK:
            {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseLeftDouble;
            } break;

            case WM_MBUTTONDBLCLK:
            {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseRightDouble;
            } break;

            case WM_RBUTTONDBLCLK:
            {
                event->kind = Gfx_EventKind_KeyPress;
                event->key = Gfx_Key_MouseMiddleDouble;
            } break;

            case WM_MBUTTONUP:
            case WM_MBUTTONDOWN:
            {
                event->key = Gfx_Key_MouseMiddle;
                goto key_begin;
            }

            case WM_RBUTTONUP:
            case WM_RBUTTONDOWN:
            {
                event->key = Gfx_Key_MouseRight;
                goto key_begin;
            }

            case WM_LBUTTONUP:
            case WM_LBUTTONDOWN:
            {
                event->key = Gfx_Key_MouseLeft;
                goto key_begin;
            }

            case WM_SYSKEYUP:
            case WM_SYSKEYDOWN:
            case WM_KEYUP:
            case WM_KEYDOWN:
            {
                key_begin:
                U32 vk_code = (U32) message.wParam;
                B32 was_down = ((message.lParam & (1 << 30)) != 0);
                B32 is_down = ((message.lParam & (1 << 31)) == 0);
                B32 alt_key_was_down = ((message.lParam & (1 << 29)));

                if (!win32_gfx_state.key_table_initialized)
                {
                    for (U64 i = 0; i < 10; ++i)
                    {
                        win32_gfx_state.key_table[0x30 + i] = (char) (Gfx_Key_0 + i);
                    }

                    for (U64 i = 0; i < 26; ++i)
                    {
                        win32_gfx_state.key_table[0x41 + i] = (char) (Gfx_Key_A + i);
                    }

                    for (U64 i = 0; i < 12; ++i)
                    {
                        win32_gfx_state.key_table[VK_F1 + i] = (char) (Gfx_Key_F1 + i);
                    }

                    win32_gfx_state.key_table[VK_BACK]    = Gfx_Key_Backspace;
                    win32_gfx_state.key_table[VK_SPACE]   = Gfx_Key_Space;
                    win32_gfx_state.key_table[VK_MENU]    = Gfx_Key_Alt;
                    win32_gfx_state.key_table[VK_LWIN]    = Gfx_Key_OS;
                    win32_gfx_state.key_table[VK_RWIN]    = Gfx_Key_OS;
                    win32_gfx_state.key_table[VK_TAB]     = Gfx_Key_Tab;
                    win32_gfx_state.key_table[VK_RETURN]  = Gfx_Key_Return;
                    win32_gfx_state.key_table[VK_SHIFT]   = Gfx_Key_Shift;
                    win32_gfx_state.key_table[VK_CONTROL] = Gfx_Key_Control;
                    win32_gfx_state.key_table[VK_ESCAPE]  = Gfx_Key_Escape;
                    win32_gfx_state.key_table[VK_PRIOR]   = Gfx_Key_PageUp;
                    win32_gfx_state.key_table[VK_NEXT]    = Gfx_Key_PageDown;
                    win32_gfx_state.key_table[VK_END]     = Gfx_Key_End;
                    win32_gfx_state.key_table[VK_HOME]    = Gfx_Key_Home;
                    win32_gfx_state.key_table[VK_LEFT]    = Gfx_Key_Left;
                    win32_gfx_state.key_table[VK_RIGHT]   = Gfx_Key_Right;
                    win32_gfx_state.key_table[VK_UP]      = Gfx_Key_Up;
                    win32_gfx_state.key_table[VK_DOWN]    = Gfx_Key_Down;
                    win32_gfx_state.key_table[VK_DELETE]  = Gfx_Key_Delete;
                    win32_gfx_state.key_table[VK_LBUTTON] = Gfx_Key_MouseLeft;
                    win32_gfx_state.key_table[VK_RBUTTON] = Gfx_Key_MouseRight;
                    win32_gfx_state.key_table[VK_MBUTTON] = Gfx_Key_MouseMiddle;

                    win32_gfx_state.key_table_initialized = true;
                }
                // NOTE(hampus): The event->key may already be set
                // by the mouse.
                if (!event->key)
                {
                    if (win32_gfx_state.key_table[vk_code] != 0)
                    {
                        event->key = win32_gfx_state.key_table[vk_code];
                    }
                }

                event->key_modifiers |= ((GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0) * Gfx_KeyModifier_Shift;
                event->key_modifiers |= ((GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0) * Gfx_KeyModifier_Control;

                B32 up_message = (message.message == WM_SYSKEYUP || message.message == WM_KEYUP || message.message == WM_LBUTTONUP || message.message == WM_RBUTTONUP || message.message == WM_MBUTTONUP);
                event->kind = up_message ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
            } break;
        }

        if (event->kind != Gfx_EventKind_Null)
        {
            dll_push_back(result.first, result.last, event);
        }
    }
    return(result);
}

internal Vec2F32
gfx_get_mouse_pos(Gfx_Context *gfx)
{
    Vec2F32 result = {0};
    POINT point = {0};
    GetCursorPos(&point);
    ScreenToClient(gfx->hwnd, &point);
    result.x = (F32) point.x;
    result.y = (F32) point.y;
    return(result);
}

internal Vec2U32
gfx_get_window_area(Gfx_Context *gfx)
{
    Vec2U32 result = {0};
    RECT rect = {0};
    GetWindowRect(gfx->hwnd, &rect);
    result.x = rect.right - rect.left;
    result.y = rect.bottom - rect.top;
    return(result);
}

internal Vec2U32
gfx_get_window_client_area(Gfx_Context *gfx)
{
    Vec2U32 result = {0};
    RECT rect = {0};
    GetClientRect(gfx->hwnd, &rect);
    result.x = rect.right - rect.left;
    result.y = rect.bottom - rect.top;
    return(result);
}

internal Void
gfx_toggle_fullscreen(Gfx_Context *context)
{
    local WINDOWPLACEMENT prev_placement = {sizeof(prev_placement)};
    DWORD window_style = GetWindowLong(context->hwnd, GWL_STYLE);
    if (window_style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitor_info = {sizeof(monitor_info)};
        if (GetWindowPlacement(context->hwnd, &prev_placement) &&
            GetMonitorInfo(MonitorFromWindow(context->hwnd, MONITOR_DEFAULTTOPRIMARY), &monitor_info))
        {
            SetWindowLong(context->hwnd, GWL_STYLE, window_style & ~WS_OVERLAPPEDWINDOW);

            SetWindowPos(context->hwnd, HWND_TOP,
                         monitor_info.rcMonitor.left, monitor_info.rcMonitor.top,
                         monitor_info.rcMonitor.right - monitor_info.rcMonitor.left,
                         monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(context->hwnd, GWL_STYLE, window_style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(context->hwnd, &prev_placement);
        SetWindowPos(context->hwnd, NULL, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal Vec2F32
gfx_get_dpi(Gfx_Context *gfx)
{
    UINT dpi = GetDpiForWindow(gfx->hwnd);
    Vec2F32 result;
    result.x = (F32) dpi;
    result.y = (F32) dpi;
    return(result);
}

internal Void
gfx_set_cursor(Gfx_Context *ctx, Gfx_Cursor cursor)
{
    HCURSOR win32_cursor = win32_gfx_state.cursors[cursor];
    if (!win32_gfx_state.resizing && win32_gfx_state.cursor != win32_cursor)
    {
        win32_gfx_state.cursor = win32_cursor;
        PostMessage(ctx->hwnd, WM_SETCURSOR, 0, 0);
        POINT p = {0};
        GetCursorPos(&p);
        SetCursorPos(p.x, p.y);
    }
}

internal void
gfx_set_window_maximized(Gfx_Context *ctx)
{
    ShowWindow(ctx->hwnd, SW_MAXIMIZE);
}

internal Gfx_Monitor
gfx_monitor_from_window(Gfx_Context *ctx)
{
    Gfx_Monitor result = {0};
    result.u64[0] = int_from_ptr(MonitorFromWindow(ctx->hwnd, MONITOR_DEFAULTTOPRIMARY));
    return(result);
}

internal Vec2F32
gfx_dim_from_monitor(Gfx_Monitor monitor)
{
    Vec2F32 result = {0};
    MONITORINFO monitor_info = {sizeof(monitor_info)};
    HMONITOR hmonitor = (HMONITOR) monitor.u64[0];
    GetMonitorInfo(hmonitor, &monitor_info);
    result.x = (F32)(monitor_info.rcMonitor.right - monitor_info.rcMonitor.left);
    result.y = (F32)(monitor_info.rcMonitor.bottom - monitor_info.rcMonitor.top);
    return(result);
}

internal Void
gfx_set_clipboard(Str8 data)
{
    // TODO(hampus): Memory leak?
    HGLOBAL memory =  GlobalAlloc(GMEM_MOVEABLE, data.size+1);
    memory_copy(GlobalLock(memory), data.data, data.size);
    GlobalUnlock(memory);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, memory);
    CloseClipboard();
}

internal Str8
gfx_push_clipboard(Arena *arena)
{
    OpenClipboard(0);
    CStr data = GetClipboardData(CF_TEXT);
    Str8 result = str8_copy_cstr(arena, data);
    CloseClipboard();
    return(result);
}

internal Vec2F32
gfx_scale_from_window(Gfx_Context *gfx)
{
    Vec2F32 result = {1, 1};
    UINT dpi_for_window = GetDpiForWindow(gfx->hwnd);
    if (dpi_for_window)
    {
        result.x = (F32)dpi_for_window / 96.0f;
        result.y = (F32)dpi_for_window / 96.0f;
    }
    return(result);
}