global Win32_State win32_state;

internal void
win32_print_error_message(Void)
{
	DWORD error = GetLastError();
	U8 buffer[1024] = { 0 };
	U64 size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
														0, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &buffer, 1024, 0);
	OutputDebugStringA((LPCSTR) buffer);
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

internal OS_CircularBuffer
os_circular_buffer_allocate(U64 minimum_size, U64 repeat_count)
{
	OS_CircularBuffer result = { 0 };

	// TODO(simon): Implement.

	return(reuslt);
}

internal Void
os_circular_buffer_free(OS_CircularBuffer circular_buffer)
{
	// TODO(simon): Implement.
}

internal B32
os_file_read(Arena *arena, Str8 path, Str8 *result_out)
{
	assert(path.size < MAX_PATH);

	B32 result = false;

	Arena_Temporary scratch = arena_get_scratch(&arena, 1);

	HANDLE file = CreateFile(cstr16_from_str8(scratch.arena, path).data,
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
			U32 file_size_u32 = (U32) file_size.QuadPart;
			U32 push_amount = file_size_u32 + 1;
			result_out->data = push_array(arena, U8, push_amount);
			result_out->size = file_size_u32;
			DWORD bytes_read;
			BOOL read_file_result = ReadFile(file, result_out->data, file_size_u32, &bytes_read, 0);
			if (read_file_result)
			{
				if (bytes_read == file_size_u32)
				{
					result = true;
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				arena_pop_amount(arena, push_amount);
				win32_print_error_message();
			}
		}
		else
		{
			win32_print_error_message();
		}
		CloseHandle(file);
	}
	else
	{
		win32_print_error_message();
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

	HANDLE file = CreateFile(cstr16_from_str8(scratch.arena, path).data,
														 GENERIC_READ,
														 FILE_SHARE_READ,
														 0,
														 create_file_flags,
														 FILE_ATTRIBUTE_NORMAL,
														 0);

	if (file != INVALID_HANDLE_VALUE)
	{
		assert(data.size <= U32_MAX);
		BOOL write_file_result = WriteFile(file, data.data, (S32) data.size, 0, 0);
		if (write_file_result)
		{
			result = true;
		}
		else
		{
			win32_print_error_message();
		}
		CloseHandle(file);
	}
	else
	{
		win32_print_error_message();
	}

	arena_release_scratch(scratch);
	return(result);
}

internal B32
os_file_delete(Str8 path)
{
	assert(path.size < MAX_PATH);

	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = DeleteFile(cstr16_from_str8(scratch.arena, path).data);
	arena_release_scratch(scratch);

	return(result);
}

internal B32
os_file_copy(Str8 old_path, Str8 new_path, B32 overwrite_existing)
{
	assert(old_path.size < MAX_PATH);
	assert(new_path.size < MAX_PATH);

	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str16 old_path16 = cstr16_from_str8(scratch.arena, old_path);
	Str16 new_path16 = cstr16_from_str8(scratch.arena, new_path);
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
	Str16 old_path16 = cstr16_from_str8(scratch.arena, old_path);
	Str16 new_path16 = cstr16_from_str8(scratch.arena, new_path);
	B32 result = MoveFile(old_path16.data, new_path16.data);
	arena_release_scratch(scratch);

	return(result);
}

internal B32
os_file_create_directory(Str8 path)
{
	assert(path.size < MAX_PATH);
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = CreateDirectory(cstr16_from_str8(scratch.arena, path).data, 0);
	arena_release_scratch(scratch);
	return(result);
}

internal B32
os_file_delete_directory(Str8 path)
{
	assert(path.size < MAX_PATH);
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	B32 result = RemoveDirectory(cstr16_from_str8(scratch.arena, path).data);
	arena_release_scratch(scratch);
	return(result);
}

internal Str16
win32_str16_from_wchar(WCHAR *wide_char)
{
	Str16 result = { 0 };
	result.data = wide_char;
	while (*wide_char++)
	{
		result.size++;
	}
	return(result);
}

internal Void
os_file_iterator_init(OS_FileIterator *iterator, Str8 path)
{
	assert(path.size < MAX_PATH);
	Str8Node nodes[2];
	Str8List list = { 0 };
	str8_list_push_explicit(&list, path, nodes + 0);
	str8_list_push_explicit(&list, str8_lit("\\*"), nodes + 1);
	Arena_Temporary scratch = arena_get_scratch(0, 0);
	Str8 path_star = str8_join(scratch.arena, &list);
	Str16 path16 = cstr16_from_str8(scratch.arena, path_star);
	memory_zero_struct(iterator);
	Win32_FileIterator *win32_iter = (Win32_FileIterator *) iterator;
	win32_iter->handle = FindFirstFile(path16.data, &win32_iter->find_data);
	arena_release_scratch(scratch);
}

internal B32
os_file_iterator_next(Arena *arena, OS_FileIterator *iterator, Str8 *result_name)
{
	B32 result = false;
	Win32_FileIterator *win32_iter = (Win32_FileIterator *) iterator;
	if (win32_iter->handle != 0 && win32_iter->handle != INVALID_HANDLE_VALUE)
	{
		for (;!win32_iter->done;)
		{
			WCHAR *file_name = win32_iter->find_data.cFileName;
			B32 is_dot = (file_name[0] == '.' && file_name[1] == 0);
			B32 is_dotdot = (file_name[0] == '.' && file_name[1]  == '.' && file_name[2] == 0);
			B32 emit = (!is_dot && !is_dotdot);
			WIN32_FIND_DATAW data = { 0 };
			if (emit)
			{
				memory_copy_struct(&data, &win32_iter->find_data);
			}

			if (!FindNextFile(win32_iter->handle, &win32_iter->find_data))
			{
				win32_iter->done = true;
			}

			if (emit)
			{
				*result_name = str8_from_str16(arena, win32_str16_from_wchar(data.cFileName));
				result = true;
				break;
			}
		}
	}
	return(result);
}

internal Void
os_file_iterator_end(OS_FileIterator *iterator)
{
	Win32_FileIterator *win32_iter = (Win32_FileIterator *) iterator;
	if (win32_iter->handle != 0 && win32_iter->handle != INVALID_HANDLE_VALUE)
	{
		FindClose(win32_iter->handle);
	}
}

internal DateTime
win32_date_time_from_system_time(SYSTEMTIME *system_time)
{
	DateTime result = { 0 };
	result.millisecond = system_time->wMilliseconds;
	result.second      = (U8) system_time->wSecond;
	result.minute      = (U8) system_time->wMinute;
	result.hour        = (U8) system_time->wHour;
	result.day         = (U8) system_time->wDay;
	result.month       = (U8) system_time->wMonth;
	result.year        = (S16) system_time->wYear;
	return(result);
}

internal SYSTEMTIME
win32_system_time_from_date_time(DateTime *date_time)
{
	SYSTEMTIME result = { 0 };
	result.wMilliseconds = (WORD) date_time->millisecond;
	result.wSecond       = (WORD) date_time->second;
	result.wMinute       = (WORD) date_time->minute;
	result.wHour         = (WORD) date_time->hour;
	result.wDay          = (WORD) date_time->day;
	result.wMonth        = (WORD) date_time->month;
	result.wYear         = (WORD) date_time->year;
	return(result);
}

internal DateTime
os_now_universal_time(Void)
{
	SYSTEMTIME universal_time = { 0 };
	GetSystemTime(&universal_time);
	DateTime result = win32_date_time_from_system_time(&universal_time);
	return(result);
}

internal DateTime
os_now_local_time(Void)
{
	SYSTEMTIME local_time = { 0 };
	GetLocalTime(&local_time);
	DateTime result = win32_date_time_from_system_time(&local_time);
	return(result);
}

internal DateTime
os_local_time_from_universal(DateTime *date_time)
{
	TIME_ZONE_INFORMATION time_zone_information;
	GetTimeZoneInformation(&time_zone_information);
	SYSTEMTIME universal_time = win32_system_time_from_date_time(date_time);
	SYSTEMTIME local_time;
	SystemTimeToTzSpecificLocalTime(&time_zone_information, &universal_time, &local_time);
	DateTime result = win32_date_time_from_system_time(&local_time);
	return(result);
}

internal DateTime
os_universal_time_from_local(DateTime *date_time)
{
	TIME_ZONE_INFORMATION time_zone_information;
	GetTimeZoneInformation(&time_zone_information);
	SYSTEMTIME local_time = win32_system_time_from_date_time(date_time);
	SYSTEMTIME universal_time;
	TzSpecificLocalTimeToSystemTime(&time_zone_information, &local_time, &universal_time);
	DateTime result = win32_date_time_from_system_time(&universal_time);
	return(result);
}

internal U64
os_now_nanoseconds(Void)
{
	U64 result = 0;
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	// NOTE(hampus): Convert from s -> us for less precision loss
	counter.QuadPart *= million(1);
	counter.QuadPart /= win32_state.frequency.QuadPart;
	// NOTE(hampus): Convert from us -> ns
	result = counter.QuadPart * thousand(1);
	return(result);
}

internal Void
os_sleep_milliseconds(U64 time)
{
	assert(time < (U64) U32_MAX);
	Sleep((DWORD) time);
}

internal OS_Library
os_library_open(Str8 path)
{
	OS_Library result = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	Str8List part_list = { 0 };
	Str8Node parts[2];
	str8_list_push_explicit(&part_list, path, &parts[0]);
	str8_list_push_explicit(&part_list, str8_lit(".dll"), &parts[1]);

	Str8 full_path = str8_join(scratch.arena, &part_list);

	HMODULE lib = LoadLibrary(cstr16_from_str8(scratch.arena, full_path).data);
	arena_release_scratch(scratch);
	if (lib)
	{
		result.u64[0] = int_from_ptr(lib);
	}
	return(result);
}

internal Void
os_library_close(OS_Library library)
{
	if (library.u64[0])
	{
		HMODULE lib = ptr_from_int(library.u64[0]);
		FreeLibrary(lib);
	}
}

internal VoidFunction *
os_library_load_function(OS_Library library, Str8 name)
{
	VoidFunction *result = 0;
	if (library.u64[0])
	{
		HMODULE lib = ptr_from_int(library.u64[0]);
		Arena_Temporary scratch = arena_get_scratch(0, 0);
		result = (VoidFunction *) GetProcAddress(lib, cstr_from_str8(scratch.arena, name));
		arena_release_scratch(scratch);
	}
	return(result);
}

internal S32
win32_common_main(Void)
{
	timeBeginPeriod(0);
	QueryPerformanceFrequency(&win32_state.frequency);
	win32_state.permanent_arena = arena_create();
	arena_init_scratch();
	// NOTE(hampus): 'command_line' handed to WinMain doesn't include the program name
	LPSTR command_line_with_exe_path = GetCommandLineA();
	Str8List argument_list = str8_split_by_codepoints(win32_state.permanent_arena, str8_cstr(command_line_with_exe_path), str8_lit(" "));
	S32 exit_code = os_main(argument_list);
	return(exit_code);
}

#if BUILD_MODE_RELEASE
S32 APIENTRY
WinMain(HINSTANCE instance, HINSTANCE prev_instance, PSTR command_line, int show_code)
{
	S32 exit_code = win32_common_main();
	ExitProcess(exit_code);
}

#else

S32
main(S32 argument_count, CStr arguments[])
{
	S32 exit_code = win32_common_main();
	return(exit_code);
}

#endif
