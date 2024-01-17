#ifndef DEBUG_INC_H
#define DEBUG_INC_H

typedef struct Prof_Time Prof_Time;
struct Prof_Time
{
	CStr file;
	U32  line;
	CStr name;
	U64  start_ns;
};

#define prof_begin_block(name) prof_time_begin_internal(__FILE__, __LINE__, name)
#define prof_end_block(prof)   prof_end(prof)
#define prof_block(name)                                                                       \
	for (                                                                                      \
		Prof_Time prof##__LINE__ = prof_begin_block(name), *pprof##__LINE__ = &prof##__LINE__; \
		pprof##__LINE__;                                                                       \
		prof_end_block(prof##__LINE__), pprof##__LINE__ = 0                                    \
	)

#define prof_begin_function()   prof_time_begin_internal(__FILE__, __LINE__, __FUNCTION__)
#define prof_end_function(prof) prof_end(prof)
#define prof_function()                                                                       \
	for (                                                                                     \
		Prof_Time prof##__LINE__ = prof_begin_function(), *pprof##__LINE__ = &prof##__LINE__; \
		pprof##__LINE__;                                                                      \
		prof_end_function(prof##__LINE__), pprof##__LINE__ = 0                                \
	)

internal Prof_Time prof_begin_internal(CStr file, U32 line, CStr name);
internal Void prof_end(Prof_Time time);

#endif // DEBUG_INC_H
