typedef struct Debug Debug;
struct Debug
{
};

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
}
