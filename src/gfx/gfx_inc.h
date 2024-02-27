#ifndef GFX_INC_H
#define GFX_INC_H

#include "gfx/gfx_core.h"

#if OS_WINDOWS
#    if defined(WIN32_ESSENTIAL_H)
#        include "gfx/win32/win32_gfx_core.h"
#        if defined(RENDERER_OPENGL)
#            include "gfx/win32/win32_gfx_opengl.h"
#        endif
#    else
#        error OS layer has not been included
#    endif
#elif OS_LINUX
#    if defined(LINUX_ESSENTIAL_H)
#        include "gfx/linux/linux_gfx_core.h"
#    else
#        error OS layer has not been included
#    endif
#else
#    error No gfx_inc for this OS
#endif

#endif // GFX_INC_H
