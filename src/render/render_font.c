// TODO(hampus):
   // [x] - Subpixel rendering
// [ ] - Kerning
// [ ] - Underline & strikethrough
// [ ] - Font atlas & cache

// NOTE(hampus): Freetype have variables called 'internal' :(
#undef internal
#include "freetype/freetype.h"
#define internal static

internal Str8
render_get_ft_error_message(FT_Error err)
{
#undef FTERRORS_H_
#define FT_ERRORDEF( e, v, s )  case e: return(str8_lit(s));
#define FT_ERROR_START_LIST     switch (err) {
#define FT_ERROR_END_LIST       }
#include FT_ERRORS_H
    return(str8_lit("(Unknown error)"));
}

internal B32
render_make_glyph(Arena *arena, R_Context *renderer, R_Font *font, FT_Face face, U32 glyph_index, R_FontRenderMode render_mode)
{
    S32 bitmap_height = 0;
    S32 bearing_left  = 0;
    S32 bearing_top   = 0;
    S32 bitmap_width  = 0;
    U8 *texture_data = 0;

    B32 result = true;
    Str8 error;
    R_Glyph *glyph = font->glyphs + glyph_index;

    // NOTE(hampus): Get the width of a space
    FT_Int32 ft_load_flags = 0;
    FT_Render_Mode ft_render_flags = 0;
    switch (render_mode)
    {
        case R_FontRenderMode_Normal:
        {
            ft_load_flags = FT_LOAD_DEFAULT;
            ft_render_flags = FT_RENDER_MODE_NORMAL;
        } break;
        case R_FontRenderMode_LCD:
        {
            ft_load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LCD;
            ft_render_flags = FT_RENDER_MODE_LCD;
        } break;
        invalid_case;
    }

    FT_Error ft_load_glyph_error = FT_Load_Char(face, glyph_index, ft_load_flags);
    if (!ft_load_glyph_error)
    {
        FT_Error ft_render_glyph_error = FT_Render_Glyph(face->glyph, ft_render_flags);
        if (!ft_render_glyph_error)
        {
            switch (render_mode)
            {
                case R_FontRenderMode_Normal:
                {
                    bitmap_height = (S32) face->glyph->bitmap.rows;
                    bearing_left  = face->glyph->bitmap_left;
                    bearing_top   = face->glyph->bitmap_top;
                    bitmap_width  = (S32) face->glyph->bitmap.width;

                    // TODO(hampus): SIMD (or check that the compiler actually SIMD's this)
                    // NOTE(hampus): Convert from 8 bit to 32 bit
                    texture_data = push_array(arena, U8, (U64) (bitmap_height * bitmap_width * 4));
                    U32 *dst = (U32 *)texture_data;
                    U8 *src = face->glyph->bitmap.buffer;
                    for (S32 i = 0; i < (bitmap_height*bitmap_width); ++i)
                    {
                        U8 val = src[i];
                        *dst++ = (U32) (
							(0xff <<  0) |
							(0xff <<  8) |
							(0xff << 16) |
							(val  << 24)
						);
                    }
                } break;

                case R_FontRenderMode_LCD:
                {
                    bitmap_height = (S32) face->glyph->bitmap.rows;
                    bearing_left  = face->glyph->bitmap_left;
                    bearing_top   = face->glyph->bitmap_top;
                    // NOTE(hampus): We now have 3 "pixels" for each value.
                    bitmap_width = face->glyph->bitmap.width / 3;

                    // TODO(hampus): SIMD (or check that the compiler actually SIMD's this)
                    // NOTE(hampus): Convert from 24 bit RGB to 32 bit RGBA
                    texture_data = push_array(arena, U8, (U64) (bitmap_height * bitmap_width * 4));
                    U8 *dst = texture_data;
                    U8 *src = face->glyph->bitmap.buffer;
                    for (S32 y = 0; y < bitmap_height; ++y)
                    {
                        U8 *dst_row = dst;
                        U8 *src_row = src;
                        for (S32 x = 0; x < bitmap_width; ++x)
                        {
                            U8 r = *src_row++;
                            U8 g = *src_row++;
                            U8 b = *src_row++;
                            *dst_row++ = r;
                            *dst_row++ = g;
                            *dst_row++ = b;
                            *dst_row++ = 0xff;
                        }

                        dst += (S32)(bitmap_width * 4);

                        // NOTE(hampus): Freetype actually adds padding
                        // so the pitch is the correct width to increment
                        // by.
                        src += (S32)(face->glyph->bitmap.pitch);
                    }
                } break;
                invalid_case;
                }
            }
            else
            {
            }
        }
        else
        {
        }

    glyph->slice.texture     = render_create_texture_from_bitmap(renderer, texture_data, bitmap_width, bitmap_height, R_ColorSpace_Linear);
    glyph->slice.region.max  = v2f32(1, 1);
    glyph->size_in_pixels    = v2f32((F32)bitmap_width, (F32)bitmap_height);
    glyph->bearing_in_pixels = v2f32((F32)bearing_left, (F32)bearing_top);
    glyph->advance_width     = (F32)(face->glyph->advance.x >> 6);

    return(result);
}

internal R_Font *
render_font_init_freetype(Arena *arena, R_Context *renderer, Str8 path, R_FontRenderMode render_mode)
{
    R_Font *result = 0;
    Arena_Temporary scratch = arena_get_scratch(&arena, 1);

    FT_Library ft;
    // NOTE(hampus): 0 indicates a success in freetype, otherwise error
    Str8 error;
    FT_Error ft_init_error = FT_Init_FreeType(&ft);
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
			FT_Error ft_open_face_error = FT_Open_Face(ft, &open_args, 0, &face);
            if (!ft_open_face_error)
            {
                result = push_struct(arena, R_Font);
                F32 font_size = 18;
                // TODO(hampus): Get the monitor's DPI
                 F32 dpi = 96;
                FT_Error ft_set_pixel_sizes_error = FT_Set_Char_Size(face, (U32)font_size << 6, (U32)font_size << 6, (U32)dpi, (U32)dpi);
                if (!ft_set_pixel_sizes_error)
                {
                    F32 points_per_inch = 72;
                    F32 em_per_font_unit = 1.0f / (F32)face->units_per_EM;
                    F32 points_per_font_unit = em_per_font_unit / font_size;
                    F32 inches_per_font_unit = points_per_font_unit / points_per_inch;
                    F32 pixels_per_font_unit = inches_per_font_unit * (F32)dpi;

                    result->line_height         = face->height             * pixels_per_font_unit;
                    result->underline_position  = face->underline_position * pixels_per_font_unit;
                    result->max_advance_width   = face->max_advance_width  * pixels_per_font_unit;
                    result->max_ascent          = face->ascender           * pixels_per_font_unit;
                    result->max_descent         = face->descender          * pixels_per_font_unit;
                    result->has_kerning         = FT_HAS_KERNING(face);
                    result->family_name         = str8_copy_cstr(arena, (U8 *)face->family_name);
                    result->style_name          = str8_copy_cstr(arena, (U8 *)face->style_name);
                    result->font_size           = font_size;

                    {
                        // NOTE(hampus): Get the width of a space
                        FT_Int32 ft_load_flags = 0;
                        switch (render_mode)
                        {
                            case R_FontRenderMode_Normal: ft_load_flags = FT_LOAD_DEFAULT; break;
                            case R_FontRenderMode_LCD:    ft_load_flags = FT_LOAD_RENDER | FT_LOAD_TARGET_LCD; break;
                            case R_FontRenderMode_LCD_V:  assert(false); break;
                        }

                        FT_Error ft_load_glyph_error = FT_Load_Char(face, ' ', ft_load_flags);
                        if (!ft_load_glyph_error)
                        {
                            result->space_width = (F32) face->glyph->linearHoriAdvance / 65536.0f;
                            // FT_Done_Glyph(face->glyph);
                        }
                        else
                        {
                            // TODO(hampus): Failed to load glyph
                            error = render_get_ft_error_message(ft_set_pixel_sizes_error);
                        }
                    }

                    for (U32 glyph_index = 33; glyph_index < 128; ++glyph_index)
                    {
                        if (render_make_glyph(arena, renderer, result, face, glyph_index, render_mode))
                        {
                            // TODO(hampus): We don't have the declaration for
                            // this function for some reason
                            // FT_Done_Glyph(face->glyph);
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
                    error = render_get_ft_error_message(ft_set_pixel_sizes_error);
                }

                FT_Done_Face(face);
            }
            else
            {
                // TODO(hampus): Logging
                error = render_get_ft_error_message(ft_open_face_error);
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
        error = render_get_ft_error_message(ft_init_error);
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
    // TODO(hampus): Check the pixel orientation of the monitor.
    R_Font *result = render_font_init_freetype(arena, renderer, path, R_FontRenderMode_LCD);
    return(result);
}

internal void
render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color)
{
	for (U64 i = 0; i < text.size; ++i)
	{
        if (text.data[i] == ' ')
        {
            min.x += font->space_width;
        }
        else
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
                        .color = color,
                        .is_subpixel_text = true);
            min.x += (glyph->advance_width);
        }
	}
}

internal Vec2F32
render_measure_text(R_Font *font, Str8 text)
{
    Vec2F32 result = {0};
    for (U64 i = 0; i < text.size; ++i)
	{
        if (text.data[i] == ' ')
        {
            result.x += font->space_width;
        }
        else
        {
		R_Glyph *glyph = font->glyphs + text.data[i];
        result.x += (glyph->advance_width);
        }
	}
    result.y = font->line_height;
	return(result);
}

typedef struct R_FreeFontAtlasRegion R_FreeFontAtlasRegion;
struct R_FreeFontAtlasRegion
{
    struct R_FreeFontAtlasRegion *next;
    RectF32 region;
};

typedef struct R_FontAtlas R_FontAtlas;
struct R_FontAtlas
{
    Vec2U32 dim;
    R_Texture texture;
};

internal R_FontAtlas *
render_make_font_atlas(Arena *arena, Vec2U32 dim)
{
    return(0);
}

internal RectF32
render_alloc_font_atlas_region(Arena *arena, Vec2U32 dim)
{
    return((RectF32){0});
}

internal void
render_free_font_atlas_region(R_FontAtlas *atlas, RectF32 region)
{
}
