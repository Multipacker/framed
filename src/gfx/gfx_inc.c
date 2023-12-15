#if OS_WINDOWS
#	include "gfx/win32/win32_gfx_core.c"
#elif OS_LINUX
#	include "gfx/linux/linux_gfx_core.c"
#else
#	error No gfx_inc for this OS
#endif