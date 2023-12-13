#if OS_WINDOWS
#	include "os/win32/win32_essential.c"
#elif OS_LINUX
#	include "os/linux/linux_essential.c"
#else
#	error No os_inc for this OS
#endif