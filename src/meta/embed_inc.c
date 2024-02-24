internal Str8
framed_embed_unpack(Arena *arena, U8 *data, U64 size)
{
    Str8 result = str8_copy(arena, str8(data, size));

    return(result);
}
