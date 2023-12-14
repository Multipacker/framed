#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

S32
main(S32 argument_count, CStr arguments[])
{
	Arena *arena = arena_create();
	S32 *x = push_array(arena, S32, 1);
	((U8 *)arena->memory)[100] = 5; // Should crash here
	return(0);
}
