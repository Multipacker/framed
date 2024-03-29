/*
 * TODO(simon):
 *  [ ] Store the raw data in the user availible buffer and convert it to a
 *      string on demand. Note that this currently crashes inside of
 *      stb_vsnprintf and that thread names need to a deep copy.
 *  [ ] Only wait on the semaphore when it is strictly needed, like when we
 *      have run out of entries.
 */

#define LOG_QUEUE_SIZE (1 << 9)
#define LOG_QUEUE_MASK (LOG_QUEUE_SIZE - 1)

#define LOG_BUFFER_SIZE (1 << 10)
#define LOG_BUFFER_MASK (LOG_BUFFER_SIZE - 1)

typedef struct Logger Logger;
struct Logger
{
    Arena *arena;
    OS_File log_file;

    OS_Semaphore semaphore;

    Log_EntryBuffer buffers[2];
    Log_EntryBuffer *volatile read_buffer;
    Log_EntryBuffer *volatile write_buffer;

    Log_QueueEntry *queue;
    U32 volatile queue_write_index;
    U32 volatile queue_read_index;
};

global Logger global_logger;

internal Str8
log_format_entry(Arena *arena, Log_QueueEntry *entry)
{
    CStr cstr_level = "";
    switch (entry->level)
    {
        case Log_Level_Info:    cstr_level = "INFO";    break;
        case Log_Level_Warning: cstr_level = "WARNING"; break;
        case Log_Level_Error:   cstr_level = "ERROR";   break;
        case Log_Level_Trace:   cstr_level = "TRACE";   break;
        invalid_case;
    }

    Str8 result = str8_pushf(
        arena,
        "%d-%.2u-%.2u %.2u:%.2u:%.2u.%03u %s %s %s:%u: %s\n",
        entry->time.year, entry->time.month + 1, entry->time.day + 1,
        entry->time.hour, entry->time.minute, entry->time.second, entry->time.millisecond,
        cstr_level,
        entry->thread_name,
        entry->file, entry->line,
        entry->message
    );

    return(result);
}

internal Void
log_flusher_thread(Void *argument)
{
    Logger *logger = &global_logger;

    thread_ctx_init(str8_lit("Logger"));

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
            Log_EntryBuffer *buffer = logger->write_buffer;
            if (buffer->count < LOG_BUFFER_SIZE)
            {
                buffer->buffer[buffer->count] = entry;
                memory_fence();
                ++buffer->count;
            }

            arena_scratch(0, 0)
            {
                Str8 log_entry = log_format_entry(scratch, &entry);
                os_file_stream_write(logger->log_file, log_entry);
            }
        }
    }
}

internal Void
log_init(Str8 log_file)
{
    Logger *logger = &global_logger;

    logger->arena = arena_create("LoggerPerm");
    os_semaphore_create(&logger->semaphore, 0);
    logger->buffers[0].buffer = push_array_zero(logger->arena, Log_QueueEntry, LOG_BUFFER_SIZE);
    logger->buffers[1].buffer = push_array_zero(logger->arena, Log_QueueEntry, LOG_BUFFER_SIZE);
    logger->write_buffer = &logger->buffers[0];
    logger->read_buffer  = &logger->buffers[1];
    logger->queue = push_array_zero(logger->arena, Log_QueueEntry, LOG_QUEUE_SIZE);
    os_file_stream_open(log_file, OS_FileMode_Replace, &logger->log_file);

    os_thread_create(log_flusher_thread, 0);
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

    U32 queue_index = u32_atomic_add(&logger->queue_write_index, 1);
    while (queue_index - logger->queue_read_index >= LOG_QUEUE_SIZE)
    {
        // NOTE(simon): The queue is full, so busy wait. This should not be
        // that common.
    }

    Log_QueueEntry *entry = &logger->queue[queue_index & LOG_QUEUE_MASK];

    assert(thread_name.size + 1 <= array_count(entry->thread_name));
    assert(message.size + 1 <= array_count(entry->message));

    entry->time  = time;
    entry->level = level;
    memory_copy((U8 *) entry->thread_name, thread_name.data, thread_name.size);
    entry->thread_name[thread_name.size] = 0;
    entry->file = file;
    entry->line = line;
    memory_copy((U8 *) entry->message, message.data, message.size);
    entry->message[message.size] = 0;

    // NOTE(simon): Make sure all the data of the log entry is written before
    // we mark it as valid.
    memory_fence();

    entry->is_valid = true;
    os_semaphore_signal(&logger->semaphore);

    release_scratch(scratch);
}

internal Log_EntryBuffer *
log_get_new_entries(Void)
{
    Logger *logger = &global_logger;

    logger->read_buffer->count = 0;
    swap(logger->read_buffer, logger->write_buffer, Log_EntryBuffer *);

    Log_EntryBuffer *result = logger->read_buffer;
    return(result);
}
