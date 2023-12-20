internal Void
log_init(Str8 log_file)
{
}

internal Void
log_flush(Void)
{
}

internal Void
log_message(Log_Level level, CStr file, U32 line, CStr format, ...)
{
	DateTime time = os_now_local_time();

	Arena *arena = arena_create();

	va_list args;
	va_start(args, format);

	Str8 message = str8_pushfv(arena, format, args);
	va_end(args);

	printf("%"PRISTR8"\n", str8_expand(message));
	//printf("%"PRISTR8"\n", str8_expand(message));

	arena_destroy(arena);
}
