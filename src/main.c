#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

internal S32
os_main(Str8List arguments)
{
	Arena *arena = arena_create();
	S32 *x = push_array(arena, S32, 1);
	((U8 *)arena->memory)[100] = 5; // Should crash here
#if 0
	DateTime universal_time = os_now_universal_time();
	DateTime local_time = os_now_local_time();

	DateTime local_from_universal = os_local_time_from_universal(&universal_time);
	DateTime universal_from_local = os_universal_time_from_local(&local_time);
#endif
	return(0);
}