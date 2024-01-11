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

// TODO(simon): Do all of these fields really need to be marked volatile?
typedef struct Log_QueueEntry Log_QueueEntry;
struct Log_QueueEntry
{
	B32       volatile is_valid;
	DateTime  volatile time;
	Log_Level volatile level;
	U8        volatile thread_name[THREAD_CONTEXT_NAME_SIZE];
	CStr      volatile file;
	U32       volatile line;
	U8        volatile message[256];
};

typedef struct Log_EntryBuffer Log_EntryBuffer;
struct Log_EntryBuffer
{
	Log_QueueEntry *buffer;
	U32 volatile count;
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

internal Log_EntryBuffer *log_get_new_entries(Void);

#endif // LOGGING_INC_H
