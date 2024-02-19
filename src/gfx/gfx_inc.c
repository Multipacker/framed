#if OS_WINDOWS
#    include "gfx/win32/win32_gfx_core.c"
#    if defined(RENDERER_OPENGL)
#        include "gfx/win32/win32_gfx_opengl.c"
#    endif
#elif OS_LINUX
#    include "gfx/linux/linux_gfx_core.c"
#else
#    error No gfx_inc for this OS
#endif