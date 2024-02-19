#include "png.c"

internal B32
image_load(Arena *arena, Str8 contents, Image *result)
{
    B32 success = png_load(arena, contents, result);
    return(success);
}
