#ifndef BASE_THREAD_CTX_H
#define BASE_THREAD_CTX_H

#define THREAD_SCRATCH_ARENA_POOL_SIZE 4

typedef struct ThreadContext ThreadContext;
struct ThreadContext
{
    Arena *scratch_arenas[THREAD_SCRATCH_ARENA_POOL_SIZE];
    char thread_name[512];
};

internal ThreadContext  thread_ctx_alloc(Void);
internal Void           thread_ctx_release(ThreadContext *tctx);
internal Void           set_thread_ctx(ThreadContext *tctx);
internal ThreadContext *get_thread_ctx(Void);

#define release_scratch(scratch) arena_end_temporary(scratch);

internal Arena_Temporary get_scratch(Arena **conflicts, U32 count);

#endif //BASE_THREAD_CTX_H
