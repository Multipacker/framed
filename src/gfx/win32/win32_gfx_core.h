#ifndef WIN32_GFX_CORE_H
#define WIN32_GFX_CORE_H

typedef struct Gfx_Context Gfx_Content;
struct Gfx_Context
{
    HWND hwnd;
    HDC hdc;
};

typedef struct Win32_Gfx_State Win32_Gfx_State;
struct Win32_Gfx_State
{
    Gfx_Context *context;
    U32 placeholder;
    Arena *event_arena;
    Gfx_EventList *event_list;
};

#endif // WIN32_GFX_CORE_H