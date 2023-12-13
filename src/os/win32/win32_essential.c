internal Void *
os_memory_reserve(U64 size)
{
	
}

internal Void
os_memory_commit(Void *ptr, U64 size)
{
	
}

internal Void 
os_memory_decommit(Void *ptr, U64 size)
{
	
}

internal Void  
os_memory_release(Void *ptr, U64 size)
{
	
}

internal B32 
os_file_read(Arena *arena, Str8 path, Str8 *result)
{
	
}

internal B32 
os_file_write(Str8 path, Str8 data)
{
	
}

internal B32 os_file_delete(Str8 path);
internal B32 os_file_copy(Str8 old_path, Str8 new_path);
internal B32 os_file_rename(Str8 old_path, Str8 new_path);
internal B32 os_file_create_directory(Str8 path);
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_init(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name);
internal Void os_file_iterator_end(OS_FileIterator *iterator);