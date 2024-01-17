#ifndef DEBUG_INC_H
#define DEBUG_INC_H

typedef struct Debug_Time Debug_Time;
struct Debug_Time
{
	CStr file;
	U32  line;
	CStr name;
	U64  start_ns;
};

#define debug_begin_block(name) debug_time_begin_internal(__FILE__, __LINE__, name)
#define debug_end_block(time)   debug_end(time)
#define debug_block(name)                                                                       \
	for (                                                                                      \
		Debug_Time time##__LINE__ = debug_begin_block(name), *ptime##__LINE__ = &time##__LINE__; \
		ptime##__LINE__;                                                                       \
		debug_end_block(time##__LINE__), ptime##__LINE__ = 0                                    \
	)

#define debug_begin_function()   debug_time_begin_internal(__FILE__, __LINE__, __FUNCTION__)
#define debug_end_function(time) debug_end(time)
#define debug_function()                                                                       \
	for (                                                                                     \
		Debug_Time time##__LINE__ = debug_begin_function(), *ptime##__LINE__ = &time##__LINE__; \
		ptime##__LINE__;                                                                      \
		debug_end_function(time##__LINE__), ptime##__LINE__ = 0                                \
	)

internal Debug_Time debug_begin_internal(CStr file, U32 line, CStr name);
internal Void debug_end(Debug_Time time);

#endif // DEBUG_INC_H
