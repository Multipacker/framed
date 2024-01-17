typedef struct Prof Prof;
struct Prof
{
};

internal Prof_Time
prof_begin_internal(CStr file, U32 line, CStr name)
{
	Prof_Time result = { 0 };

	result.file     = file;
	result.line     = line;
	result.name     = name;
	result.start_ns = os_now_nanoseconds();

	return(result);
}

internal Void
prof_end(Prof_Time time)
{
	U64 end_ns = os_now_nanoseconds();
}
