#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H 

typedef struct Arena Arena;
struct Arena
{
	U8 *memory;
	U64 cap;
	U64 pos;
	U64 commit_pos;
};

typedef struct Arena_Temporary Arena_Temporary;
struct Arena_Temporary
{
	Arena *arena;
	U64 pos;
};

#define ARENA_DEFAULT_RESERVE_SIZE gigabytes(1)
#define ARENA_COMMIT_BLOCK_SIZE    megabytes(64)

internal Arena *arena_create_reserve(U64 reserve_size);
internal Arena *arena_create(Void);

internal Void arena_destroy(Arena *arena);

internal Void *arena_push(Arena *arena, U64 size);
internal Void  arena_pop_to(Arena *arena, U64 pos);
internal Void  arena_pop_amount(Arena *arena, U64 amount);

internal Void *arena_push_zero(Arena *arena, U64 size);

internal Void arena_align(Arena *arena, U64 power);
internal Void arena_align_zero(Arena *arena, U64 power);

internal Arena_Temporary arena_begin_temporary(Arena *arena);
internal Void            arena_end_temporary(Arena_Temporary temporary);

internal Void            arena_create_scratch(Void);
internal Void            arena_destroy_scratch(Void);
internal Arena_Temporary arena_get_scratch(Arena **conflicts, U32 count);

#define arena_release_scratch(scratch) arena_end_temporary(scratch);

#define push_struct(arena, type)            ((type *) arena_push(arena, sizeof(type)))
#define push_struct_zero(arena, type)       ((type *) arena_push_zero(arena, sizeof(type)))
#define push_array(arena, type, count)      ((type *) arena_push(arena, sizeof(type)*(count)))
#define push_array_zero(arena, type, count) ((type *) arena_push_zero(arena, sizeof(type)*(count)))

#define memory_zero(destionation, size)       memset((destionation), 0, (size))
#define memory_zero_struct(destination)       memory_zero((destination), sizeof(*(destination)))
#define memory_zero_array(destination)        memory_zero((destination), sizeof(destination))
#define memory_zero_typed(destination, count) memory_zero((destination), sizeof(*(destination)) * (count))

#define memory_match(a, b, size) (memcmp((a), (b), (size)) == 0)
#define type_is_zero(x) (memcmp(&x, (U8 [sizeof(x)]) { 0 }, sizeof(x)) == 0)

#define memory_move(destination, source, size)        memmove((destination), (source), size)
#define memory_copy(destination, source, size)        memcpy((destination), (source), size)
#define memory_copy_struct(destination, source)       memory_copy((destination), (source), u64_min(sizeof(*(destination)), sizeof(*(source))))
#define memory_copy_array(destination, source) 	      memory_copy((destination), (source), u64_min(sizeof(destination), sizeof(source)))
#define memory_copy_typed(destination, source, count) memory_copy((destination), (source), u64_min(sizeof(*(destination)), sizeof(*(source)))*(count))

#endif // BASE_MEMORY_H
