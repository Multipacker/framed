// NOTE(simon): Deliberately using small values to test the circular buffer
#define LOGGER_MINIMUM_BUFFER_SIZE 300 /* megabytes(1) */
#define LOGGER_MINIMUM_FLUSH_SIZE  200 /* kilobytes(4) */

typedef struct Logger Logger;
struct Logger
{
	OS_CircularBuffer buffer;
	U64 write_index;
	U64 read_index;

	OS_File log_file;
};

global Logger global_logger;

internal Void
log_init(Str8 log_file)
{
	Logger *logger = &global_logger;

	logger->buffer = os_circular_buffer_allocate(LOGGER_MINIMUM_BUFFER_SIZE, 3);
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

	U8 *start = &logger->buffer.data[logger->read_index % logger->buffer.size];
	U8 *opl   = &logger->buffer.data[logger->write_index % logger->buffer.size];

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

	DateTime time = os_now_local_time();

	Arena *arena = arena_create();

	// NOTE(simon): Format the users message.
	va_list args;
	va_start(args, format);
	Str8 message = str8_pushfv(arena, format, args);
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
		arena,
		"[%s] %d-%.2u-%.2u %.2u:%.2u:%.2u.%u %s:%u: %"PRISTR8"\n",
		cstr_level,
		time.year, time.month + 1, time.day + 1,
		time.hour, time.minute, time.second, time.millisecond,
		file, line,
		str8_expand(message)
	);

	assert(log_entry.size < logger->buffer.size);

	U64 write_index = logger->write_index;
	logger->write_index += log_entry.size;

	memory_copy(&logger->buffer.data[write_index % logger->buffer.size], log_entry.data, log_entry.size);

	if (logger->write_index - logger->read_index > LOGGER_MINIMUM_FLUSH_SIZE)
	{
		U8 *start = &logger->buffer.data[logger->read_index % logger->buffer.size];
		U8 *opl = start + LOGGER_MINIMUM_FLUSH_SIZE;
		U8 *end = start + logger->write_index;

		// NOTE(simon): We might be midway through an entry, search for the end of it.
		while (opl < end && opl[-1] != '\n')
		{
			++opl;
		}

		os_file_stream_write(logger->log_file, str8_range(start, opl));

		logger->read_index += (U64) (opl - start);
	}

	arena_destroy(arena);
}
