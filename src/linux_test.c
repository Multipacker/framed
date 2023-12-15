#include "base/base_inc.h"
#include "os/os_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"

internal S32
os_main(Str8List arguments)
{
	// NOTE(simon): Test for os_file_rename
	/*
	if (os_file_rename(str8_lit("test"), str8_lit("hello"))) {
		printf("Done!\n");
	}
	*/

	// NOTE(simon): This doesn't work.
	/*
	OS_FileIterator iterator = { 0 };
	os_file_iterator_init(&iterator, str8_lit("."));
	Str8 name = { 0 };
	while (os_file_iterator_next(&iterator, &name)) {
		printf("%.*s!\n", (int) name.size, name.data);
	}
	os_file_iterator_end(&iterator);
	*/

	Arena_Temporary scratch = arena_get_scratch(0, 0);

	OS_Library lib = os_library_open(str8_lit("lib.so"));
	VoidFunction *func = os_library_load_function(lib, str8_lit("greet"));
	void (*greet)(char *) = (void (*)(char *)) func;
	greet("simon");
	os_library_close(lib);

	arena_end_temporary(scratch);
	return(0);
}
