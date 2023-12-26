#include <stdatomic.h>
#include <semaphore.h>

typedef struct OS_Semaphore OS_Semaphore;
struct OS_Semaphore
{
	sem_t semaphore;
};

internal Void
os_semaphore_create(OS_Semaphore *handle, U32 initial_value)
{
	assert(initial_value < SEM_VALUE_MAX);

	sem_init(&handle->semaphore, 0, initial_value);
}

internal Void
os_semaphore_destroy(OS_Semaphore *handle)
{
	sem_destroy(&handle->semaphore);
}

internal Void
os_semaphore_signal(OS_Semaphore *handle)
{
	sem_post(&handle->semaphore);
}

internal Void
os_semaphore_wait(OS_Semaphore *handle)
{
	int success = 0;
	do
	{
		errno = 0;
		success = sem_wait(&handle->semaphore);
	} while (success == -1 && errno == EINTR);
}

typedef Void *ThreadProc(Void *);

internal Void
os_create_thread(ThreadProc *proc)
{
	pthread_t thread;
	pthread_create(&thread, 0, proc, 0);
}

internal U32
u32_atomic_add(U32 volatile *location, U32 amount)
{
	// TODO(simon): Is the memory ordering correct?
#if COMPILER_CLANG || COMPILER_GCC
	U32 result = __atomic_fetch_add(location, amount, memory_order_acq_rel);
#else
#error "There isn't a u32_atomic_add for your compiler."
#endif
	return(result);
}

// NOTE(simon): This hinders the compiler from reordering operations around the
// point this is used.
#if COMPILER_CLANG || COMPILER_GCC
#define memory_fence() asm("" ::: "memory")
#else
#error "There isn't a memory fence defined for this compiler."
#endif



#define LOG_QUEUE_SIZE (1 << 9)
#define LOG_QUEUE_MASK (LOG_QUEUE_SIZE - 1)

#define LOG_BUFFER_SIZE (1 << 13)
#define LOG_BUFFER_MASK (LOG_BUFFER_SIZE - 1)

typedef struct Logger Logger;
struct Logger
{
	Arena *arena;
	OS_File log_file;

	OS_Semaphore semaphore;

	Log_QueueEntry *buffer;
	U32 volatile buffer_write_index;
	U32 volatile buffer_read_index;

	Log_QueueEntry *queue;
	U32 volatile queue_write_index;
	U32 volatile queue_read_index;
};

global Logger global_logger;

internal Void *
log_flusher_proc(Void *argument)
{
	Logger *logger = &global_logger;

	for (;;)
	{
		// TODO(simon): Only wait if we don't have an entries to do.
		os_semaphore_wait(&logger->semaphore);

		if (logger->queue_write_index - logger->queue_read_index != 0)
		{
			Log_QueueEntry *waiting_entry = &logger->queue[logger->queue_read_index & LOG_QUEUE_MASK];
			while (!waiting_entry->is_valid)
			{
				// NOTE(simon): Busy wait for the entry to become valid.
			}
			waiting_entry->is_valid = false;
			Log_QueueEntry entry = *waiting_entry;
			memory_fence();
			++logger->queue_read_index;

			// NOTE(simon): Potentially save the entry for later retrieval.
			if (logger->buffer_write_index < LOG_BUFFER_SIZE)
			{
				logger->buffer[logger->buffer_write_index] = entry;
				memory_fence();
				// TODO(simon): There is a possibility that
				// `log_update_entries` is called *here*, causing the increment
				// to point one past the valid entries.
				++logger->buffer_write_index;
			}

			os_file_stream_write(logger->log_file, str8_cstr((CStr) entry.message));
		}
	}

	return(0);
}

internal Void
log_init(Str8 log_file)
{
	Logger *logger = &global_logger;

	logger->arena = arena_create();
	os_semaphore_create(&logger->semaphore, 0);
	logger->buffer = push_array_zero(logger->arena, Log_QueueEntry, LOG_BUFFER_SIZE);
	logger->queue = push_array_zero(logger->arena, Log_QueueEntry, LOG_QUEUE_SIZE);
	os_file_stream_open(log_file, OS_FileMode_Replace, &logger->log_file);

	os_create_thread(log_flusher_proc);
}

internal Void
log_message(Log_Level level, CStr file, U32 line, CStr format, ...)
{
	Logger *logger = &global_logger;

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
		"%d-%.2u-%.2u %.2u:%.2u:%.2u.%03u %s %"PRISTR8" %s:%u: %"PRISTR8"\n",
		time.year, time.month + 1, time.day + 1,
		time.hour, time.minute, time.second, time.millisecond,
		cstr_level,
		str8_expand(thread_name),
		file, line,
		str8_expand(message)
	);

	U32 queue_index = u32_atomic_add(&logger->queue_write_index, 1);
	while (queue_index - logger->queue_read_index >= LOG_QUEUE_SIZE)
	{
		// NOTE(simon): The queue is full, so busy wait. This should not be
		// that common.
	}

	Log_QueueEntry *entry = &logger->queue[queue_index & LOG_QUEUE_MASK];

	assert(log_entry.size + 1 <= array_count(entry->message));

	memory_copy(entry->message, log_entry.data, log_entry.size);
	entry->message[log_entry.size] = 0;

	// NOTE(simon): Make sure all the data of the log entry is written before
	// we mark it as valid.
	memory_fence();

	entry->is_valid = true;
	os_semaphore_signal(&logger->semaphore);

	release_scratch(scratch);
}

internal Log_QueueEntry *
log_get_entries(U32 *entry_count)
{
	Logger *logger = &global_logger;

	*entry_count = logger->buffer_write_index;

	return(logger->buffer);
}

internal Void
log_update_entries(U32 keep_count)
{
	Logger *logger = &global_logger;

	U32 current_count = logger->buffer_write_index;
	U32 to_keep = u32_min(keep_count, current_count);

	memory_move(logger->buffer, &logger->buffer[current_count - to_keep], to_keep * sizeof(*logger->buffer));
	memory_fence();
	logger->buffer_write_index = to_keep;
}
