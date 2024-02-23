#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

#define THREAD_SCRATCH_ARENA_POOL_SIZE 4

#define THREAD_CONTEXT_NAME_SIZE 16

typedef struct ThreadContext ThreadContext;
struct ThreadContext
{
    Arena *permanent_arena;
    Arena *scratch_arenas[THREAD_SCRATCH_ARENA_POOL_SIZE];
    U8 name[THREAD_CONTEXT_NAME_SIZE];
};

internal ThreadContext *thread_ctx_init(Str8 name);

internal ThreadContext *thread_ctx_alloc(Void);
internal Void           thread_ctx_release(ThreadContext *tctx);
internal Void           thread_set_ctx(ThreadContext *tctx);
internal ThreadContext *thread_get_ctx(Void);

internal Void thread_set_name(Str8 string);
internal Str8 thread_get_name(Void);

#define release_scratch(scratch) arena_end_temporary(scratch)

#define arena_scratch(conflicts, count)                                  \
    Arena_Temporary glue(temp,__LINE__) = get_scratch(conflicts, count); \
    for (                                                                \
        Arena *scratch = glue(temp,__LINE__).arena;                      \
        scratch;                                                         \
        release_scratch(glue(temp,__LINE__)), scratch = 0                \
    )

internal Arena_Temporary get_scratch(Arena **conflicts, U32 count);

#endif //BASE_THREAD_CTX_H
