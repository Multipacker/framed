#ifndef OS_INC_H
#define OS_INC_H

#include "os/os_essential.h"

#if OS_WINDOWS
#	include "os/win32/win32_essential.h"
#elif OS_LINUX
#else
#	error No os_inc for this OS
#endif

#endif // OS_INC_H