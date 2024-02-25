#if OS_WINDOWS
// NOTE(hampus): munmap() can't take 0 as a length in os_memory_release(). We can only do this on windows
#define STBTT_malloc(x, u) ((void)(u),os_memory_alloc(x))
#define STBTT_free(x,u)    ((void)(u),os_memory_release(x, 0))
#endif
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

    S32 horizontal_filter_padding = 1;

    int glyph_advance_width = 0;
    int glyph_left_side_bearing = 0;
    stbtt_GetGlyphHMetrics(&stb_font, stb_glyph_index, &glyph_advance_width, &glyph_left_side_bearing);

    RectS32 glyph_bounding_box = {0};
    stbtt_GetGlyphBox(&stb_font, stb_glyph_index, &glyph_bounding_box.x0, &glyph_bounding_box.y0, &glyph_bounding_box.x1, &glyph_bounding_box.y1);

    RectS32 glyph_box = {0};
    stbtt_GetGlyphBitmapBox(&stb_font, stb_glyph_index, scale, scale, &glyph_box.x0, &glyph_box.y0, &glyph_box.x1, &glyph_box.y1);
    Vec2S32 glyph_dim = rects32_dim(glyph_box);

    if (glyph_dim.x > 0 && glyph_dim.y > 0)
    {
        Vec2S32 padded_glyph_dim = glyph_dim;
        padded_glyph_dim.x += 2*horizontal_filter_padding;

        S32 horizontal_resolution = 3;

        S32 bitmap_stride = padded_glyph_dim.x * horizontal_resolution;
        S32 bitmap_size = bitmap_stride * padded_glyph_dim.y;
        S32 glyph_offset_x = (horizontal_filter_padding) * horizontal_resolution;
        U8 *glyph_bitmap = push_array_zero(scratch.arena, U8, bitmap_size+4); // NOTE(hampus): This 4 stops the sanitizer from complaining for some reason..
        stbtt_MakeGlyphBitmap(&stb_font,
                              glyph_bitmap + glyph_offset_x,
                              padded_glyph_dim.x* horizontal_resolution,
                              padded_glyph_dim.y,
                              bitmap_stride,
                              scale * horizontal_resolution,
                              scale,
                              stb_glyph_index);

        // TODO(hampus): Just render directly into the atlas bitmap
        U8 *subpx_bitmap = push_array_zero(scratch.arena, U8, bitmap_size);

        //- hampus: Subpixel rendering

        U8 *subpx_row = subpx_bitmap;
        U8 filter_weights[5] = { 0x08, 0x4D, 0x56, 0x4D, 0x08 };
        for (S32 y = 0; y < padded_glyph_dim.y; y++)
        {
            S32 x_end = padded_glyph_dim.x * horizontal_resolution - 1;
            U8 *subpx_pixel = subpx_row + 4;
            for (S32 x = 4; x < x_end; x++)
            {
                S32 sum = 0;
                S32 filter_weight_index = 0;
                    S32 kernel_x_end = (x == x_end - 1) ? x + 1 : x + 2;

                for (S32 kernel_x = x - 2; kernel_x <= kernel_x_end; kernel_x++)
                {
                    S32 offset = kernel_x + y*bitmap_stride;
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
            *atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32(padded_glyph_dim.x+2, padded_glyph_dim.y+2));
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
            .max = v2u32(rect_region.min.x + padded_glyph_dim.x,
                         rect_region.min.y + padded_glyph_dim.y),
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

    glyph->bearing_in_pixels = v2f32((F32)f32_round(glyph_left_side_bearing*scale),
                                     (F32)f32_floor(glyph_bounding_box.y1*scale));
    glyph->advance_width     = f32_round(glyph_advance_width * scale);

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
        F32 scale = stbtt_ScaleForMappingEmToPixels(&stb_font, (F32)params.size*1.33f);

        {
            int ascent, descent, line_gap;
            stbtt_GetFontVMetrics(&stb_font, &ascent, &descent, &line_gap);
            font->max_ascent = (F32)f32_floor(ascent * scale);
            font->max_descent = (F32)f32_floor(descent * scale);
            font->line_height = (F32)f32_floor(font->max_ascent - font->max_descent + line_gap);
        }

        U64 num_glyphs_to_load = 0;
        U32 glyph_count     = 0;

        U32 *glyph_indicies = 0;
        U32 *codepoints     = 0;

            num_glyphs_to_load = u64_min(stb_font.numGlyphs-1, 128);

            glyph_indicies = push_array(scratch.arena, U32, num_glyphs_to_load);
            codepoints     = push_array(scratch.arena, U32, num_glyphs_to_load);

            for (int codepoint = 0, index = (U32) stbtt_FindGlyphIndex(&stb_font, codepoint);
                 glyph_count < num_glyphs_to_load;
                 index = (U32) stbtt_FindGlyphIndex(&stb_font, codepoint))
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
                int kerning = stbtt_GetCodepointKernAdvance(&stb_font, glyph_indicies[i], glyph_indicies[j]);
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
                    int kerning = stbtt_GetCodepointKernAdvance(&stb_font, glyph_indicies[i], glyph_indicies[j]);
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
                        font->kern_pairs[index].value = (F32) f32_round(kerning * scale)*10;
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
