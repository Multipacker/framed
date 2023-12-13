#define UNICODE

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

internal Void *
os_memory_reserve(U64 size)
{
	Void *result = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
	return(result);
}

internal Void
os_memory_commit(Void *ptr, U64 size)
{
	VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
}

internal Void
os_memory_decommit(Void *ptr, U64 size)
{
	VirtualFree(ptr, size, MEM_DECOMMIT);
}

internal Void
os_memory_release(Void *ptr, U64 size)
{
	VirtualFree(ptr, 0, MEM_RELEASE);
}

internal B32
os_file_read(Arena *arena, Str8 path, Str8 *result)
{
	B32 success = true;

	Arena_Temporary scratch = arena_get_scratch(&arena, 1);

	HANDLE file = CreateFile(
		str16_from_str8(scratch.arena, path).data,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (file == INVALID_HANDLE_VALUE)
	{
		// TODO(hampus): Logging
		success = false;
		goto end;
	}

	arena_release_scratch(scratch);

	LARGE_INTEGER file_size;
	if (!GetFileSizeEx(file, &file_size))
	{
		// TODO(hampus): Logging
		success = false;
		goto end;
	}

	assert(file_size.QuadPart <= S32_MAX);

	S32 file_size_s32 = (S32)file_size.QuadPart;
	U32 push_amount = file_size_s32 + 1;

	result->data = push_array(arena, U8, push_amount);
	result->size = file_size_s32;

	DWORD bytes_read;

	if (!ReadFile(file, result->data, file_size_s32, &bytes_read, 0))
	{
		// TODO(hampus): Logging
		arena_pop_amount(arena, push_amount);
		success = false;
		goto end;
	}

	assert(bytes_read == file_size_s32);

	CloseHandle(file);

end:
	return(success);
}

internal B32
os_file_write(Str8 path, Str8 data)
{
	B32 success = true;

	Arena_Temporary scratch = arena_get_scratch(0, 0);

	HANDLE file = CreateFile(
		str16_from_str8(scratch.arena, path).data,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (file == INVALID_HANDLE_VALUE)
	{
		success = false;
		goto end;
	}

	arena_release_scratch(scratch);

		

end:
	return(success);
}

internal B32
os_file_delete(Str8 path)
{
	B32 success = true;

	return(success);
}

internal B32
os_file_copy(Str8 old_path, Str8 new_path)
{
	B32 success = true;

	return(success);
}

internal B32
os_file_rename(Str8 old_path, Str8 new_path)
{
	B32 success = true;

	return(success);
}

internal B32
os_file_create_directory(Str8 path)
{
	B32 success = true;

	return(success);
}

internal B32
os_file_delete_directory(Str8 path)
{
	B32 success = true;

	return(success);
}

internal Void
os_file_iterator_init(OS_FileIterator *iterator, Str8 path)
{
}

internal B32
os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name)
{
	B32 success = true;

	return(success);
}

internal Void
os_file_iterator_end(OS_FileIterator *iterator)
{
}