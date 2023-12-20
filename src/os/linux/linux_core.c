#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>

// NOTE(simon): Don't attempt to close file descriptors multiple times as per `man close.2`.

// NOTE(simon): Ensures that all of the data written to `file_descriptor` is
// flushed from the OSs caches.
internal B32
linux_sync_file_descriptor(S32 file_descriptor)
{
	B32 success = true;

	S32 return_code = 0;
	do
	{
		errno = 0;
		return_code = fsync(file_descriptor);
	} while (return_code == -1 && errno == EINTR);

	if (return_code == -1)
	{
		success = false;
	}

	return success;
}

internal DateTime
linux_date_time_from_tm_and_milliseconds(struct tm *time, U16 milliseconds)
{
	DateTime result = { 0 };

	result.millisecond = milliseconds;
	result.second      = (U8) time->tm_sec;
	result.minute      = (U8) time->tm_min;
	result.hour        = (U8) time->tm_hour;
	result.day         = (U8) (time->tm_mday - 1);
	result.month       = (U8) time->tm_mon;
	result.year        = (S16) (time->tm_year + 1900);

	return(result);
}

internal struct tm
linux_tm_from_date_time(DateTime *date_time)
{
	struct tm result = { 0 };

	result.tm_sec    = date_time->second;
	result.tm_min    = date_time->minute;
	result.tm_hour   = date_time->hour;
	result.tm_mday   = date_time->day + 1;
	result.tm_mon    = date_time->month;
	result.tm_year   = date_time->year - 1900;

	return(result);
}

internal Void *
os_memory_reserve(U64 size)
{
	Void *result = mmap(0, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return(result);
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

internal OS_CircularBuffer
os_circular_buffer_allocate(U64 minimum_size, U64 repeat_count)
{
	B32 success = true;

	S64 page_size = -1;
	if (success)
	{
		page_size = sysconf(_SC_PAGESIZE);
		success &= (page_size != -1);
	}

	// TODO(simon): Rounding could be optimized if `page_size` is guaranteed to
	// be a power of 2.
	U64 size = 0;
	if (success)
	{
		size = (minimum_size + (U64) page_size - 1) / (U64) page_size * (U64) page_size;
	}

	U8 *reserved_addresses = MAP_FAILED;
	if (success)
	{
		reserved_addresses = mmap(0, repeat_count * size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		success &= (reserved_addresses != MAP_FAILED);
	}

	S32 file_descriptor = -1;
	if (success)
	{
		file_descriptor = memfd_create("circular-buffer", 0);
		success &= (file_descriptor != -1);
	}

	if (success)
	{
		success &= (ftruncate(file_descriptor, (off_t) size) != -1);
	}

	if (success)
	{
		for (U64 i = 0; i < repeat_count; ++i)
		{
			U8 *mapping = mmap(reserved_addresses + i * size, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, file_descriptor, 0);
			success &= (mapping != MAP_FAILED);
		}
	}

	if (file_descriptor != -1)
	{
		close(file_descriptor);
	}

	if (!success && reserved_addresses)
	{
		munmap(reserved_addresses, repeat_count * size);
	}

	OS_CircularBuffer result = { 0 };
	if (success)
	{
		result.data         = reserved_addresses;
		result.size         = size;
		result.repeat_count = repeat_count;
	}

	return result;
}

internal Void
os_circular_buffer_free(OS_CircularBuffer circular_buffer)
{
	munmap(circular_buffer.data, circular_buffer.repeat_count * circular_buffer.size);
}

internal B32
os_file_read(Arena *arena, Str8 path, Str8 *result)
{
	assert(arena);
	assert(result);

	B32 success = true;

	Arena_Temporary scratch = arena_get_scratch(&arena, 1);
	Arena_Temporary restore_point = arena_begin_temporary(arena);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	S32 file_descriptor = open(cstr_path, O_RDONLY);
	if (file_descriptor != -1)
	{
		struct stat status = { 0 };
		if (fstat(file_descriptor, &status) != -1)
		{
			result->size = (U64) status.st_size;
			result->data = push_array(arena, U8, result->size);

			U8 *ptr = result->data;
			U8 *opl = result->data + result->size;

			// NOTE(simon): `read` doesn't neccessarily read all data in one
			// call, so we need to loop until all data has been read.
			while (ptr < opl)
			{
				U64 total_to_read = u64_min((U64) (opl - ptr), SSIZE_MAX);
				errno = 0;
				S64 actual_read = read(file_descriptor, ptr, total_to_read);
				if (!(actual_read == -1 && errno == EINTR) && !(actual_read != -1))
				{
					success = false;
					break;
				}

				ptr += actual_read;
			}
		}
		else
		{
			success = false;
		}

		close(file_descriptor);
	}
	else
	{
		success = false;
	}

	// NOTE(simon): If we cannot read the file, free the memory.
	if (!success)
	{
		arena_end_temporary(restore_point);
	}

	arena_release_scratch(scratch);
	return(success);
}

// TODO(simon): What should we do if we can't write the entire file?
internal B32
os_file_write(Str8 path, Str8 data, B32 overwrite_existing)
{
	B32 success = true;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	S32 file_descriptor = open(
		cstr_path,
		O_WRONLY | O_CREAT | (overwrite_existing ? 0 : O_EXCL),
		S_IRUSR | S_IWUSR
	);

	if (file_descriptor != -1)
	{
		U8 *ptr = data.data;
		U8 *opl = data.data + data.size;

		// NOTE(simon): `write` doesn't neccessarily write all data in one
		// call, so we need to loop until all data has been written.
		while (ptr < opl)
		{
			U64 to_write = u64_min((U64) (opl - ptr), SSIZE_MAX);
			errno = 0;
			S64 actual_write = write(file_descriptor, ptr, to_write);
			if (!(actual_write == -1 && errno == EINTR) && !(actual_write != -1))
			{
				success = false;
				break;
			}

			ptr += actual_write;
		}

		success &= linux_sync_file_descriptor(file_descriptor);

		close(file_descriptor);
	}
	else
	{
		success = false;
	}

	arena_release_scratch(scratch);
	return(success);
}

internal B32
os_file_delete(Str8 path)
{
	S32 success = 0;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	// TODO(simon): Not sure if we should keep trying to unlink the path if an
	// IO error occurs.
	do
	{
		errno = 0;
		success = unlink(cstr_path);
	} while (success == -1 && errno == EIO);

	arena_release_scratch(scratch);
	return(success == 0);
}

// TODO(simon): Should we create directories as needed?
// TODO(simon): Cleanup
internal B32
os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing)
{
	S32 success = 0;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_old_path = cstr_from_str8(scratch.arena, old_path);
	CStr cstr_new_path = cstr_from_str8(scratch.arena, new_path);

	// TODO(simon): Should we use O_CLOEXEC and O_NOCTTY?
	S32 old_fd = open(cstr_old_path, O_RDONLY);
	if (old_fd != -1)
	{
		S32 new_fd = open(
			cstr_new_path,
			O_WRONLY | O_CREAT | (overwrite_existing ? 0 : O_EXCL),
			S_IRUSR | S_IWUSR
		);

		if (new_fd != -1)
		{
			// TODO(simon): Check for errors
			struct stat status = { 0 };
			if (fstat(old_fd, &status) == 0)
			{
				ssize_t bytes_to_copy = status.st_size;

				do
				{
					ssize_t bytes_copied = copy_file_range(old_fd, 0, new_fd, 0, (size_t) bytes_to_copy, 0);
					if (bytes_copied == -1)
					{
						// TODO(simon): Indicate error.
						break;
					}
					bytes_to_copy -= bytes_copied;
				} while (bytes_to_copy != 0);

				// TODO(simon): In case we cannot write the entire file, should we leave
				// the file half written or should we delete it?

				// NOTE(simon): Remove any trailing data if the file already existed.
				do
				{
					errno = 0;
					success = ftruncate(new_fd, status.st_size);
				} while (success == -1 && errno == EINTR);

				// TODO(simon): Handle return value.
				linux_sync_file_descriptor(new_fd);
			}
			else
			{
				return(false);
			}

			close(new_fd);
		}
		else
		{
			return(false);
		}

		close(old_fd);
	}
	else
	{
		return(false);
	}

	arena_release_scratch(scratch);
	return(true);
}

// TODO(simon): Make sure that this has the same behaviour as the windows
// backend.
internal B32
os_file_rename(Str8 old_path, Str8 new_path)
{
	B32 success = true;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_old_path = cstr_from_str8(scratch.arena, old_path);
	CStr cstr_new_path = cstr_from_str8(scratch.arena, new_path);

	success = (renameat2(AT_FDCWD, cstr_old_path, AT_FDCWD, cstr_new_path, RENAME_NOREPLACE) == 0);

	arena_release_scratch(scratch);
	return(success);
}

internal B32
os_file_create_directory(Str8 path)
{
	B32 success = true;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	success = (mkdir(cstr_path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == 0);

	arena_release_scratch(scratch);
	return(success);
}

internal B32
os_file_delete_directory(Str8 path)
{
	B32 success = true;
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	success = (rmdir(cstr_path) == 0);

	arena_release_scratch(scratch);
	return(success);
}

internal Void
os_file_iterator_init(OS_FileIterator *iterator, Str8 path)
{
	assert(sizeof(Linux_FileIterator) <= sizeof(OS_FileIterator));

	Arena_Temporary scratch = arena_get_scratch(0, 0);

	Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;
	CStr cstr_path = cstr_from_str8(scratch.arena, path);

	linux_iterator->file_descriptor = open(cstr_path, O_RDONLY | O_DIRECTORY);
	if (linux_iterator->file_descriptor == -1)
	{
		linux_iterator->done = true;
	}

	arena_release_scratch(scratch);
}

internal B32
os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *result_name)
{
	assert(sizeof(Linux_FileIterator) <= sizeof(OS_FileIterator));

	Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;

	while (!linux_iterator->done)
	{
		while (!linux_iterator->done)
		{
			U64 bytes_availible = linux_iterator->write_index - linux_iterator->read_index;

			U64 bytes_needed = sizeof(Linux_Dirent64);
			if (bytes_availible >= sizeof(Linux_Dirent64))
			{
				Linux_Dirent64 *entry = (Linux_Dirent64 *) &linux_iterator->buffer[linux_iterator->read_index];
				bytes_needed = entry->d_reclen;
			}

			if (bytes_availible >= bytes_needed)
			{
				break;
			}
			else
			{
				U64 space_left = array_count(linux_iterator->buffer) - linux_iterator->write_index;

				// NOTE(simon): Do we have enough contiguous memory for the entry?
				if (bytes_availible + space_left < bytes_needed)
				{
					memory_move(linux_iterator->buffer, &linux_iterator->buffer[linux_iterator->read_index], bytes_availible);
					linux_iterator->read_index = 0;
					linux_iterator->write_index = (U32) bytes_availible;

					space_left = array_count(linux_iterator->buffer) - linux_iterator->write_index;
				}

				errno = 0;
				S64 actual_read = getdents64(
					linux_iterator->file_descriptor,
					&linux_iterator->buffer[linux_iterator->write_index],
					space_left
				);

				if (actual_read == -1 && errno == EINVAL)
				{
					// NOTE(simon): Not enough space in the buffer, move all
					// data to the begining and try again.
					memory_move(linux_iterator->buffer, &linux_iterator->buffer[linux_iterator->read_index], bytes_availible);
					linux_iterator->read_index = 0;
					linux_iterator->write_index = (U32) bytes_availible;
				}
				else if (actual_read == -1)
				{
					linux_iterator->done = true;
					// TODO(simon): Report error.
				}
				else if (actual_read == 0)
				{
					linux_iterator->done = true;
				}
				else
				{
					linux_iterator->write_index += (U64) actual_read;
				}
			}
		}

		// NOTE(simon): Do we have an entry or are we done?
		if (!linux_iterator->done)
		{
			Linux_Dirent64 *entry = (Linux_Dirent64 *) &linux_iterator->buffer[linux_iterator->read_index];
			linux_iterator->read_index += entry->d_reclen;

			Str8 name = str8_cstr(entry->d_name);

			// NOTE(simon): Is it the current directory or the parent
			// directory?
			if (!str8_equal(name, str8_lit(".")) && !str8_equal(name, str8_lit("..")))
			{
				*result_name = str8_copy(arena, name);
				break;
			}
		}
	}

	return(!linux_iterator->done);
}

internal Void
os_file_iterator_end(OS_FileIterator *iterator)
{
	assert(sizeof(Linux_FileIterator) <= sizeof(OS_FileIterator));

	Linux_FileIterator *linux_iterator = (Linux_FileIterator *) iterator;

	if (linux_iterator->file_descriptor != -1)
	{
		close(linux_iterator->file_descriptor);
	}
}

internal DateTime
os_now_universal_time(Void)
{
	DateTime universal_date_time = { 0 };

	// NOTE(simon): According to `man gettimeofday.2` this syscall should not fail.
	struct timeval time = { 0 };
	S32 return_code = gettimeofday(&time, 0);
	assert(return_code == -1);

	struct tm deconstructed_time = { 0 };
	if (gmtime_r(&time.tv_sec, &deconstructed_time) == &deconstructed_time)
	{
		universal_date_time = linux_date_time_from_tm_and_milliseconds(
			&deconstructed_time,
			(U16) (time.tv_usec / 1000)
		);
	}
	else
	{
		// NOTE(simon): The year did not fit in an integer.
		// TODO(simon): Handle this case in a nice way.
	}

	return(universal_date_time);
}

internal DateTime
os_now_local_time(Void)
{
	DateTime local_date_time = { 0 };

	// NOTE(simon): According to `man gettimeofday.2` this syscall should not fail.
	struct timeval time = { 0 };
	S32 return_code = gettimeofday(&time, 0);
	assert(return_code == -1);

	struct tm deconstructed_time = { 0 };
	if (localtime_r(&time.tv_sec, &deconstructed_time) == &deconstructed_time)
	{
		local_date_time = linux_date_time_from_tm_and_milliseconds(
			&deconstructed_time,
			(U16) (time.tv_usec / 1000)
		);
	}
	else
	{
		// NOTE(simon): The year did not fit in an integer.
		// TODO(simon): Handle this case in a nice way.
	}

	return(local_date_time);
}

internal DateTime
os_local_time_from_universal(DateTime *date_time)
{
	DateTime local_date_time = { 0 };

	struct tm universal_tm   = linux_tm_from_date_time(date_time);
	// TODO(simon): Handle error
	time_t    universal_time = timegm(&universal_tm);

	struct tm local_tm = { 0 };
	if (localtime_r(&universal_time, &local_tm) == &local_tm)
	{
		local_date_time = linux_date_time_from_tm_and_milliseconds(
			&local_tm,
			date_time->millisecond
		);
	}
	else
	{
		// NOTE(simon): The year did not fit in an integer.
		// TODO(simon): Handle this case in a nice way.
	}

	return(local_date_time);
}

internal DateTime
os_universal_time_from_local(DateTime *date_time)
{
	DateTime universal_date_time = { 0 };

	struct tm local_tm   = linux_tm_from_date_time(date_time);
	// NOTE(simon): `timelocal` is deprecated in favour of `mktime`, see `man timelocal`.
	// TODO(simon): Handle error
	time_t    local_time = mktime(&local_tm);

	struct tm universal_tm = { 0 };
	if (gmtime_r(&local_time, &universal_tm) == &universal_tm)
	{
		universal_date_time = linux_date_time_from_tm_and_milliseconds(
			&universal_tm,
			date_time->millisecond
		);
	}
	else
	{
		// NOTE(simon): The year did not fit in an integer.
		// TODO(simon): Handle this case in a nice way.
	}

	return(universal_date_time);
}

internal U64
os_now_nanoseconds(Void)
{
	// NOTE(simon): According to `man clock_gettime.2` this syscall should not fail.
	struct timespec time = { 0 };
	S32 return_code = clock_gettime(CLOCK_MONOTONIC_RAW, &time);
	assert(return_code == 0);

	U64 nanoseconds = (U64) time.tv_sec * billion(1) + (U64) time.tv_nsec;
	return(nanoseconds);
}

internal Void
os_sleep_milliseconds(U64 time)
{
	struct timespec sleep     = { 0 };
	struct timespec remainder = { 0 };

	sleep.tv_sec  = time / 1000;
	sleep.tv_nsec = (time % 1000) * million(1);

	// NOTE(simon): The only errno we should get from this call is EINTR, so
	// just loop until we succseed.
	while (nanosleep(&sleep, &remainder) == -1)
	{
		sleep = remainder;
		remainder.tv_sec  = 0;
		remainder.tv_nsec = 0;
	}
}

internal OS_Library
os_library_open(Str8 path)
{
	OS_Library result = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	Str8List part_list = { 0 };
	Str8Node parts[3];
	str8_list_push_explicit(&part_list, str8_lit("./"), &parts[0]);
	str8_list_push_explicit(&part_list, path, &parts[1]);
	str8_list_push_explicit(&part_list, str8_lit(".so"), &parts[2]);

	Str8 full_path = str8_join(scratch.arena, &part_list);

	CStr cstr_path = cstr_from_str8(scratch.arena, full_path);

	Void *library = dlopen(cstr_path, RTLD_NOW | RTLD_LOCAL);

	result.u64[0] = int_from_ptr(library);

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

int
main(int argument_count, char *arguments[])
{
	linux_permanent_arena = arena_create();
	arena_init_scratch();

	Str8List argument_list = { 0 };
	for (int i = 0; i < argument_count; ++i)
	{
		str8_list_push(linux_permanent_arena, &argument_list, str8_cstr(arguments[i]));
	}

	S32 exit_code = os_main(argument_list);

	return(exit_code);
}
