#if COMPILER_CL
// NOTE(hampus): Compiler warning 4255 complains that functions
// from this header doesn't have void in empty function paramters
#	pragma warning(push, 0)
#	include <sanitizer/asan_interface.h>
#	pragma warning(pop)
#else
#	include <sanitizer/asan_interface.h>
#endif

internal Void *os_memory_reserve(U64 size);
internal Void  os_memory_commit(Void *ptr, U64 size);
internal Void  os_memory_decommit(Void *ptr, U64 size);
internal Void  os_memory_release(Void *ptr, U64 size);

internal Arena *
arena_create_reserve(U64 reserve_size)
{
	// We need to commit one block in order to have space for the arena itself.
	U8 *memory = os_memory_reserve(reserve_size);
	os_memory_commit(memory, ARENA_COMMIT_BLOCK_SIZE);

	Arena *result = (Arena *) memory;

	result->memory     = memory;
	result->cap        = reserve_size;
	// NOTE(simon): This is for the arena itself.
	result->pos        = sizeof(*result);
	result->commit_pos = ARENA_COMMIT_BLOCK_SIZE;

	ASAN_POISON_MEMORY_REGION(result->memory + result->pos, result->commit_pos - result->pos);

	return(result);
}

internal Arena *
arena_create(Void)
{
	Arena *result = arena_create_reserve(ARENA_DEFAULT_RESERVE_SIZE);
	return(result);
}

internal Void
arena_destroy(Arena *arena)
{
	os_memory_release(arena->memory, arena->cap);
}

internal Void *
arena_push(Arena *arena, U64 size)
{
	Void *result = 0;

	if (arena->pos + size <= arena->cap)
	{
		result      = arena->memory + arena->pos;
		arena->pos += size;

		if (arena->pos > arena->commit_pos)
		{
			U64 pos_aligned     = u64_round_up_to_power_of_2(arena->pos, ARENA_COMMIT_BLOCK_SIZE);
			U64 next_commit_pos = u64_min(pos_aligned, arena->cap);
			U64 commit_size     = next_commit_pos - arena->commit_pos;
			os_memory_commit(arena->memory + arena->commit_pos, commit_size);
			ASAN_POISON_MEMORY_REGION((U8 *) arena->memory + arena->commit_pos, commit_size);
			arena->commit_pos = next_commit_pos;
		}

		ASAN_UNPOISON_MEMORY_REGION(result, size);
	}

	return result;
}

internal Void
arena_pop_to(Arena *arena, U64 pos)
{
	// NOTE(simon): Don't pop the arena state from the arena.
	pos = u64_max(pos, sizeof(Arena));

	if (pos < arena->pos)
	{
		U64 dpos = arena->pos - pos;
		arena->pos = pos;

		ASAN_POISON_MEMORY_REGION((U8 *) arena->memory + arena->pos, dpos);

		U64 pos_aligned     = u64_round_up_to_power_of_2(arena->pos, ARENA_COMMIT_BLOCK_SIZE);
		U64 next_commit_pos = u64_min(pos_aligned, arena->cap);
		if (next_commit_pos < arena->commit_pos)
		{
			U64 decommit_size = arena->commit_pos - next_commit_pos;
			os_memory_decommit(arena->memory + next_commit_pos, decommit_size);
			arena->commit_pos = next_commit_pos;
		}
	}
}

internal Void
arena_pop_amount(Arena *arena, U64 amount)
{
	arena_pop_to(arena, arena->pos - amount);
}

internal Void *
arena_push_zero(Arena *arena, U64 size)
{
	Void *result = arena_push(arena, size);
	memory_zero(result, size);
	return result;
}

internal Void
arena_align(Arena *arena, U64 power)
{
	U64 pos_aligned = u64_round_up_to_power_of_2(arena->pos, power);
	U64 align = pos_aligned - arena->pos;
	if (align)
	{
		arena_push(arena, align);
	}
}

internal Void
arena_align_zero(Arena *arena, U64 power)
{
	U64 pos_aligned = u64_round_up_to_power_of_2(arena->pos, power);
	U64 align = pos_aligned - arena->pos;
	if (align)
	{
		arena_push_zero(arena, align);
	}
}

internal Arena_Temporary
arena_begin_temporary(Arena *arena)
{
	Arena_Temporary result;
	result.arena = arena;
	result.pos = arena->pos;

	return result;
}

internal Void
arena_end_temporary(Arena_Temporary temporary)
{
	arena_pop_to(temporary.arena, temporary.pos);
}
