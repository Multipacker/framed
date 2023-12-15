#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

internal S32
os_main(Str8List arguments)
{
	assert(false);
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str8 test_string = str8_pushf(scratch.arena, "Test: %d", 5);
	str8_pushf(scratch.arena, "aTest: %d", 5);
	str8_pushf(scratch.arena, "aaTest: %d", 5);
	str8_pushf(scratch.arena, "aaaTest: %d", 5);
	str8_pushf(scratch.arena, "aaaaTest: %d", 5);
	arena_release_scratch(scratch);
	return(0);
}