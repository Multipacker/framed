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

typedef struct Log_Entry Log_Entry;
struct Log_Entry
{
	Log_Entry *next;
	Log_Entry *prev;
	Log_Level level;
	DateTime time;
	Str8 message;
};

typedef struct Log_EntryList Log_EntryList;
struct Log_EntryList
{
	Log_Entry *first;
	Log_Entry *last;
	U64 count;
};

internal Void log_init(Str8 log_file);
internal Void log_flush(Void);

#define log_info(...)    log_message(Log_Level_Info,    __FILE__, __LINE__, __VA_ARGS__)
#define log_warning(...) log_message(Log_Level_Warning, __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...)   log_message(Log_Level_Error,   __FILE__, __LINE__, __VA_ARGS__)
#if !BUILD_MODE_RELEASE
#define log_trace(...)   log_message(Log_Level_Trace,   __FILE__, __LINE__, __VA_ARGS__)
#else
#define log_trace(...)
#endif

internal Void log_message(Log_Level level, CStr file, U32 line, CStr format, ...);

internal Log_EntryList log_get_entries(Arena *arena);

#endif // LOGGING_INC_H
