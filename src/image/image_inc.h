#ifndef IMAGE_INC_H
#define IMAGE_INC_H

typedef struct Image Image;
struct Image
{
	U32 width;
	U32 height;
	U8 *pixels;
	Render_ColorSpace color_space;
};

internal B32 image_load(Arena *arena, Str8 contents, Image *result);

#include "png.h"

#endif // IMAGE_INC_H
