#ifndef BASE_ATOMIC_H
#define BASE_ATOMIC_H

internal U32 u32_atomic_add(U32 volatile *location, U32 amount);

// NOTE(simon): This hinders the compiler from reordering operations around the
// point this is used.
#if COMPILER_CLANG || COMPILER_GCC
#define memory_fence() asm("" ::: "memory")
#else
#error "There isn't a memory fence defined for this compiler."
#endif

#endif // BASE_ATOMIC_H
