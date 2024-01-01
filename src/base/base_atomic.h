#ifndef BASE_ATOMIC_H
#define BASE_ATOMIC_H

internal U32 u32_atomic_add(U32 volatile *location, U32 amount);
internal B32 u32_atomic_compare_exchange(U32 volatile *location, U32 exchange_value, U32 compare_value);

// NOTE(simon): This hinders the compiler from reordering operations around the
// point this is used.
#if COMPILER_CLANG || COMPILER_GCC
#define memory_fence() asm("" ::: "memory")
#elif COMPILER_CL
#define memory_fence() MemoryBarrier()
#else
#error "There isn't a memory fence defined for this compiler."
#endif

#endif // BASE_ATOMIC_H
