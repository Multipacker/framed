internal Void *
os_memory_alloc(U64 size)
{
    Void *result = os_memory_reserve(size);
    os_memory_commit(result, size);
    return(result);
}
