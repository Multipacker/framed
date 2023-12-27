// TODO(hampus):
// - Window resizing glitching (showing an ancient window for some reason)
// - Stop the program from pausing during resize
// - Test on laptop

global Win32_Gfx_State win32_gfx_state;

DWORD main_thread_id;

internal LRESULT CALLBACK
win32_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	Gfx_Context *context = (Gfx_Context *) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
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
		message == WM_CLOSE)
	{
		PostThreadMessage(main_thread_id, message, wparam, lparam);
	}
	else
	{
	result = DefWindowProcW(hwnd, message, wparam, lparam);
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
	ThreadContext context = thread_ctx_alloc();
	thread_set_ctx(&context);
	thread_set_name(str8_lit("Events"));

	Win32_WindowCreationData *window_creation_data = (Win32_WindowCreationData *)data;

	Gfx_Context result = { 0 };
	Arena_Temporary scratch = get_scratch(0, 0);

	HINSTANCE instance = GetModuleHandle(0);

	Str16 class_name = cstr16_from_str8(scratch.arena, str8_lit("ApplicationWindowClassName"));

	WNDCLASS window_class = { 0 };

	window_class.style = 0;
	window_class.lpfnWndProc = win32_window_proc;
	window_class.hInstance = instance;
	window_class.lpszClassName = class_name.data;
	window_class.hCursor = LoadCursor(0, (LPCWSTR) IDC_ARROW);

	ATOM register_class_result = RegisterClass(&window_class);
	if (register_class_result)
	{
		DWORD create_window_flags = WS_OVERLAPPEDWINDOW;

		Str16 title_s16 = cstr16_from_str8(scratch.arena, window_creation_data->title);
		result.hwnd = CreateWindow(window_class.lpszClassName, (LPCWSTR) title_s16.data,
															 create_window_flags,
								   window_creation_data->x, window_creation_data->y,
								   window_creation_data->width, window_creation_data->height,
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

	release_scratch(scratch);

#if defined(RENDERER_OPENGL)
	win32_init_opengl(&result);
#endif
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
	main_thread_id = GetThreadId(GetCurrentThread());
	Win32_WindowCreationData data = {x, y, width, height, title};
	CreateThread(0, 0, win32_gfx_startup_thread, &data, 0, 0);
	while (!win32_gfx_state.context.hwnd);
	Gfx_Context result = win32_gfx_state.context;
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
				event->kind = Gfx_EventKind_Char;
				event->character = (char) message.wParam;
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

				// NOTE(hampus): Don't repeat key down/up messages
				if (was_down != is_down)
				{
					// NOTE(hampus): The event->key may already be set
					// by the mouse.
					if (event->key == Gfx_Key_Null)
					{
						event->key = win32_gfx_state.key_table[vk_code];
					}

					B32 up_message = (message.message == WM_SYSKEYUP || message.message == WM_KEYUP || message.message == WM_LBUTTONUP || message.message == WM_RBUTTONUP || message.message == WM_MBUTTONUP);
					event->kind = up_message ? Gfx_EventKind_KeyRelease : Gfx_EventKind_KeyPress;
				}
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
	Vec2F32 result = { 0 };
	POINT point = { 0 };
	GetCursorPos(&point);
	result.x = (F32) point.x;
	result.y = (F32) point.y;
	return(result);
}

internal Vec2U32
gfx_get_window_area(Gfx_Context *gfx)
{
	Vec2U32 result = { 0 };
	RECT rect = { 0 };
	GetWindowRect(gfx->hwnd, &rect);
	result.x = rect.right - rect.left;
	result.y = rect.bottom - rect.top;
	return(result);
}

internal Vec2U32
gfx_get_window_client_area(Gfx_Context *gfx)
{
	Vec2U32 result = { 0 };
	RECT rect = { 0 };
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

internal Vec2F32
gfx_get_dpi(Gfx_Context *gfx)
{
	// NOTE(hampus): The primary monitor by definition
	// has its upper left corner at (0, 0)
	POINT point = { 0, 0 };
	HMONITOR monitor = MonitorFromPoint(point, MONITOR_DEFAULTTOPRIMARY);
	U32 dpi_x, dpi_y;
	GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
	Vec2F32 result;
	result.x = (F32) dpi_x;
	result.y = (F32) dpi_y;
	return(result);
}