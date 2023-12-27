#ifndef LOGGING_INC_H
#define LOGGING_INC_H

typedef enum Log_Level Log_Level;
enum Log_Level
{
	Log_Level_Info,
	Log_Level_Warning,
	Log_Level_Error,
	Log_Level_Trace,

	Log_Level_COUNT,
};

typedef struct Log_QueueEntry Log_QueueEntry;
struct Log_QueueEntry
{
	B32 volatile is_valid;
	U8 message[512];
};

internal Void log_init(Str8 log_file);

#define log_info(...)    log_message(Log_Level_Info,    __FILE__, __LINE__, __VA_ARGS__)
#define log_warning(...) log_message(Log_Level_Warning, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)   log_message(Log_Level_Error,   __FILE__, __LINE__, __VA_ARGS__)
#if !BUILD_MODE_RELEASE
#define log_trace(...)   log_message(Log_Level_Trace,   __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_trace(...)
#endif

internal Void log_message(Log_Level level, CStr file, U32 line, CStr format, ...);

internal Log_QueueEntry *log_get_entries(U32 *entry_count);
internal Void log_update_entries(U32 keep_count);

#endif // LOGGING_INC_H
