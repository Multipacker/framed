#ifndef WIN32_GFX_CORE_H
#define WIN32_GFX_CORE_H

typedef struct Gfx_Context Gfx_Context;
struct Gfx_Context
{
    U8 temp;
};

typedef struct Win32_Gfx_State Win32_Gfx_State;
struct Win32_Gfx_State
{
    B32 key_table_initialized;
    U8  key_table[128];
    DWORD main_thread_id;
    HCURSOR cursors[Gfx_Cursor_COUNT];
    HCURSOR cursor;
    B32 resizing;

    // NOTE(simon): Per window state, I think.
    HWND hwnd;
    HDC hdc;
};

global volatile Win32_Gfx_State win32_gfx_state;

#endif // WIN32_GFX_CORE_H
