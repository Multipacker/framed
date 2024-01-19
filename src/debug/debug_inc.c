#define DEBUG_TIME_BUFFER_SIZE (1 << 15)

typedef struct Debug_TimeEntry Debug_TimeEntry;
struct Debug_TimeEntry
{
	CStr file;
	U32  line;
	CStr name;
	U64  start_ns;
	U64  end_ns;
};

typedef struct Debug_TimeBuffer Debug_TimeBuffer;
struct Debug_TimeBuffer
{
	Debug_TimeEntry buffer[DEBUG_TIME_BUFFER_SIZE];
	U32 volatile started_writes;
	U32 volatile finished_writes;
	U32 count;
};

typedef struct Debug Debug;
struct Debug
{
	Debug_TimeBuffer buffers[2];
	Debug_TimeBuffer *write_buffer;
	Debug_TimeBuffer *read_buffer;
};

global Debug global_debug;

internal Void
debug_init(Void)
{
	Debug *debug = &global_debug;
	debug->write_buffer = &debug->buffers[0];
	debug->read_buffer  = &debug->buffers[1];
}

internal Debug_Time
debug_begin_internal(CStr file, U32 line, CStr name)
{
	Debug_Time result = { 0 };

	result.file     = file;
	result.line     = line;
	result.name     = name;
	result.start_ns = os_now_nanoseconds();

	return(result);
}

internal Void
debug_end(Debug_Time time)
{
	U64 end_ns = os_now_nanoseconds();

	Debug *debug = &global_debug;

	Debug_TimeBuffer *buffer = debug->write_buffer;
	U32 index = u32_atomic_add(&buffer->started_writes, 1);
	if (index < DEBUG_TIME_BUFFER_SIZE)
	{
		Debug_TimeEntry *entry = &buffer->buffer[index];
		entry->file     = time.file;
		entry->line     = time.line;
		entry->name     = time.name;
		entry->start_ns = time.start_ns;
		entry->end_ns   = end_ns;
		memory_fence();
	}
	u32_atomic_add(&buffer->finished_writes, 1);
}

internal Debug_TimeBuffer *
debug_get_times(Void)
{
	Debug *debug = &global_debug;

	debug->read_buffer->started_writes  = 0;
	debug->read_buffer->finished_writes = 0;
	debug->read_buffer->count           = 0;

	swap(debug->read_buffer, debug->write_buffer, Debug_TimeBuffer *);

	Debug_TimeBuffer *result = debug->read_buffer;

	while (result->finished_writes != result->started_writes)
	{
		// NOTE(simon): Busy wait for all entries to be written. This should
		// not take long.
	}

	result->count = u32_min(result->finished_writes, DEBUG_TIME_BUFFER_SIZE);

	return(result);
}
