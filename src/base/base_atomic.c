#if COMPILER_CLANG || COMPILER_GCC
#include <stdatomic.h>
#endif

internal U32
u32_atomic_add(U32 volatile *location, U32 amount)
{
    // TODO(simon): Is the memory ordering correct?
#if COMPILER_CLANG || COMPILER_GCC
    U32 result = __atomic_fetch_add(location, amount, memory_order_acq_rel);
#elif COMPILER_CL
    U32 result = InterlockedAdd((LONG *) location, amount) - amount;
#else
#error "There isn't a u32_atomic_add for your compiler."
#endif
    return(result);
}

// TODO(simon): This function should ONLY perform a compare exchange and
// nothing else to be usefull in as many places as possible.
internal B32
u32_atomic_compare_exchange(U32 volatile *location, U32 exchange_value, U32 compare_value)
{
    // TODO(simon): Is the memory ordering correct?
#if COMPILER_CLANG || COMPILER_GCC
    U32 initial_value = compare_value;
    __atomic_compare_exchange_n(location, &initial_value, exchange_value, false, memory_order_acq_rel, memory_order_acquire);
#elif COMPILER_CL
    // NOTE(hampus): Returns the value at location
    U32 initial_value = InterlockedCompareExchange((LONG *) location, exchange_value, compare_value);
#else
#error "There isn't a u32_atomic_compare_exchange for your compiler."
#endif
    return(initial_value == compare_value);
}
