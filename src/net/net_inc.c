#include "net/net_core.c"
#if OS_WINDOWS
#    include "net/win32/win32_net_core.c"
#elif OS_LINUX
#    include "net/linux/linux_net_core.c"
#else
#    error No net_inc for this OS
#endif