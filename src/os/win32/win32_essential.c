global W32_State w32_state; 

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
	return(result);
}

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
os_file_read(Arena *arena, Str8 path, Str8 *result_out)
{
	assert(path.size < MAX_PATH);

	B32 result = false;

	Arena_Temporary scratch = arena_get_scratch(&arena, 1);

	HANDLE file = CreateFile(
	  str16_from_str8(scratch.arena, path).data,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	arena_release_scratch(scratch);

	if (file != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER file_size;
		BOOL get_file_size_result = GetFileSizeEx(file, &file_size);
		if (get_file_size_result)
		{
			assert(file_size.QuadPart <= S32_MAX);
			S32 file_size_s32 = (S32)file_size.QuadPart;
			U32 push_amount = file_size_s32 + 1;
			result_out->data = push_array(arena, U8, push_amount);
			result_out->size = file_size_s32;
			DWORD bytes_read;
			BOOL read_file_result = ReadFile(file, result_out->data, file_size_s32, &bytes_read, 0);
			if (read_file_result)
			{
				result = true;
			}
			else
			{
				arena_pop_amount(arena, push_amount);
				assert(bytes_read == file_size_s32);
			}
		}
		CloseHandle(file);
	}

	return(result);
}

internal B32
os_file_write(Str8 path, Str8 data, B32 overwrite_existing)
{
	assert(path.size < MAX_PATH);

	B32 result = false;

	Arena_Temporary scratch = arena_get_scratch(0, 0);

	// NOTE(hampus): OPEN_ALWAYS create a new file if it does not exist.
	DWORD create_file_flags = OPEN_ALWAYS;
	if (overwrite_existing)
	{
		create_file_flags = CREATE_ALWAYS;
	}

	HANDLE file = CreateFile(
		str16_from_str8(scratch.arena, path).data,
		GENERIC_READ,
		FILE_SHARE_READ,
		0,
		create_file_flags,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (file != INVALID_HANDLE_VALUE)
	{
		assert(data.size <= S32_MAX);
		BOOL write_file_result = WriteFile(file, data.data, (S32)data.size, 0, 0);
		if (write_file_result)
		{
			result = true;
		}
		CloseHandle(file);
	}

	arena_release_scratch(scratch);
	return(result);
}

internal B32
os_file_delete(Str8 path)
{
	assert(path.size < MAX_PATH);
	assert(path.size < MAX_PATH);

	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = DeleteFile(str16_from_str8(scratch.arena, path).data);
	arena_release_scratch(scratch);

	return(result);
}

internal B32
os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing)
{
	assert(old_path.size < MAX_PATH);
	assert(new_path.size < MAX_PATH);

	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str16 old_path16 = str16_from_str8(scratch.arena, old_path);
	Str16 new_path16 = str16_from_str8(scratch.arena, new_path);
	B32 result = CopyFile(old_path16.data, new_path16.data, overwrite_existing);
	arena_release_scratch(scratch);

	return(result);
}

internal B32
os_file_rename(Str8 old_path, Str8 new_path)
{
	assert(old_path.size < MAX_PATH);
	assert(new_path.size < MAX_PATH);

	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str16 old_path16 = str16_from_str8(scratch.arena, old_path);
	Str16 new_path16 = str16_from_str8(scratch.arena, new_path);
	B32 result = MoveFile(old_path16.data, new_path16.data);
	arena_release_scratch(scratch);

	return(result);
}

internal B32
os_file_create_directory(Str8 path)
{
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = CreateDirectory(str16_from_str8(scratch.arena, path).data, 0);
	arena_release_scratch(scratch);
	return(result);
}

internal B32
os_file_delete_directory(Str8 path)
{
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = RemoveDirectory(str16_from_str8(scratch.arena, path).data);
	arena_release_scratch(scratch);
	return(result);
}

internal Void
os_file_iterator_init(OS_FileIterator *iterator, Str8 path)
{
	Str8Node nodes[2];
	Str8List list = { 0 };
	str8_list_push_explicit(&list, path, nodes + 0);
	str8_list_push_explicit(&list, str8_lit("\\*"), nodes + 1);
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str8 path_star = str8_join(scratch.arena, &list);
	Str16 path16 = str16_from_str8(scratch.arena, path_star);
	memory_zero_struct(iterator);
	W32_FileIterator *w32_iter = (W32_FileIterator *)iterator;
	w32_iter->handle = FindFirstFile(path16.data, &w32_iter->find_data);
	arena_release_scratch(scratch);
}

internal B32
os_file_iterator_next(OS_FileIterator *iterator, Str8 *result_name)
{
	B32 result = false;
	W32_FileIterator *w32_iter = (W32_FileIterator *)iterator;
	if (w32_iter->handle != 0 && w32_iter->handle != INVALID_HANDLE_VALUE)
	{
		for (;!w32_iter->done;)
		{
			WCHAR *file_name = w32_iter->find_data.cFileName;
			B32 is_dot = (file_name[0] == '.' && file_name[1] == 0);
			B32 is_dotdot = (file_name[0] == '.' && file_name[1]  == '.' && file_name[2] == 0);

			B32 emit = (!is_dot && !is_dotdot);
			WIN32_FIND_DATAW data = { 0 };
			if (emit)
			{
				memory_copy_struct(&data, &w32_iter->find_data);
				result = true;
			}

			if (!FindNextFile(w32_iter->handle, &w32_iter->find_data))
			{
				w32_iter->done = true;
			}
		}
	}

	return(result);
}

internal Void
os_file_iterator_end(OS_FileIterator *iterator)
{
	W32_FileIterator *w32_iter = (W32_FileIterator *)iterator;
	if (w32_iter->handle != 0 && w32_iter->handle != INVALID_HANDLE_VALUE)
	{
		FindClose(w32_iter->handle);
	}
}

internal OS_Library
os_library_open(Str8 path)
{
	OS_Handle result = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	HMODULE lib = LoadLibrary(str16_from_str8(scratch.arena, path).data);
	arena_release_scratch(scratch);
	if (lib)
	{
		result.u64[0] = int_from_ptr(lib);
	}
	return(result);
}

internal Void
os_library_close(OS_Library handle)
{
	if (!os_handle_is_zero(handle))
	{
		HMODULE lib = ptr_from_int(handle.u64[0]);
		FreeLibrary(lib);
	}
}

internal VoidFunction *
os_library_load_function(OS_Library handle, Str8 name)
{
	VoidFunction *result = 0;
	if (!os_handle_is_zero(handle))
	{
		HMODULE lib = ptr_from_int(handle.u64[0]);
		Arena_Temporary scratch = arena_get_scratch(0, 0);
		result = (VoidFunction *)GetProcAddress(lib, cstr_from_str8(scratch.arena, name));
		arena_release_scratch(scratch);
	}
	return(result);
}

S32 APIENTRY 
WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, int show_code)
{
	QueryPerformanceCounteR(&w32_state.start_counter);	
}