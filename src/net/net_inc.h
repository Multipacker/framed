#ifndef NET_INC_H
#define NET_INC_H

#include "net/net_core.h"

#if OS_WINDOWS
#	include "net/win32/win32_net_core.h"
#elif OS_LINUX
#	include "net/linux/linux_net_core.h"
#else
#	error No net_inc for this OS
#endif

#endif