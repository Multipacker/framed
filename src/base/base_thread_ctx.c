internal ThreadContext
thread_ctx_alloc(Void)
{
	ThreadContext result = { 0 };
	for (U32 i = 0; i < array_count(result.scratch_arenas); ++i)
	{
		result.scratch_arenas[i] = arena_create();
	}
	return(result);
}

internal Void
thread_ctx_release(ThreadContext *tctx)
{
	for (U32 i = 0; i < array_count(tctx->scratch_arenas); ++i)
	{
		arena_destroy(tctx->scratch_arenas[i]);
	}
}

internal Void
set_thread_ctx(ThreadContext *tctx)
{
	os_set_tls(tctx);
}

internal ThreadContext *
get_thread_ctx(Void)
{
	ThreadContext *result = os_get_tls();
	return(result);
}

internal Void
set_thread_name(Str8 string)
{
	ThreadContext *ctx = get_thread_ctx();

	assert(string.size + 1 <= array_count(ctx.name));

	memory_copy(ctx->name, string.data, string.size);
	ctx->name[string.size] = 0;
}

internal Str8
get_thread_name(Void)
{
	ThreadContext *ctx = get_thread_ctx();
	Str8 result = str8_cstr((CStr) ctx->name);
	return(result);
}


internal Arena_Temporary
get_scratch(Arena **conflicts, U32 count)
{
	Arena *selected = 0;

	ThreadContext *tctx = get_thread_ctx();

	for (U32 i = 0; i < array_count(tctx->scratch_arenas); ++i)
	{
		Arena *arena = tctx->scratch_arenas[i];

		B32 is_non_conflicting = true;
		for (U32 j = 0; j < count; ++j)
		{
			if (arena == conflicts[j])
			{
				is_non_conflicting = false;
				break;
			}
		}

		if (is_non_conflicting)
		{
			selected = arena;
			break;
		}
	}

	return arena_begin_temporary(selected);
}
