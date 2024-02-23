#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

typedef char *CStr;

typedef struct Arena Arena;
struct Arena
{
    U8 *memory;
    U64 cap;
    U64 pos;
    U64 commit_pos;
    Str8 name;
    U64 min_pos;
};

typedef struct Arena_Temporary Arena_Temporary;
struct Arena_Temporary
{
    Arena *arena;
    U64 pos;
};

#define ARENA_DEFAULT_RESERVE_SIZE gigabytes(1)
// TODO(hampus): Test performance with higher/lower commit block size
#define ARENA_COMMIT_BLOCK_SIZE    megabytes(1)

internal Arena *arena_create_reserve(U64 reserve_size, CStr format, ...);
internal Arena *arena_create(CStr format, ...);

internal Void arena_destroy_internal(Arena *arena, CStr file, U32 line);

internal Void *arena_push_internal(Arena *arena, U64 size, CStr file, U32 line);
internal Void  arena_pop_to_internal(Arena *arena, U64 pos, CStr file, U32 line);
internal Void  arena_pop_amount_internal(Arena *arena, U64 amount, CStr file, U32 line);

internal Void *arena_push_zero_internal(Arena *arena, U64 size, CStr file, U32 line);

internal Void arena_align_internal(Arena *arena, U64 power, CStr file, U32 line);
internal Void arena_align_zero_internal(Arena *arena, U64 power, CStr file, U32 line);

internal Arena_Temporary arena_begin_temporary(Arena *arena);
internal Void            arena_end_temporary_internal(Arena_Temporary temporary, CStr file, U32 line);

#define arena_destroy(arena) arena_destroy_internal(arena, __FILE__, __LINE__)

#define arena_push(arena, size)         arena_push_internal(arena, size, __FILE__, __LINE__)
#define arena_pop_to(arena, pos)        arena_pop_to_internal(arena, pos, __FILE__, __LINE__)
#define arena_pop_amount(arena, amount) arena_pop_amount_internal(arena, amount, __FILE__, __LINE__)

#define arena_push_zero(arena, size) arena_push_zero_internal(arena, size, __FILE__, __LINE__)

#define arena_align(arena, power)      arena_align_internal(arena, power, __FILE__, __LINE__)
#define arena_align_zero(arena, power) arena_align_zero_internal(arena, power, __FILE__, __LINE__)
#define arena_end_temporary(temporary) arena_end_temporary_internal(temporary, __FILE__, __LINE__)

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

#define arena_temporary(a)                                             \
    for (                                                              \
        Arena_Temporary temp##__LINE__ = arena_begin_temporary(a);     \
        temp##__LINE__.arena;                                          \
        arena_end_temporary(temp##__LINE__), temp##__LINE__.arena = 0  \
    )

#endif // BASE_MEMORY_H
