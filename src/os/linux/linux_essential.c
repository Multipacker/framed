#include <sys/mman.h>

#include <dlfcn>

internal OS_Handle
os_handle_zero(Void)
{
	OS_Handle result = { 0 };
	return(result);
}

internal B32
os_handle_match(OS_Handle a, OS_Handle b)
{
	B32 result = memory_match(&a, &b, sizeof(OS_Handle));
	return(result);
}

internal B32
os_handle_is_zero(OS_Handle handle)
{
	B32 result = os_handle_match(handle, os_handle_zero());
}

internal Void *
os_memory_reserve(U64 size)
{
	Void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return result;
}
internal Void
os_memory_commit(Void *ptr, U64 size)
{
	mprotect(ptr, size, PROT_READ | PROT_WRITE);
}
internal Void
os_memory_decommit(Void *ptr, U64 size)
{
	mprotect(ptr, size, PROT_NONE);
	madvise(ptr, size, MADV_DONTNEED);
}
internal Void
os_memory_release(Void *ptr, U64 size)
{
	munmap(ptr, size);
}

internal B32 os_file_read(Arena *arena, Str8 path, Str8 *result);
internal B32 os_file_write(Str8 path, Str8 data);

internal B32 os_file_delete(Str8 path);
// NOTE(simon): Moves the file if neccessary and replaces existing files.
internal B32 os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing);
internal B32 os_file_rename(Str8 old_path, Str8 new_path);
internal B32 os_file_create_directory(Str8 path);
// NOTE(simon): The directory must be empty.
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_init(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name);
internal Void os_file_iterator_end(OS_FileIterator *iterator);

internal OS_Handle
os_library_open(Str8 path)
{
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	OS_Handle handle = { 0 };

	CStr cstr_path = cstr_from_str8(scratch.arena, path);
	dlopen(cstr_path, );

	arena_release_scratch(scratch);
	return handle;
}

os_library_close(OS_Handle handle)
{
}

internal VoidFunction *
os_library_load_function(OS_Handle handle, Str8 name)
{
}
