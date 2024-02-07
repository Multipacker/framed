#ifndef DEBUG_CORE_H
#define DEBUG_CORE_H

#define DEBUG_TIME_BUFFER_SIZE (1 << 15)
#define DEBUG_MEMORY_BUFFER_SIZE (1 << 16)
#define DEBUG_MEMORY_DELETED U64_MAX

typedef struct Debug_Time Debug_Time;
struct Debug_Time
{
	CStr file;
	U32  line;
	CStr name;
	U64  start_ns;
};

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

typedef struct Debug_MemoryEntry Debug_MemoryEntry;
struct Debug_MemoryEntry
{
	CStr file;
	U32  line;
	// NOTE(simon): Only used to identify which arena we are talking about.
	Arena *arena;
	U64    position;
};

typedef struct Debug_MemoryBuffer Debug_MemoryBuffer;
struct Debug_MemoryBuffer
{
	Debug_MemoryEntry buffer[DEBUG_MEMORY_BUFFER_SIZE];
	U32 volatile started_writes;
	U32 volatile finished_writes;
	U32 count;
};

#define debug_block_begin(name) debug_begin_internal(__FILE__, __LINE__, name)
#define debug_block_end(time)   debug_end(time)
#define debug_block(name)                                                                        \
	for (                                                                                        \
		Debug_Time time##__LINE__ = debug_block_begin(name), *ptime##__LINE__ = &time##__LINE__; \
		ptime##__LINE__;                                                                         \
		debug_block_end(time##__LINE__), ptime##__LINE__ = 0                                     \
	)

#define debug_function_begin()   debug_begin_internal(__FILE__, __LINE__, (CStr) __FUNCTION__)
#define debug_function_end(time) debug_end(time)
#define debug_function()                                                                        \
	for (                                                                                       \
		Debug_Time time##__LINE__ = debug_function_begin(), *ptime##__LINE__ = &time##__LINE__; \
		ptime##__LINE__;                                                                        \
		debug_function_end(time##__LINE__), ptime##__LINE__ = 0                                 \
	)

internal Debug_Time debug_begin_internal(CStr file, U32 line, CStr name);
internal Void       debug_end(Debug_Time time);

internal Debug_TimeBuffer * debug_get_times(Void);

internal Void debug_arena_deleted_internal(CStr file, U32 line, Arena *arena);
internal Void debug_arena_changed_internal(CStr file, U32 line, Arena *arena);

internal Debug_MemoryBuffer * debug_get_memory(Void);

#endif // DEBUG_CORE_H