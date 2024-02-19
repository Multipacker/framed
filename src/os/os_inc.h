#ifndef OS_INC_H
#define OS_INC_H

#include "os/os_core.h"

#if OS_WINDOWS
#    include "os/win32/win32_core.h"
#elif OS_LINUX
#    include "os/linux/linux_core.h"
#else
#    error No os_inc for this OS
#endif

#endif // OS_INC_H
