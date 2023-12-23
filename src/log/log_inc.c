// NOTE(simon): Deliberately using small values to test the circular buffer
#define LOGGER_MINIMUM_BUFFER_SIZE megabytes(1)
#define LOGGER_MINIMUM_FLUSH_SIZE  kilobytes(4)

typedef struct Logger Logger;
struct Logger
{
	OS_CircularBuffer buffer;
	U8 *data;
	U64 write_index;
	U64 oldest_entry;

	U64 flush_start;
	U64 next_flush_mark;

	OS_File log_file;
};

global Logger global_logger;

internal Void
log_init(Str8 log_file)
{
	Logger *logger = &global_logger;

	logger->buffer = os_circular_buffer_allocate(LOGGER_MINIMUM_BUFFER_SIZE, 3);
	logger->data = logger->buffer.data + logger->buffer.size;
	logger->next_flush_mark = LOGGER_MINIMUM_FLUSH_SIZE;
	os_file_stream_open(log_file, OS_FileMode_Replace, &logger->log_file);
}

internal Void
log_flush(Void)
{
	Logger *logger = &global_logger;

	// NOTE(simon): Do we anything to flush?
	if (!logger->buffer.data)
	{
		return;
	}

	U8 *start = &logger->data[logger->flush_start % logger->buffer.size];
	U8 *opl   = &logger->data[logger->write_index % logger->buffer.size];
	if (opl < start)
	{
		opl += logger->buffer.size;
	}

	os_file_stream_write(logger->log_file, str8_range(start, opl));

	os_file_stream_close(logger->log_file);
	os_circular_buffer_free(logger->buffer);
}

internal Void
log_message(Log_Level level, CStr file, U32 line, CStr format, ...)
{
	Logger *logger = &global_logger;

	// NOTE(simon): Can we log anything?
	if (!logger->buffer.data)
	{
		return;
	}

	Arena_Temporary scratch = get_scratch(0, 0);

	Str8 thread_name = thread_get_name();
	DateTime time = os_now_local_time();

	// NOTE(simon): Format the users message.
	va_list args;
	va_start(args, format);
	Str8 message = str8_pushfv(scratch.arena, format, args);
	va_end(args);

	CStr cstr_level = "";
	switch (level)
	{
		case Log_Level_Info:    cstr_level = "INFO";    break;
		case Log_Level_Warning: cstr_level = "WARNING"; break;
		case Log_Level_Error:   cstr_level = "ERROR";   break;
		case Log_Level_Trace:   cstr_level = "TRACE";   break;
		invalid_case;
	}

	Str8 log_entry = str8_pushf(
		scratch.arena,
		"[%s] %d-%.2u-%.2u %.2u:%.2u:%.2u.%u %"PRISTR8"@%s:%u: %"PRISTR8"\n",
		cstr_level,
		time.year, time.month + 1, time.day + 1,
		time.hour, time.minute, time.second, time.millisecond,
		str8_expand(thread_name),
		file, line,
		str8_expand(message)
	);

	assert(log_entry.size < logger->buffer.size);

	U64 write_index = logger->write_index;
	logger->write_index += log_entry.size;

	if (logger->write_index - logger->oldest_entry > logger->buffer.size)
	{
		while (logger->oldest_entry < logger->write_index && logger->data[logger->oldest_entry % logger->buffer.size - 1] != '\n')
		{
			++logger->oldest_entry;
		}
	}

	memory_copy(&logger->data[write_index % logger->buffer.size], log_entry.data, log_entry.size);

	if (log_entry.size > logger->next_flush_mark - write_index)
	{
		U8 *start = &logger->data[logger->flush_start % logger->buffer.size];
		U8 *opl   = &logger->data[(write_index + log_entry.size) % logger->buffer.size];
		if (opl < start)
		{
			opl += logger->buffer.size;
		}

		os_file_stream_write(logger->log_file, str8_range(start, opl));

		logger->flush_start     = write_index + log_entry.size;
		logger->next_flush_mark = logger->flush_start + LOGGER_MINIMUM_FLUSH_SIZE;
	}

	release_scratch(scratch);
}

internal Log_EntryList
log_get_entries(Arena *arena)
{
	Log_EntryList entries = { 0 };

	Logger *logger = &global_logger;

	U8 *oldest = &logger->data[logger->oldest_entry % logger->buffer.size];
	U8 *opl    = &logger->data[logger->write_index  % logger->buffer.size];
	if (oldest > opl)
	{
		oldest -= logger->buffer.size;
	}

	while (opl > oldest)
	{
		U8 *start = opl - 1;
		while (start > oldest && start[-1] != '\n')
		{
			--start;
		}

		if (start >= oldest)
		{
			Log_Entry *entry = push_struct_zero(arena, Log_Entry);
			entry->message = str8_copy(arena, str8_range(start, opl - 1));
			dll_push_back(entries.first, entries.last, entry);
		}

		opl = start;
	}

	return(entries);
}
