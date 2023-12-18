/* TODO(hampus):
- [ ] Subpixel rendering
 - [ ] Premultiplied alpha
- [ ] Kerning
- [ ] Underline & strikethrough
- [ ] Font atlas & cache
*/

// NOTE(hampus): Freetype have variables called 'internal' :(
#undef internal
#include "freetype/freetype.h"
#define internal static

internal R_Font *
render_font_init_freetype(Arena *arena, R_Context *renderer, Str8 path)
{
    R_Font *result = 0;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    FT_Library ft;
    // NOTE(hampus): 0 indicates a success in freetype, otherwise error
    B32 ft_init_error = FT_Init_FreeType(&ft);
        if (!ft_init_error)
    {
        S32 major_version, minor_version, patch;
        FT_Library_Version(ft, &major_version, &minor_version, &patch);

        Str8 file_read_result = {0};
        if (os_file_read(scratch.arena, path, &file_read_result))
        {
            FT_Open_Args open_args = {0};
            open_args.flags = FT_OPEN_MEMORY;
            open_args.memory_base = file_read_result.data;
            assert(file_read_result.size <= U32_MAX);
            open_args.memory_size = (U32)file_read_result.size;
            FT_Face face;
             B32 ft_open_face_error = FT_Open_Face(ft, &open_args, 0, &face);
            if (!ft_open_face_error)
            {
                result = push_struct(arena, R_Font);
                F32 size = 10;
                B32 ft_set_pixel_sizes_error = FT_Set_Pixel_Sizes(face, 0, (U32)size);
                if (!ft_set_pixel_sizes_error)
                {
                    F32 pixels_per_EM = size / (F32)face->units_per_EM;

                    result->line_height         = face->height             * pixels_per_EM;
                    result->underline_position  = face->underline_position * pixels_per_EM;
                    result->max_advance_width   = face->max_advance_width  * pixels_per_EM;
                    result->max_ascent          = face->ascender           * pixels_per_EM;
                    result->max_descent         = face->descender          * pixels_per_EM;
                    result->has_kerning         = FT_HAS_KERNING(face);
                    result->family_name         = str8_pushf(arena, "%s", face->family_name);
                    result->style_name          = str8_pushf(arena, "%s", face->style_name);
                    result->size_in_pixels      = size;

                    for (U32 glyph_index = 33; glyph_index < 128; ++glyph_index)
                    {
                        R_Glyph *glyph = result->glyphs + glyph_index;
                        B32 ft_load_glyph_error = FT_Load_Char(face, glyph_index, FT_LOAD_DEFAULT);
                        if (!ft_load_glyph_error)
                        {
                            B32 ft_render_glyph_error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
                            if (!ft_render_glyph_error)
                            {
                                S32 bitmap_width = face->glyph->bitmap.width;
                                S32 bitmap_height = face->glyph->bitmap.rows;
                                S32 bearing_left = face->glyph->bitmap_left;
                                S32 bearing_top = face->glyph->bitmap_top;

                                // TODO(hampus): SIMD
                                U8 *new_texture_data = push_array(arena, U8, (bitmap_height * bitmap_width * 4));
                                U32 *dst = (U32 *)new_texture_data;
                                U8 *src = face->glyph->bitmap.buffer;
                                for (S32 i = 0; i < (bitmap_height*bitmap_width); ++i)
                                {
                                    U8 val = src[i];
                                    *dst++ =
                                    (0xff <<  0) |
                                    (0xff <<  8) |
                                    (0xff << 16) |
                                    (val << 24);
                                }

                                glyph->slice.texture = render_create_texture_from_bitmap(renderer, (U8 *)new_texture_data, bitmap_width, bitmap_height, R_TextureFormat_Linear);
                                glyph->slice.region.max = v2f32(1, 1);
                                glyph->size_in_pixels = v2f32((F32)bitmap_width, (F32)bitmap_height);
                                glyph->bearing_in_pixels = v2f32((F32)bearing_left, (F32)bearing_top);
                                glyph->advance_width = (F32)(face->glyph->advance.x >> 6);
                            }
                            else
                            {
                                // TODO(hampus): Logging
                            }
                        }
                        else
                        {
                            // TODO(hampus): Logging
                        }
                    }
                }
                else
                {
                    // TODO(hampus): Logging
                }

                FT_Done_Face(face);
            }
            else
            {
                // TODO(hampus): Logging
            }
        }
        else
        {
            // TODO(hampus): Logging
        }

        FT_Done_FreeType(ft);
    }
    else
    {
        // TODO(hampus): Logging
    }

    arena_release_scratch(scratch);
    return(result);
}

internal R_Font *
render_font_init_stb_truetype(Arena *arena, R_Context *renderer, Str8 path)
{
    R_Font *result = 0;
    return(result);
}

internal R_Font *
render_font_init(Arena *arena, R_Context *renderer, Str8 path)
{
    R_Font *result = render_font_init_freetype(arena, renderer, path);
    return(result);
}

internal void
render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color)
{
	for (U64 i = 0; i < text.size; ++i)
	{
		R_Glyph *glyph = font->glyphs + text.data[i];
		F32 xpos = min.x + glyph->bearing_in_pixels.x;
        F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

        F32 width = (F32)glyph->size_in_pixels.x;
        F32 height = (F32)glyph->size_in_pixels.y;

        render_rect(renderer,
                    v2f32(xpos, ypos),
                    v2f32(xpos + width,
                      ypos + height),
                    .slice = glyph->slice,
                    .color = color);
        min.x += (glyph->advance_width);
	}
}

internal Vec2F32
render_measure_text(R_Font *font, Str8 text)
{
	Vec2F32 result = { 0 };
	return(result);
}
