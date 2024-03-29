global Gfx_Key linux_sdl_to_gfx_keycode[128];

internal Gfx_Context
gfx_init(U32 x, U32 y, U32 width, U32 height, Str8 title)
{
    Gfx_Context gfx = {0};

    Arena_Temporary scratch = get_scratch(0, 0);

    CStr cstr_title = cstr_from_str8(scratch.arena, title);

    // NOTE(simon): Initialize `linux_sdl_to_gfx_keycode`
    for (SDL_KeyCode key = SDLK_0; key <= SDLK_9; ++key)
    {
        linux_sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_0 + (key - SDLK_0));
    }
    for (SDL_KeyCode key = SDLK_a; key <= SDLK_z; ++key)
    {
        linux_sdl_to_gfx_keycode[key] = (Gfx_Key) (Gfx_Key_A + (key - SDLK_a));
    }
    linux_sdl_to_gfx_keycode[SDLK_BACKSPACE] = Gfx_Key_Backspace;
    linux_sdl_to_gfx_keycode[SDLK_SPACE]     = Gfx_Key_Space;
    linux_sdl_to_gfx_keycode[SDLK_TAB]       = Gfx_Key_Tab;
    linux_sdl_to_gfx_keycode[SDLK_RETURN]    = Gfx_Key_Return;
    linux_sdl_to_gfx_keycode[SDLK_ESCAPE]    = Gfx_Key_Escape;
    linux_sdl_to_gfx_keycode[SDLK_DELETE]    = Gfx_Key_Delete;

    if (SDL_Init(SDL_INIT_VIDEO) == 0)
    {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
        SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);

        gfx.window = SDL_CreateWindow(
            cstr_title,
            (int) x, (int) y,
            (int) width, (int) height,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
        );

        if (gfx.window)
        {
            gfx.gl_context = SDL_GL_CreateContext(gfx.window);

            if (gladLoadGLLoader(SDL_GL_GetProcAddress))
            {
                SDL_GL_SetSwapInterval(1);
            }
            else
            {
                // TODO(simon): Could not load OpenGL functions.
            }
        }
        else
        {
            // TODO(simon): Could not create window
        }
    }
    else
    {
        // TODO(simon): Could not initialize SDL
    }

    release_scratch(scratch);
    return(gfx);
}

internal Void
gfx_show_window(Gfx_Context *gfx)
{
    SDL_ShowWindow(gfx->window);
}

internal Gfx_EventList
gfx_get_events(Arena *arena, Gfx_Context *gfx)
{
    Gfx_EventList events = {0};

    for (SDL_Event sdl_event = {0}; SDL_PollEvent(&sdl_event);)
    {
        Gfx_Event *event = push_struct_zero(arena, Gfx_Event);

        switch (sdl_event.type)
        {
            case SDL_QUIT:
            {
                event->kind = Gfx_EventKind_Quit;
            } break;

            case SDL_WINDOWEVENT:
            {
                if (sdl_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
                {
                    event->kind = Gfx_EventKind_Resize;
                }
            } break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
            {
                if (sdl_event.key.type == SDL_KEYDOWN)
                {
                    event->kind = Gfx_EventKind_KeyPress;
                }
                else
                {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                SDL_Keycode sdl_keycode = sdl_event.key.keysym.sym;

                switch (sdl_keycode)
                {
                    case SDLK_PAGEUP:   event->key = Gfx_Key_PageUp;   break;
                    case SDLK_PAGEDOWN: event->key = Gfx_Key_PageDown; break;
                    case SDLK_LEFT:     event->key = Gfx_Key_Left;     break;
                    case SDLK_RIGHT:    event->key = Gfx_Key_Right;    break;
                    case SDLK_UP:       event->key = Gfx_Key_Up;       break;
                    case SDLK_DOWN:     event->key = Gfx_Key_Down;     break;
                    case SDLK_LSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_RSHIFT:   event->key = Gfx_Key_Shift;    break;
                    case SDLK_END:      event->key = Gfx_Key_End;      break;
                    case SDLK_HOME:     event->key = Gfx_Key_Home;     break;
                    case SDLK_LCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_RCTRL:    event->key = Gfx_Key_Control;  break;
                    case SDLK_LALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_RALT:     event->key = Gfx_Key_Alt;      break;
                    case SDLK_LGUI:     event->key = Gfx_Key_OS;       break;
                    case SDLK_RGUI:     event->key = Gfx_Key_OS;       break;
                    default:
                    {
                        if (SDLK_F1 <= sdl_keycode && sdl_keycode <= SDLK_F12)
                        {
                            event->key = (Gfx_Key) (Gfx_Key_F1 + (sdl_keycode - SDLK_F1));
                        }
                        else if (sdl_keycode < (SDL_Keycode) array_count(linux_sdl_to_gfx_keycode))
                        {
                            event->key = linux_sdl_to_gfx_keycode[sdl_keycode];
                        }
                    } break;
                }

                if (event->key == Gfx_Key_Null)
                {
                    event->kind = Gfx_EventKind_Null;
                }
                else
                {
                    SDL_Keymod modifiers = SDL_GetModState();
                    event->key_modifiers |= (modifiers & KMOD_SHIFT ? Gfx_KeyModifier_Shift   : 0);
                    event->key_modifiers |= (modifiers & KMOD_CTRL  ? Gfx_KeyModifier_Control : 0);
                }
            } break;

            // TODO(simon): We might want to use this event for candidate text.
            case SDL_TEXTEDITING:
            {
            } break;

            case SDL_TEXTINPUT:
            {
                Str8 input = str8_cstr(sdl_event.text.text);
                U8 *ptr = input.data;
                U8 *opl = input.data + input.size;

                while (ptr < opl)
                {
                    StringDecode decode = string_decode_utf8(ptr, (U64) (opl - ptr));
                    ptr += decode.size;

                    event->kind      = Gfx_EventKind_Char;
                    event->character = decode.codepoint;

                    dll_push_back(events.first, events.last, event);
                    ++events.count;

                    event = push_struct_zero(arena, Gfx_Event);
                }
            } break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
            {
                if (sdl_event.button.type == SDL_MOUSEBUTTONDOWN)
                {
                    event->kind = Gfx_EventKind_KeyPress;
                }
                else
                {
                    event->kind = Gfx_EventKind_KeyRelease;
                }

                switch (sdl_event.button.button)
                {
                    case SDL_BUTTON_LEFT:
                    {
                        if (sdl_event.button.clicks == 1)
                        {
                            event->key = Gfx_Key_MouseLeft;
                        }
                        else
                        {
                            event->key = Gfx_Key_MouseLeftDouble;
                        }
                    } break;
                    case SDL_BUTTON_MIDDLE:
                    {
                        if (sdl_event.button.clicks == 1)
                        {
                            event->key = Gfx_Key_MouseMiddle;
                        }
                        else
                        {
                            event->key = Gfx_Key_MouseMiddleDouble;
                        }
                    } break;
                    case SDL_BUTTON_RIGHT:
                    {
                        if (sdl_event.button.clicks == 1)
                        {
                            event->key = Gfx_Key_MouseRight;
                        }
                        else
                        {
                            event->key = Gfx_Key_MouseRightDouble;
                        }
                    } break;
                    default:
                    {
                    } break;
                }
            } break;

            case SDL_MOUSEWHEEL:
            {
                event->kind   = Gfx_EventKind_Scroll;
                event->scroll = v2f32(-sdl_event.wheel.preciseX, sdl_event.wheel.preciseY);
            } break;
        }

        if (event->kind != Gfx_EventKind_Null)
        {
            dll_push_back(events.first, events.last, event);
            ++events.count;
        }
    }

    return(events);
}

internal Vec2F32
gfx_get_mouse_pos(Gfx_Context *gfx)
{
    int x = 0;
    int y = 0;
    SDL_GetMouseState(&x, &y);

    Vec2F32 result = v2f32((F32) x, (F32) y);
    return(result);
}

internal Vec2U32
gfx_get_window_area(Gfx_Context *gfx)
{
    int top = 0, left = 0, bottom = 0, right = 0;
    SDL_GetWindowBordersSize(gfx->window, &top, &left, &bottom, &right);

    int width = 0, height = 0;
    SDL_GetWindowSize(gfx->window, &width, &height);

    Vec2U32 result = v2u32((U32) (left + width + right), (U32) (top + height + bottom));
    return(result);
}

internal Vec2U32
gfx_get_window_client_area(Gfx_Context *gfx)
{
    int width  = 0;
    int height = 0;
    SDL_GL_GetDrawableSize(gfx->window, &width, &height);
    Vec2U32 result = v2u32((U32) width, (U32) height);
    return(result);
}

internal Void
gfx_toggle_fullscreen(Gfx_Context *gfx)
{
    // TODO(simon): This de-synchs when fullscreening in any other way than
    // calling this functions because we cannot query the state.
    gfx->is_fullscreen = !gfx->is_fullscreen;
    SDL_SetWindowFullscreen(gfx->window, gfx->is_fullscreen);
}

internal Void
gfx_swap_buffers(Gfx_Context *gfx)
{
    SDL_GL_SwapWindow(gfx->window);
}

internal Vec2F32
gfx_get_dpi(Gfx_Context *ctx)
{
    Vec2F32 dpi = {0};
    int display_index = SDL_GetWindowDisplayIndex(ctx->window);
    SDL_GetDisplayDPI(display_index, 0, &dpi.x, &dpi.y);
    return(dpi);
}

internal Void
gfx_set_cursor(Gfx_Context *ctx, Gfx_Cursor cursor)
{
    // TODO(simon): SDL doesn't seem to have support for this for some reason?
}

internal Void
gfx_set_window_maximized(Gfx_Context *ctx)
{
    SDL_MaximizeWindow(ctx->window);
}

internal Gfx_Monitor
gfx_monitor_from_window(Gfx_Context *ctx)
{
    Gfx_Monitor result = {0};
    int display_index = SDL_GetWindowDisplayIndex(ctx->window);
    result.u64[0] = (U64) display_index;
    return(result);
}

internal Vec2F32
gfx_dim_from_monitor(Gfx_Monitor monitor)
{
    Vec2F32 result = {0};
    SDL_DisplayMode display_mode = {0};
    SDL_GetCurrentDisplayMode((int) monitor.u64[0], &display_mode);
    result.x = (F32) display_mode.w;
    result.y = (F32) display_mode.h;
    return(result);
}
