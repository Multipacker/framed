#include <sys/mman.h>

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
internal B32 os_file_copy(Str8 old_path, Str8 new_path);
internal B32 os_file_rename(Str8 old_path, Str8 new_path);
internal B32 os_file_create_directory(Str8 path);
// NOTE(simon): The directory must be empty.
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_init(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name);
internal Void os_file_iterator_end(OS_FileIterator *iterator);
