#include <sys/mman.h>
#include <sys/stat.h>

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

// NOTE(simon): Don't attempt to close file descriptors multiple times as per `man close.2`.

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
	return result;
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
internal B32 os_file_write(Str8 path, Str8 data, B32 overwrite_existing);

internal B32
os_file_delete(Str8 path)
{
	S32 success = 0;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	// TODO(simon): Not sure if we should keep trying to unlink the path if an
	// IO error occurs.
	do {
		errno = 0;
		success = unlink(cstr_path);
	} while (success == -1 && errno == EIO);

	arena_release_scratch(scratch);
	return(success == 0);
}

// TODO(simon): Should we create directories as needed?
internal B32
os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing)
{
	S32 success = 0;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_old_path = cstr_from_str8(scratch.arena, old_path);
	CStr cstr_new_path = cstr_from_str8(scratch.arena, new_path);

	// TODO(simon): Should we use O_CLOEXEC and O_NOCTTY?
	// TODO(simon): Check for errors
	S32 old_fd = open(cstr_old_path, O_RDONLY);

	// TODO(simon): Check for errors
	S32 new_fd = open(
		cstr_new_path,
		O_WRONLY | O_CREAT | (overwrite_existing ? 0 : O_EXCL),
		S_IRUSR | S_IWUSR
	);

	// TODO(simon): Check for errors
	struct stat status = { 0 };
	fstat(old_fd, &status);
	ssize_t bytes_to_copy = status.st_size;

	// TODO(simon): May want to make sure this doesn't happend
	//              EINVAL fd_in and fd_out refer to the same file and the
	//              source and target ranges overlap.
	ssize_t bytes_copied = 0;
	errno = 0;
	do {
		bytes_copied = copy_file_range(old_fd, 0, new_fd, 0, (size_t) bytes_to_copy, 0);
		if (bytes_copied != -1) {
			bytes_to_copy -= bytes_copied;
		}
	} while ((bytes_copied == -1 && errno == EIO) || bytes_to_copy != 0);

	// TODO(simon): In case we cannot write the entire file, should we leave
	// the file half written or should we delete it?

	// TODO(simon): Check for errors
	ftruncate(new_fd, status.st_size);

	// NOTE(simon): Ensure that all of the data is written to disk.
	do {
		errno = 0;
		success = fsync(new_fd);
	} while (success == -1 && (errno == EINTR || errno == EIO));

	close(old_fd);
	close(new_fd);

	arena_release_scratch(scratch);
	return(bytes_copied != -1 && success != -1);
}

internal B32 os_file_rename(Str8 old_path, Str8 new_path);
internal B32 os_file_create_directory(Str8 path);
internal B32 os_file_delete_directory(Str8 path);

internal Void os_file_iterator_init(OS_FileIterator *iterator, Str8 path);
internal B32  os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name);
internal Void os_file_iterator_end(OS_FileIterator *iterator);

internal OS_Library
os_library_open(Str8 path)
{
	OS_Library result = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);
	result.u64[0] = int_from_ptr(dlopen(cstr_path, RTLD_NOW | RTLD_LOCAL));

	arena_release_scratch(scratch);
	return(result);
}

internal Void
os_library_close(OS_Library library)
{
	if (library.u64[0])
	{
		Void *handle = ptr_from_int(library.u64[0]);
		dlclose(handle);
	}
}

internal VoidFunction *
os_library_load_function(OS_Library library, Str8 name)
{
	VoidFunction *result = 0;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	if (library.u64[0])
	{
		CStr  cstr_name = cstr_from_str8(scratch.arena, name);
		Void *handle    = ptr_from_int(library.u64[0]);

		result = dlsym(handle, cstr_name);
	}

	arena_release_scratch(scratch);
	return(result);
}

global Arena *linux_permanent_arena;

int main(int argument_count, char *arguments[]) {
	linux_permanent_arena = arena_create();
	arena_init_scratch();

	Str8List argument_list = { 0 };
	for (int i = 0; i < argument_count; ++i) {
		str8_list_push(linux_permanent_arena, &argument_list, str8_cstr(arguments[i]));
	}

	S32 exit_code = os_main(argument_list);

	return(exit_code);
}
