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
internal Void debug_end(Debug_Time time);

#endif // DEBUG_INC_H
