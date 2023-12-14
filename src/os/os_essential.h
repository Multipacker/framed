#ifndef OS_ESSENTIAL_H
#define OS_ESSENTIAL_H

// TODO(simon): File properties

typedef struct OS_FileIterator OS_FileIterator;
struct OS_FileIterator
{
	U8 data[512];
};

typedef struct OS_Handle OS_Handle;
struct OS_Handle
{
	U64 u64[1];
};

typedef struct OS_Library OS_Library;
struct OS_Library 
{
	U64 u64[1];
};

internal OS_Handle os_handle_zero(Void);
internal B32       os_handle_match(OS_Handle a, OS_Handle b);
internal B32       os_handle_is_zero(OS_Handle handle);

internal Void *os_memory_reserve(U64 size);
internal Void  os_memory_commit(Void *ptr, U64 size);
internal Void  os_memory_decommit(Void *ptr, U64 size);
internal Void  os_memory_release(Void *ptr, U64 size);

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

internal DateTime os_now_universal_time(Void);
internal DateTime os_local_time_from_universal(DateTime *date_time);
internal DateTime os_universal_time_from_local(DateTime *date_time);

internal U64  os_now_nanoseconds(Void);
internal Void os_sleep_milliseconds(U64 time);

internal OS_Library    os_library_open(Str8 path);
internal Void          os_library_close(OS_Library handle);
internal VoidFunction *os_library_load_function(OS_Library handle, Str8 name);

internal S32 os_main(Str8List arguments);

#endif // OS_ESSENTIAL_H
