// TODO(hampus):
// - Window resizing glitching (showing an ancient window for some reason)
// - Stop the program from pausing during resize
// - Test on laptop

global Win32_Gfx_State win32_gfx_state;

internal LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    Gfx_Context *context = (Gfx_Context *)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    Arena_Temporary scratch = arena_get_scratch(0, 0);
    Gfx_EventList fallback_event_list = { 0 };
	if (win32_gfx_state.event_arena == 0)
	{
		win32_gfx_state.event_arena = scratch.arena;
		win32_gfx_state.event_list = &fallback_event_list;
	}
    Arena *event_arena = win32_gfx_state.event_arena;
    Gfx_EventList *event_list = win32_gfx_state.event_list;
	Gfx_Event *event = push_struct_zero(event_arena, Gfx_Event);
    event->kind = Gfx_EventKind_Null;
	LRESULT result = 0;
	switch (message)
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
			event->kind = Gfx_EventKind_Char;
			event->character = (char)wparam;
		} break;

		case WM_SIZE:
		{
			event->kind = Gfx_EventKind_Resize;
		} break;

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
			result = DefWindowProcW(hwnd, message, wparam, lparam);
        } break;

		case WM_MOUSEWHEEL:
		{
			event->kind = Gfx_EventKind_Scroll;
			event->scroll.y = (F32)(GET_WHEEL_DELTA_WPARAM(wparam) / WHEEL_DELTA);
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
			U32 vk_code = (U32)wparam;
			B32 was_down = ((lparam & (1 << 30)) != 0);
			B32 is_down = ((lparam & (1 << 31)) == 0);
			B32 alt_key_was_down = ((lparam & (1 << 29)));

			local B32 key_table_initialized = false;

			local char key_table[128] ={ 0 };

			if (!key_table_initialized)
			{
				for (U64 i = 0; i < 10; ++i)
				{
					key_table[0x30 + i] = (char)(Gfx_Key_0 + i);
				}

				for (U64 i = 0; i < 26; ++i)
				{
					key_table[0x41 + i] = (char)(Gfx_Key_A + i);
				}

				for (U64 i = 0; i < 12; ++i)
				{
					key_table[VK_F1 + i] = (char)(Gfx_Key_F1 + i);
				}

				key_table[VK_BACK]    = Gfx_Key_Backspace;
				key_table[VK_SPACE]   = Gfx_Key_Space;
				key_table[VK_MENU]    = Gfx_Key_Alt;
				key_table[VK_LWIN]    = Gfx_Key_OS;
				key_table[VK_RWIN]    = Gfx_Key_OS;
				key_table[VK_TAB]     = Gfx_Key_Tab;
				key_table[VK_RETURN]  = Gfx_Key_Return;
				key_table[VK_SHIFT]   = Gfx_Key_Shift;
				key_table[VK_CONTROL] = Gfx_Key_Control;
				key_table[VK_ESCAPE]  = Gfx_Key_Escape;
				key_table[VK_PRIOR]   = Gfx_Key_PageUp;
				key_table[VK_NEXT]    = Gfx_Key_PageDown;
				key_table[VK_END]     = Gfx_Key_End;
				key_table[VK_HOME]    = Gfx_Key_Home;
				key_table[VK_LEFT]    = Gfx_Key_Left;
				key_table[VK_RIGHT]   = Gfx_Key_Right;
				key_table[VK_UP]      = Gfx_Key_Up;
				key_table[VK_DOWN]    = Gfx_Key_Down;
				key_table[VK_DELETE]  = Gfx_Key_Delete;
				key_table[VK_LBUTTON] = Gfx_Key_MouseLeft;
				key_table[VK_RBUTTON] = Gfx_Key_MouseRight;
				key_table[VK_MBUTTON] = Gfx_Key_MouseMiddle;

				key_table_initialized = true;
			}

            // NOTE(hampus): Don't repeat key down/up messages
            if (was_down != is_down)
            {
                // NOTE(hampus): The event->key may already be set
                // by the mouse.
                if (event->key == Gfx_Key_Null)
                {
                    event->key = key_table[vk_code];
                }

                B32 up_message = (message == WM_SYSKEYUP || message == WM_KEYUP || message == WM_LBUTTONUP || message == WM_RBUTTONUP || message == WM_MBUTTONUP);
                event->kind = up_message ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
            }

		} break;

		default:
		{
			result = DefWindowProcW(hwnd, message, wparam, lparam);
		} break;
	}

	if (event->kind != Gfx_EventKind_Null)
	{
		dll_push_back(event_list->first, event_list->last, event);
	}

	arena_release_scratch(scratch);

    if (win32_gfx_state.event_list == &fallback_event_list)
    {
        win32_gfx_state.event_arena = 0;
        win32_gfx_state.event_list = 0;
    }

	return(result);
}

internal Gfx_Context
gfx_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    Gfx_Context result = {0};
    Arena_Temporary scratch = arena_get_scratch(0, 0);

    win32_gfx_state.context = &result;

	HINSTANCE instance = GetModuleHandle(0);

	Str16 class_name = cstr16_from_str8(scratch.arena, str8_lit("ApplicationWindowClassName"));

	WNDCLASS window_class = {0};

	window_class.style = 0;
	window_class.lpfnWndProc = win32_window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = class_name.data;
	window_class.hCursor = LoadCursor(0, (LPCWSTR)IDC_ARROW);

	ATOM register_class_result = RegisterClass(&window_class);
    if (register_class_result)
    {
        DWORD create_window_flags = WS_OVERLAPPEDWINDOW;

        Str16 title_s16 = cstr16_from_str8(scratch.arena, title);
        result.hwnd = CreateWindow(window_class.lpszClassName, (LPCWSTR)title_s16.data,
                                   create_window_flags,
                                   x, y,
                                   width, height,
                                   0, 0, instance, 0);
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

    arena_release_scratch(scratch);

    return(result);
}

internal Void
gfx_show_window(Gfx_Context *gfx)
{
    Arena_Temporary scratch = arena_get_scratch(0, 0);
	Gfx_EventList events ={ 0 };
    win32_gfx_state.event_list = &events;
	win32_gfx_state.event_arena = scratch.arena;
	ShowWindow(gfx->hwnd, SW_SHOW);
	UpdateWindow(gfx->hwnd);
	arena_release_scratch(scratch);
}

internal Gfx_EventList
gfx_get_events(Arena *arena, Gfx_Context *gfx)
{
    Gfx_EventList event_list = { 0 };
	win32_gfx_state.event_arena = arena;
	win32_gfx_state.event_list = &event_list;

	for (MSG message; PeekMessage(&message, 0, 0, 0, PM_REMOVE);)
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}

	win32_gfx_state.event_arena = 0;
	win32_gfx_state.event_list = 0;

	return(event_list);
}

internal Vec2F32
gfx_get_mouse_pos(Gfx_Context *gfx)
{
    Vec2F32 result = {0};
    POINT point = {0};
    GetCursorPos(&point);
    result.x = (F32)point.x;
    result.y = (F32)point.y;
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
    local WINDOWPLACEMENT prev_placement = { sizeof(prev_placement) };
	DWORD WindowStyle = GetWindowLong(context->hwnd, GWL_STYLE);
	if (WindowStyle & WS_OVERLAPPEDWINDOW)
	{
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetWindowPlacement(context->hwnd, &prev_placement) &&
		    GetMonitorInfo(MonitorFromWindow(context->hwnd, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
		{
			SetWindowLong(context->hwnd, GWL_STYLE, WindowStyle & ~WS_OVERLAPPEDWINDOW);

			SetWindowPos(context->hwnd, HWND_TOP,
			             MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
			             MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
			             MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
			             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else
	{
		SetWindowLong(context->hwnd, GWL_STYLE, WindowStyle | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(context->hwnd, &prev_placement);
		SetWindowPos(context->hwnd, NULL, 0, 0, 0, 0,
		             SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
		             SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}