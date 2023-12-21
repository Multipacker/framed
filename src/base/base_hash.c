#define FNV_OFFSET_BASIS_U64 14695981039346656037
#define FNV_PRIME_U64        1099511628211

// NOTE(hampus): This is the FNV hash
internal U64
hash_str8(Str8 string)
{
    U64 result = FNV_OFFSET_BASIS_U64;

    for (U64 i = 0; i < string.size; ++i)
    {
        result *= FNV_PRIME_U64;
        result ^= string.data[i];
    }

    return(result);
}
