#ifndef GFX_INC_H
#define GFX_INC_H

#include "gfx/gfx_essential.h"

#if OS_WINDOWS
#	include "gfx/win32/win32_gfx_core.h"
#elif OS_LINUX
#	include "gfx/linux/linux_gfx_core.h"
#else
#	error No gfx_inc for this OS
#endif

#endif // GFX_INC_H