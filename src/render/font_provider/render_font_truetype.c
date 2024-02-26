#if OS_WINDOWS
// NOTE(hampus): munmap() can't take 0 as a length in os_memory_release(). We can only do this on windows
#    define STBTT_malloc(x, u) ((void)(u),os_memory_alloc(x))
#    define STBTT_free(x,u)    ((void)(u),os_memory_release(x, 0))
#endif
#define STBTT_ifloor(x)   ((int) f64_floor(x))
#define STBTT_iceil(x)    ((int) f64_ceil(x))
#define STBTT_sqrt(x)      f64_sqrt(x)
#define STBTT_pow(x,y)     f64_pow(x,y)
#define STBTT_fmod(x,y)    f64_mod(x,y)
#define STBTT_cos(x)       f64_cos(x)
#define STBTT_acos(x)      f64_acos(x)
#define STBTT_fabs(x)      f64_abs(x)
#define STBTT_assert(x)    assert(x)
#define STBTT_strlen(x)    length_cstr(x)
#define STBTT_memcpy       memory_copy
#define STBTT_memset       memory_set
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION

#if COMPILER_CLANG
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wconversion"
#elif COMPILER_GCC
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#    pragma GCC diagnostic ignored "-Wsign-conversion"
#    pragma GCC diagnostic ignored "-Wsign-compare"
#endif

#include "stb_truetype.h"

#if COMPILER_CLANG
#    pragma clang diagnostic pop
#elif COMPILER_GCC
#    pragma GCC diagnostic pop
#endif

#include "data/fonts/NotoSansMono_Medium.ttf.embed"
#include "data/fonts/fontello.ttf.embed"

internal Void
render_make_glyph(Render_Context *renderer, Render_Font *font, stbtt_fontinfo stb_font, U32 stb_glyph_index, U32 glyph_array_index, U32 codepoint, Render_FontRenderMode render_mode, F32 scale)
{
    Arena_Temporary scratch = get_scratch(0, 0);

    U64 map_mask = font->codepoint_map_size - 1;
    U64 index = ((U64) codepoint * 11400714819323198485LLU) & map_mask;
    while (font->codepoint_map[index].codepoint != U32_MAX)
    {
        index = (index + 1) & map_mask;
    }

    font->codepoint_map[index].codepoint   = codepoint;
    font->codepoint_map[index].glyph_index = glyph_array_index;

    Render_Glyph *glyph = font->glyphs + glyph_array_index;

    U32 horizontal_filter_padding = 1;

    int glyph_advance_width = 0;
    int glyph_left_side_bearing = 0;
    stbtt_GetGlyphHMetrics(&stb_font, (int) stb_glyph_index, &glyph_advance_width, &glyph_left_side_bearing);

    RectS32 glyph_bounding_box = {0};
    stbtt_GetGlyphBox(&stb_font, (int) stb_glyph_index, &glyph_bounding_box.x0, &glyph_bounding_box.y0, &glyph_bounding_box.x1, &glyph_bounding_box.y1);

    RectS32 glyph_box = {0};
    stbtt_GetGlyphBitmapBox(&stb_font, (int) stb_glyph_index, scale, scale, &glyph_box.x0, &glyph_box.y0, &glyph_box.x1, &glyph_box.y1);
    Vec2S32 glyph_dim = rects32_dim(glyph_box);

    if (glyph_dim.x > 0 && glyph_dim.y > 0)
    {
        Vec2S32 padded_glyph_dim = glyph_dim;
        padded_glyph_dim.x += 2 * horizontal_filter_padding;

        U32 horizontal_resolution = 3;

        U32 bitmap_stride = (U32) padded_glyph_dim.x * horizontal_resolution;
        U32 bitmap_size = bitmap_stride * (U32) padded_glyph_dim.y;
        U32 glyph_offset_x = horizontal_filter_padding * horizontal_resolution;
        U8 *glyph_bitmap = push_array_zero(scratch.arena, U8, bitmap_size + 4); // NOTE(hampus): This 4 stops the sanitizer from complaining for some reason..
        stbtt_MakeGlyphBitmap(&stb_font,
                              glyph_bitmap + glyph_offset_x,
                              padded_glyph_dim.x * (S32) horizontal_resolution,
                              padded_glyph_dim.y,
                              (int) bitmap_stride,
                              scale * (F32) horizontal_resolution,
                              scale,
                              (int) stb_glyph_index);

        // TODO(hampus): Just render directly into the atlas bitmap
        U8 *subpx_bitmap = push_array_zero(scratch.arena, U8, bitmap_size);

        //- hampus: Subpixel rendering

        U8 *subpx_row = subpx_bitmap;
        U8 filter_weights[5] = { 0x08, 0x4D, 0x56, 0x4D, 0x08 };
        for (U32 y = 0; y < (U32) padded_glyph_dim.y; y++)
        {
            U32 x_end = (U32) padded_glyph_dim.x * horizontal_resolution - 1;
            U8 *subpx_pixel = subpx_row + 4;
            for (U32 x = 4; x < x_end; x++)
            {
                S32 sum = 0;
                S32 filter_weight_index = 0;
                U32 kernel_x_end = (x == x_end - 1) ? x + 1 : x + 2;

                for (U32 kernel_x = x - 2; kernel_x <= kernel_x_end; kernel_x++)
                {
                    U32 offset = kernel_x + y * bitmap_stride;
                    sum += glyph_bitmap[offset] * filter_weights[filter_weight_index++];
                }

                sum = sum / 255;
                *subpx_pixel++ = (U8)((sum > 255) ? 255 : sum);
            }
            subpx_row += bitmap_stride;
        }

        //- hampus: Move subpixel RGB to RGBA in our font atlas

        Render_FontAtlasRegion *atlas_region = 0;
        os_mutex(&renderer->font_atlas_mutex)
        {
            atlas_region = &font->font_atlas_regions[font->num_font_atlas_regions++];
            *atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32((U32) padded_glyph_dim.x + 2, (U32) padded_glyph_dim.y + 2));
        }

        RectU32 rect_region = atlas_region->region;
        rect_region.min.x += 1;
        rect_region.min.y += 1;

        U8 *texture_data = (U8 *) renderer->font_atlas->memory + (rect_region.min.x + rect_region.min.y * renderer->font_atlas->dim.x)*4;
        U8 *dst = texture_data;
        U8 *src = subpx_bitmap;
        for (S32 y = 0; y < padded_glyph_dim.y; ++y)
        {
            U8 *dst_row = dst;
            U8 *src_row = src;
            for (S32 x = 0; x < padded_glyph_dim.x; ++x)
            {
                U8 r = *src_row++;
                U8 g = *src_row++;
                U8 b = *src_row++;
                *dst_row++ = r;
                *dst_row++ = g;
                *dst_row++ = b;
                *dst_row++ = 0xff;
            }

            dst += renderer->font_atlas->dim.x * 4;
            src += padded_glyph_dim.x * 3;
        }

        // NOTE(hampus): Adjust the rect region to only cover the bitmap
        // and not more
        RectU32 adjusted_rect_region =
        {
            .min = rect_region.min,
            .max = v2u32(rect_region.min.x + (U32) padded_glyph_dim.x,
                         rect_region.min.y + (U32) padded_glyph_dim.y),
        };

        RectF32 adjusted_rect_region_f32 = rectf32_from_rectu32(adjusted_rect_region);

        RectF32 uv =
        {
            .x0 = adjusted_rect_region_f32.x0 / (F32) renderer->font_atlas->dim.x,
            .x1 = adjusted_rect_region_f32.x1 / (F32) renderer->font_atlas->dim.x,
            .y0 = adjusted_rect_region_f32.y0 / (F32) renderer->font_atlas->dim.y,
            .y1 = adjusted_rect_region_f32.y1 / (F32) renderer->font_atlas->dim.y,
        };

        glyph->slice             = render_slice_from_texture(renderer->font_atlas->texture, uv);
        glyph->size_in_pixels    = v2f32((F32) padded_glyph_dim.x, (F32) padded_glyph_dim.y);
    }

    glyph->bearing_in_pixels = v2f32(f32_round((F32) glyph_left_side_bearing * scale),
                                     f32_floor((F32) glyph_bounding_box.y1 * scale));
    glyph->advance_width     = f32_round((F32) glyph_advance_width * scale);

    release_scratch(scratch);
}

internal B32
render_load_font_truetype(Render_Context *renderer, Render_Font *font, Render_FontLoadParams params)
{
    assert(font);
    Arena_Temporary scratch = get_scratch(0, 0);

    U64 last_slash = 0;
    str8_last_index_of(params.path, '/', &last_slash);

    Str8 file_read_result = { 0 };
    Str8 font_name = str8_skip(params.path, last_slash+1);
    if (str8_equal(font_name, str8_lit("NotoSansMono-Medium.ttf")))
    {
        file_read_result = framed_embed_unpack(scratch.arena, embed_NotoSansMono_Medium_data, embed_NotoSansMono_Medium_size);
    }
    else if (str8_equal(font_name, str8_lit("fontello.ttf")))
    {
        file_read_result = framed_embed_unpack(scratch.arena, embed_fontello_data, embed_fontello_size);
    }
    else
    {
        os_file_read(scratch.arena, params.path, &file_read_result);
    }

    if (file_read_result.size)
    {
        //- hampus: Init font

        font->load_params = params;

        stbtt_fontinfo stb_font = {0};
        stbtt_InitFont(&stb_font, file_read_result.data, 0);
        F32 scale = stbtt_ScaleForMappingEmToPixels(&stb_font, (F32) params.size * 1.33f);

        {
            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &line_gap);
            font->max_ascent  = f32_floor((F32) ascent  * scale);
            font->max_descent = f32_floor((F32) descent * scale);
            font->line_height = f32_floor(font->max_ascent - font->max_descent + (F32) line_gap);
        }

        U32 glyph_count        = 0;
        U64 num_glyphs_to_load = u64_min((U64) stb_font.numGlyphs - 1, 128);

        U32 *glyph_indicies = push_array(scratch.arena, U32, num_glyphs_to_load);
        U32 *codepoints     = push_array(scratch.arena, U32, num_glyphs_to_load);

        for (U32 codepoint = 0, index = (U32) stbtt_FindGlyphIndex(&stb_font, (int) codepoint);
             glyph_count < num_glyphs_to_load;
             index = (U32) stbtt_FindGlyphIndex(&stb_font, (int) codepoint))
        {
            if (index)
            {
                glyph_indicies[glyph_count] = index;
                codepoints[glyph_count]     = codepoint;
                ++glyph_count;
            }
            ++codepoint;
        }

        font->num_glyphs       = (U32) num_glyphs_to_load;
        font->font_atlas_regions = push_array(font->arena, Render_FontAtlasRegion, num_glyphs_to_load);
        font->glyphs             = push_array(font->arena, Render_Glyph, num_glyphs_to_load);
        font->codepoint_map_size = u32_ceil_to_power_of_2(((U32)num_glyphs_to_load * 4 + 2) / 3);
        font->codepoint_map      = push_array(font->arena, Render_CodepointMap, font->codepoint_map_size);
        for (U32 i = 0; i < font->codepoint_map_size; ++i)
        {
            font->codepoint_map[i].codepoint   = U32_MAX;
            font->codepoint_map[i].glyph_index = 0;
        }

        //- hampus: Load glyph

        for (U32 i = 0; i < glyph_count; ++i)
        {
            render_make_glyph(renderer, font, stb_font, glyph_indicies[i], i, codepoints[i], params.render_mode, scale);
        }

        // NOTE(simon): Count the number of kerning pairs.
        U64 kerning_pairs = 0;
        for (U32 i = 0; i < glyph_count; ++i)
        {
            for (U32 j = 0; j < glyph_count; ++j)
            {
                int kerning = stbtt_GetCodepointKernAdvance(&stb_font, (int) glyph_indicies[i], (int) glyph_indicies[j]);
                if (kerning)
                {
                    ++kerning_pairs;
                }
            }
        }

        if (kerning_pairs)
        {
            // NOTE(simon): Open addressed hash table with at least 75% fill.
            font->kern_map_size = u64_ceil_to_power_of_2((kerning_pairs * 4 + 3) / 3);
            font->kern_pairs    = push_array_zero(font->arena, Render_KerningPair, font->kern_map_size);

            for (U32 i = 0; i < glyph_count; ++i)
            {
                for (U32 j = 0; j < glyph_count; ++j)
                {
                    int kerning = stbtt_GetCodepointKernAdvance(&stb_font, (int) glyph_indicies[i], (int) glyph_indicies[j]);
                    if (kerning)
                    {
                        U64 pair = (U64) glyph_indicies[i] << 32 | (U64) glyph_indicies[j];
                        U64 mask = font->kern_map_size - 1;
                        // NOTE(simon): Pseudo fibonacci hashing.
                        U64 index = (pair * 11400714819323198485LLU) & mask;
                        while (font->kern_pairs[index].value != 0.0f)
                        {
                            index = (index + 1) & mask;
                        }

                        font->kern_pairs[index].pair = pair;
                        font->kern_pairs[index].value = f32_round((F32) kerning * scale) * 10;
                    }
                }
            }
        }
        else
        {
            // NOTE(simon): We need a map of size 1 in order to be able to do lookups into it.
            font->kern_map_size = 1;
            font->kern_pairs    = push_array_zero(font->arena, Render_KerningPair, font->kern_map_size);
        }
    }

    release_scratch(scratch);
    return(font != 0);
}
