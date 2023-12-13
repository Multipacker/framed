#if COMPILER_CL

void __asan_poison_memory_region(void const volatile *addr, size_t size);
void __asan_unpoison_memory_region(void const volatile *addr, size_t size);

#	if defined(__SANITIZE_ADDRESS__)
#		define ASAN_POISON_MEMORY_REGION(addr, size)   __asan_poison_memory_region((addr), (size))
#		define ASAN_UNPOISON_MEMORY_REGION(addr, size) __asan_unpoison_memory_region((addr), (size))
#	else
#		define ASAN_POISON_MEMORY_REGION(addr, size)
#		define ASAN_UNPOISON_MEMORY_REGION(addr, size)
#	endif 

#elif COMPILER_CLANG

#	define ASAN_POISON_MEMORY_REGION(addr, size)
#	define ASAN_UNPOISON_MEMORY_REGION(addr, size)

#else

// NOTE(hampus): No sanitizer for this compiler

#	define ASAN_POISON_MEMORY_REGION(addr, size)
#	define ASAN_UNPOISON_MEMORY_REGION(addr, size)

#endif


#define THREAD_SCRATCH_ARENA_POOL_SIZE 4
thread_local Arena *thread_scratch_arenas[THREAD_SCRATCH_ARENA_POOL_SIZE];

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

	Arena *result = (Arena *)memory;

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
			ASAN_POISON_MEMORY_REGION((U8 *)arena->memory + arena->commit_pos, commit_size);
			arena->commit_pos = next_commit_pos;
		}

		ASAN_UNPOISON_MEMORY_REGION(result, size);
	}

	return result;
}

internal Void
arena_pop_to(Arena *arena, U64 pos)
{
	if (pos < arena->pos)
	{
		U64 dpos = arena->pos - pos;
		arena->pos = pos;

		ASAN_POISON_MEMORY_REGION((U8 *)arena->memory + arena->pos, dpos);

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

internal Void
arena_init_scratch(Void)
{
	for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i)
	{
		thread_scratch_arenas[i] = arena_create();
	}
}

internal Void
arena_destroy_scratch(Void)
{
	for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i)
	{
		arena_destroy(thread_scratch_arenas[i]);
	}
}

internal Arena_Temporary
arena_get_scratch(Arena **conflicts, U32 count)
{
	Arena *selected = 0;

	for (U32 i = 0; i < array_count(thread_scratch_arenas); ++i)
	{
		Arena *arena = thread_scratch_arenas[i];

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
