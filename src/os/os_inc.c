#if OS_WINDOWS
#    include "os/win32/win32_core.c"
#elif OS_LINUX
#    include "os/linux/linux_core.c"
#endif
#include "os/os_core.c"