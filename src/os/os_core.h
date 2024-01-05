#ifndef OS_ESSENTIAL_H
#define OS_ESSENTIAL_H

// TODO(simon): File properties

// NOTE(simon): Determines what happens if a file already exists when trying to
// create a new one with the same name.
typedef enum OS_FileMode OS_FileMode;
enum OS_FileMode
{
	OS_FileMode_Fail,
	OS_FileMode_Replace,
	OS_FileMode_Append,

	OS_FileMode_COUNT,
};

typedef struct OS_File OS_File;
struct OS_File
{
	U64 u64[1];
};

typedef struct OS_FileIterator OS_FileIterator;
struct OS_FileIterator
{
	U8 data[1024];
};

typedef struct OS_Library OS_Library;
struct OS_Library
{
	U64 u64[1];
};

typedef struct OS_CircularBuffer OS_CircularBuffer;
struct OS_CircularBuffer
{
	U8 *data;
	U64 size;
	U64 repeat_count;
	U64 handle;
};

typedef struct OS_Semaphore OS_Semaphore;
typedef struct OS_Mutex     OS_Mutex;

typedef Void ThreadProc(Void *);

#define os_handle_is_valid(handle) (!type_is_zero(handle))

internal Void *os_memory_reserve(U64 size);
internal Void  os_memory_commit(Void *ptr, U64 size);
internal Void  os_memory_decommit(Void *ptr, U64 size);
internal Void  os_memory_release(Void *ptr, U64 size);

internal Void  os_set_tls(Void *memory);
internal Void *os_get_tls(Void);

internal OS_CircularBuffer os_circular_buffer_allocate(U64 minimum_size, U64 repeat_count);
internal Void              os_circular_buffer_free(OS_CircularBuffer circular_buffer);

internal B32 os_file_read(Arena *arena, Str8 path, Str8 *result);
internal B32 os_file_write(Str8 path, Str8 data, OS_FileMode mode);

internal B32 os_file_stream_open(Str8 path, OS_FileMode mode, OS_File *result);
internal B32 os_file_stream_write(OS_File file, Str8 data);
internal B32 os_file_stream_close(OS_File file);

internal B32 os_file_delete(Str8 path);
// NOTE(simon): Moves the file if necessary and replaces existing files.
internal B32 os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing);
// NOTE(hampus): Only renames if new_path doesn't already exist
internal B32 os_file_rename(Str8 old_path, Str8 new_path);
internal B32 os_file_create_directory(Str8 path);
// NOTE(simon): The directory must be empty.
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_init(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *result_name);
internal Void os_file_iterator_end(OS_FileIterator *iterator);

internal DateTime os_now_universal_time(Void);
internal DateTime os_now_local_time(Void);
internal DateTime os_local_time_from_universal(DateTime *date_time);
internal DateTime os_universal_time_from_local(DateTime *date_time);

internal U64  os_now_nanoseconds(Void);
internal Void os_sleep_milliseconds(U64 time);

internal OS_Library    os_library_open(Str8 path);
internal Void          os_library_close(OS_Library library);
internal VoidFunction *os_library_load_function(OS_Library library, Str8 name);

internal Void os_semaphore_create(OS_Semaphore *handle, U32 initial_value);
internal Void os_semaphore_destroy(OS_Semaphore *handle);
internal Void os_semaphore_signal(OS_Semaphore *handle);
internal Void os_semaphore_wait(OS_Semaphore *handle);

internal Void os_mutex_create(OS_Mutex *mutex);
internal Void os_mutex_take(OS_Mutex *mutex);
internal Void os_mutex_release(OS_Mutex *mutex);

internal B32 os_run(Str8 program, Str8List arguments);

internal Void os_thread_create(ThreadProc *proc, Void *data);

#define os_mutex(mutex) defer_loop(os_mutex_take(mutex), os_mutex_release(mutex))

internal S32 os_main(Str8List arguments);

#endif // OS_ESSENTIAL_H
