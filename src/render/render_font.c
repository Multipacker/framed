// TODO(hampus):
	 // [x] - Subpixel rendering
// [x] - Atlasing
// [x] - Icons
// [x] - Unicode
// [x] - Caching
// [x] - New lines
// [ ] - Kerning
// [ ] - Specify min/max codepoints for loading glyphs
// [ ] - Subpixel positioning
// [ ] - Underline & strikethrough

// NOTE(hampus): Freetype have variables called 'internal' :(
#undef internal
#include "freetype/freetype.h"
#define internal static

#include "render/render_icon_codepoints.h"

internal Void
render_push_free_region_to_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node)
{
	node->next_free = atlas->first_free_region;
	node->prev_free = 0;
	// TODO(hampus): Use a dll_push_front
	if (atlas->first_free_region)
	{
		atlas->first_free_region->prev_free = node;
	}

	if (atlas->first_free_region == atlas->last_free_region)
	{
		atlas->last_free_region = node;
	}

	atlas->first_free_region = node;
	node->used = false;
	atlas->num_free_regions++;

    RectU32 rect_region = node->region;

    U32 width = rect_region.x1 - rect_region.x0;
    U32 height = rect_region.y1 - rect_region.y0;

    U8 *data = (U8 *)atlas->memory + (rect_region.min.x + rect_region.min.y * atlas->dim.x)*4;
    U8 *dst = data;
    for (U32 y = 0; y < height; ++y)
    {
        U32 *dst_row = (U32 *)dst;
        for (U32 x = 0; x < width; ++x)
        {
            *dst_row++ = 0;
        }

        dst += (S32) (atlas->dim.x * 4);
    }
}

internal Void
render_remove_free_region_from_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node)
{
	dll_remove_npz(atlas->first_free_region, atlas->last_free_region, node, next_free, prev_free, check_null, set_null);
    node->used = true;
    node->next_free = 0;
    node->prev_free = 0;
	atlas->num_free_regions--;
}

internal R_FontAtlas *
render_make_font_atlas(R_Context *renderer, Vec2U32 dim)
{
	R_FontAtlas *result = push_struct(renderer->permanent_arena, R_FontAtlas);
	result->dim = dim;
	R_FontAtlasRegionNode *first_free_region = push_struct(renderer->permanent_arena, R_FontAtlasRegionNode);
	first_free_region->region.min = v2u32(0, 0);
	first_free_region->region.max = v2u32(dim.x, dim.x);
	result->memory = push_array(renderer->permanent_arena, U8, dim.x * dim.y * 4);
	render_push_free_region_to_atlas(result, first_free_region);
	result->texture = render_create_texture_from_bitmap(renderer, result->memory, result->dim.x, result->dim.y, R_ColorSpace_Linear);
	return(result);
}

internal R_FontAtlasRegion
render_alloc_font_atlas_region(R_Context *renderer, R_FontAtlas *atlas, Vec2U32 dim)
{
    assert(atlas->num_free_regions > 0);
	// TODO(hampus): Benchmark and eventually optimize.
	R_FontAtlasRegionNode *first_free_region = atlas->first_free_region;
	// NOTE(hampus): Each region will always be the same size in
	// x and y, so we only need to check the width
	R_FontAtlasRegionNode *node = first_free_region;
	U32 required_size = u32_max(dim.x, dim.y);
	U32 region_size = node->region.max.x - node->region.min.x;
	B32 can_halve_size = region_size >= (required_size*2);
	B32 fits = region_size >= required_size;

    // NOTE(hampus): Find the first that fits
	while (!fits)
	{
		if (!node->next_free)
		{
			// NOTE(hampus): There wasn't any free region left
			// TODO(hampus): Logging
			assert(false);
			break;
		}
		node = node->next_free;
		region_size = node->region.max.x - node->region.min.x;
		fits = region_size > required_size;
	}

    // NOTE(hampus): Find the best that fit
    B32 find_best_fit = true;
    if (find_best_fit)
    {
        R_FontAtlasRegionNode *next = node->next_free;
        while (next)
        {
                U32 next_region_size = next->region.max.x - next->region.min.x;
                fits = next_region_size >= required_size;
                if (fits)
                {
                    if (next_region_size < region_size)
                    {
                        node = next;
                        region_size = node->region.max.x - node->region.min.x;
                    }
            }

            next = next->next_free;
        }
    }

                        can_halve_size = region_size >= (required_size*2);

	while (can_halve_size)
	{
		// NOTE(hampus): Remove the current node, it will no longer
		// be free to take because one of its descendants will
		// be taken.
		render_remove_free_region_from_atlas(atlas, node);

		// NOTE(hampus): Allocate 4 children to replace
		// the parent

        if (!node->children[0])
        {
            R_FontAtlasRegionNode *children = push_array(renderer->permanent_arena, R_FontAtlasRegionNode, Corner_COUNT);

            {
                Vec2U32 bbox[Corner_COUNT] =
                {
                    bbox[Corner_TopLeft] = node->region.min,
                    bbox[Corner_TopRight] = v2u32(node->region.max.x, node->region.min.y),
                    bbox[Corner_BottomLeft] = v2u32(node->region.min.x, node->region.max.y),
                    bbox[Corner_BottomRight] = node->region.max,
                };

                Vec2U32 middle = v2u32_div_u32(v2u32_add_v2u32(bbox[Corner_BottomRight], bbox[Corner_TopLeft]), 2);

                children[Corner_TopLeft].region     = rectu32(bbox[Corner_TopLeft], middle);

                children[Corner_TopRight].region.min = v2u32(middle.x, bbox[Corner_TopRight].y);
                children[Corner_TopRight].region.max = v2u32(bbox[Corner_TopRight].x, middle.y);

                children[Corner_BottomLeft].region.min = v2u32(bbox[Corner_TopLeft].x, middle.y);
                children[Corner_BottomLeft].region.max = v2u32(middle.x, bbox[Corner_BottomRight].y);

                children[Corner_BottomRight].region = rectu32(middle, bbox[Corner_BottomRight]);
            }

            // NOTE(hampus): Push back the new children to
            // the free list and link them into the quad-tree.
            for (U64 i = 0; i < Corner_COUNT; ++i)
            {
                children[i].parent = node;
                node->children[i] = &children[i];
            }
        }

        for (U64 i = 0; i < Corner_COUNT; ++i)
        {
            render_push_free_region_to_atlas(atlas, node->children[3 - i]);
        }

            node = node->children[0];

		region_size = node->region.max.x - node->region.min.x;
		can_halve_size = region_size >= (required_size*2);
	}

	render_remove_free_region_from_atlas(atlas, node);

	node->next_free = 0;
	node->prev_free = 0;
	R_FontAtlasRegion result = { node, node->region };
	return(result);
}

internal Void
render_free_atlas_region(R_FontAtlas *atlas, R_FontAtlasRegion region)
{
	R_FontAtlasRegionNode *node = region.node;
	assert(node->used);
    R_FontAtlasRegionNode *parent = node->parent;
	render_push_free_region_to_atlas(atlas, node);
	if (parent)
	{
		// NOTE(hampus): Lets check if we can combine
		// any empty nodes to larger nodes. Otherwise
		// we just put it back into the free list
		B32 used = false;
		for (U64 i = 0; i < Corner_COUNT; ++i)
		{
			R_FontAtlasRegionNode *child = parent->children[i];
			if (child->used)
			{
				used = true;
				break;
			}
		}

		if (!used)
		{
			// NOTE(hampus): All the siblings are marked as empty.
			// Remove the children from the free list and push
			// their parent.
			for (U64 i = 0; i < Corner_COUNT; ++i)
			{
				render_remove_free_region_from_atlas(atlas, parent->children[i]);
			}
			render_free_atlas_region(atlas, (R_FontAtlasRegion) { parent, parent->region });
		}
	}
}

internal Void
render_destroy_font(R_Context *renderer, R_Font *font)
{
    for (U64 i = 0; i < font->num_glyphs; ++i)
    {
        R_Glyph *glyph = font->glyphs + i;
        if (glyph->font_atlas_region.node)
        {
            if (glyph->font_atlas_region.node->used)
            {
            render_free_atlas_region(renderer->font_atlas, glyph->font_atlas_region);
            }
        }
    }
    arena_destroy(font->arena);
	render_update_texture(renderer, renderer->font_atlas->texture, renderer->font_atlas->memory, renderer->font_atlas->dim.width, renderer->font_atlas->dim.height, 0);
}

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
render_make_glyph(R_Context *renderer, R_Font *font, FT_Face face, U32 index, U32 codepoint, R_FontRenderMode render_mode)
{
	U32 bitmap_height = 0;
	S32 bearing_left  = 0;
	S32 bearing_top   = 0;
	U32 bitmap_width  = 0;
	U8 *texture_data = 0;
	RectU32 rect_region = { 0 };

    R_GlyphBucket *bucket = font->glyph_bucket + (codepoint % GLYPH_BUCKETS_ARRAY_SIZE);
    R_GlyphIndexNode *node = push_struct(font->arena, R_GlyphIndexNode);

node->codepoint = codepoint;
    node->index = index;
    sll_push_front(bucket->first, node);

	B32 result = true;
	Str8 error;
	R_Glyph *glyph = font->glyphs + node->index;

      // NOTE(hampus): Some codepoints may map to the same index
    if (!glyph->font_atlas_region.node)
    {
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

        B32 empty = false;

        FT_Error ft_load_glyph_error = FT_Load_Glyph(face, index, ft_load_flags);
        if (!ft_load_glyph_error)
        {
            FT_Error ft_render_glyph_error = FT_Render_Glyph(face->glyph, ft_render_flags);
            if (!ft_render_glyph_error)
            {
                switch (render_mode)
                {
                    case R_FontRenderMode_Normal:
                    {
                        empty = face->glyph->bitmap.buffer == 0;
                        if (!empty)
                        {
                        bitmap_height = face->glyph->bitmap.rows;
                        bearing_left  = face->glyph->bitmap_left;
                        bearing_top   = face->glyph->bitmap_top;
                        bitmap_width  = face->glyph->bitmap.width;

                        R_FontAtlasRegion font_atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas,
                                                                                             v2u32(bitmap_width, bitmap_height));
                        glyph->font_atlas_region = font_atlas_region;

                        rect_region = font_atlas_region.region;

                        // TODO(hampus): SIMD (or check that the compiler actually SIMD's this)
                        // NOTE(hampus): Convert from 8 bit to 32 bit
                        texture_data = (U8 *) renderer->font_atlas->memory + (rect_region.min.x + rect_region.min.y * renderer->font_atlas->dim.x)*4;
                        U32 *dst = (U32 *)texture_data;
                        U8 *src = face->glyph->bitmap.buffer;
                        for (U32 y = 0; y < bitmap_height; ++y)
                        {
                            U32 *dst_row = dst;
                            for (U32 x = 0; x < bitmap_width; ++x)
                            {
                                U8 val = *src++;
                                *dst_row++ = (U32) ((0xff <<  0) |
                                                   (0xff <<  8) |
                                                   (0xff << 16) |
                                                   (val  << 24));
                            }

                            dst += renderer->font_atlas->dim.x;
                            }
                        }
                        else
                        {
                            // NOTE(hampus): This was an empty glyph.
                            glyph->font_atlas_region = font->empty_font_atlas_region;
                        }

                    } break;

                    case R_FontRenderMode_LCD:
                    {
                        empty = face->glyph->bitmap.buffer == 0;
                        if (!empty)
                        {
                            bitmap_height = face->glyph->bitmap.rows;
                            bitmap_width = face->glyph->bitmap.width / 3;
                            bearing_left  = face->glyph->bitmap_left;
                            bearing_top   = face->glyph->bitmap_top;
                            // NOTE(hampus): We now have 3 "pixels" for each value.

                            R_FontAtlasRegion font_atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32(bitmap_width, bitmap_height));

                            glyph->font_atlas_region = font_atlas_region;
                            rect_region = font_atlas_region.region;

                            // TODO(hampus): SIMD (or check that the compiler actually SIMD's this)
                            // NOTE(hampus): Convert from 24 bit RGB to 32 bit RGBA
                            // We do not use the alpha here.
                            texture_data = (U8 *) renderer->font_atlas->memory + (rect_region.min.x + rect_region.min.y * renderer->font_atlas->dim.x)*4;
                            U8 *dst = texture_data;
                            U8 *src = face->glyph->bitmap.buffer;
                            for (U32 y = 0; y < bitmap_height; ++y)
                            {
                                U8 *dst_row = dst;
                                U8 *src_row = src;
                                for (U32 x = 0; x < bitmap_width; ++x)
                                {
                                    U8 r = *src_row++;
                                    U8 g = *src_row++;
                                    U8 b = *src_row++;
                                    *dst_row++ = r;
                                    *dst_row++ = g;
                                    *dst_row++ = b;
                                    *dst_row++ = 0xff;
                                }

                                dst += (S32) (renderer->font_atlas->dim.x * 4);

                                // NOTE(hampus): Freetype actually adds padding
                                // so the pitch is the correct width to increment
                                // by.
                                src += (S32) (face->glyph->bitmap.pitch);
                            }

                        }
                        else
                        {
                            // NOTE(hampus): This was an empty glyph.
                            glyph->font_atlas_region = font->empty_font_atlas_region;
                        }
                    } break;
                    invalid_case;
                }
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

        rect_region.max.x = rect_region.min.x + bitmap_width;
        rect_region.max.y = rect_region.min.y + bitmap_height;

        RectF32 uv = rectf32_from_rectu32(rect_region);

        uv.min.x /= (F32) renderer->font_atlas->dim.x;
        uv.max.x /= (F32) renderer->font_atlas->dim.x;

        uv.min.y /= (F32) renderer->font_atlas->dim.y;
        uv.max.y /= (F32) renderer->font_atlas->dim.y;

        glyph->slice             = render_slice_from_texture(renderer->font_atlas->texture, uv);
        glyph->size_in_pixels    = v2f32((F32) bitmap_width, (F32) bitmap_height);
        glyph->bearing_in_pixels = v2f32((F32) bearing_left, (F32) bearing_top);
        glyph->advance_width     = (F32) (face->glyph->advance.x >> 6);
    }

	return(result);
}

internal S32
render_pixels_from_font_unit(S32 funit, FT_Fixed scale)
{
    S32 result = 0;

    result = (S32) ((F32) funit * ((F32) scale / 65536.0f)) >> 6;

    return(result);
}

internal R_Font *
render_make_font_freetype(R_Context *renderer, S32 font_size, Str8 path, R_FontRenderMode render_mode)
{
    R_Font *result = 0;
	Arena_Temporary scratch = get_scratch(0, 0);

	FT_Library ft;
	// NOTE(hampus): 0 indicates a success in freetype, otherwise error
	Str8 error;
	FT_Error ft_init_error = FT_Init_FreeType(&ft);
	if (!ft_init_error)
	{
		S32 major_version, minor_version, patch;
		FT_Library_Version(ft, &major_version, &minor_version, &patch);

		Str8 file_read_result = { 0 };
		if (os_file_read(scratch.arena, path, &file_read_result))
		{
			FT_Open_Args open_args = { 0 };
			open_args.flags = FT_OPEN_MEMORY;
			open_args.memory_base = file_read_result.data;
			assert(file_read_result.size <= U32_MAX);
			open_args.memory_size = (U32) file_read_result.size;
			FT_Face face;
			FT_Error ft_open_face_error = FT_Open_Face(ft, &open_args, 0, &face);
			if (!ft_open_face_error)
			{
                Arena *arena = arena_create();
                result = push_struct(arena, R_Font);
                result->arena = arena;
                result->glyphs = push_array(result->arena, R_Glyph, (U64) face->num_glyphs);
                FT_Select_Charmap(face, ft_encoding_unicode);
				result->num_glyphs = (U32) face->num_glyphs;
                result->path = path;
                result->render_mode = render_mode;
                // TODO(hampus): Get the monitor's DPI
				F32 dpi = 72;
				FT_Error ft_set_pixel_sizes_error = FT_Set_Char_Size(face, (U32) font_size << 6, (U32) font_size << 6, (U32) dpi, (U32) dpi);
				if (!ft_set_pixel_sizes_error)
				{
                    result->line_height         = (F32)render_pixels_from_font_unit(face->height, face->size->metrics.y_scale);
					result->underline_position  = (F32)render_pixels_from_font_unit(face->underline_position, face->size->metrics.y_scale);
					result->max_advance_width   = (F32)render_pixels_from_font_unit(face->max_advance_width, face->size->metrics.x_scale);
					result->max_ascent          = (F32)render_pixels_from_font_unit(face->ascender, face->size->metrics.y_scale);
					result->max_descent         = (F32)render_pixels_from_font_unit(face->descender, face->size->metrics.y_scale);
					result->has_kerning         = FT_HAS_KERNING(face);
					result->family_name         = str8_copy_cstr(result->arena, (U8 *) face->family_name);
					result->style_name          = str8_copy_cstr(result->arena, (U8 *) face->style_name);
					result->font_size           = (U32) font_size;

                    // NOTE(hampus): Make an empty glyph that the empty glyphs will use
                    result->empty_font_atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32(1, 1));

                    // NOTE(hampus): Get the rectangle glyph which represents
                    // an missing charcter
                    render_make_glyph(renderer, result, face, 0, 0, render_mode);
                    result->num_loaded_glyphs++;

                    U32 index;
                    U32 charcode = (U32) FT_Get_First_Char(face, &index);
                    while (index != 0)
                    {
                        if (render_make_glyph(renderer, result, face, index, charcode, render_mode))
						{
                            result->num_loaded_glyphs++;
                            if (result->num_loaded_glyphs == 128)
                            {
                                break;
                            }
							// TODO(hampus): We don't have the declaration for
							// this function for some reason
							// FT_Done_Glyph(face->glyph);
						}
						else
						{
							// TODO(hampus): Logging
						}

                        charcode = (U32) FT_Get_Next_Char(face, charcode, &index);
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

	render_update_texture(renderer, renderer->font_atlas->texture, renderer->font_atlas->memory, renderer->font_atlas->dim.width, renderer->font_atlas->dim.height, 0);

	release_scratch(scratch);
    return(result);
}

internal R_Font *
render_make_font(R_Context *renderer, S32 font_size, Str8 path, R_FontRenderMode render_mode)
{
	// TODO(hampus): Check the pixel orientation of the monitor.
	R_Font *font = render_make_font_freetype(renderer, font_size, path, render_mode);
    return(font);
}

internal U32
render_glyph_index_from_codepoint(R_Font *font, U32 codepoint)
{
    U32 index = 0;
    R_GlyphBucket *bucket = font->glyph_bucket + (codepoint % GLYPH_BUCKETS_ARRAY_SIZE);

    for (R_GlyphIndexNode *node = bucket->first;
         node != 0;
         node = node->next)
    {
        if (node->codepoint == codepoint)
        {
            index = node->index;
            break;
        }
    }
    return(index);
}

internal R_Font *
render_font_from_key(R_Context *renderer, R_FontKey font_key)
{
    R_Font *result = 0;
     S32 empty_slot = -1;
     S32 unused_slot = -1;
    for (S32 i = 0; i < R_FONT_CACHE_SIZE; ++i)
    {
        R_Font *font = renderer->font_cache->fonts[i];
        if (font)
        {
            if (str8_equal(font->path, font_key.path) &&
                font->font_size == font_key.font_size)
            {
                result = font;
                break;
            }

            if (font->last_frame_index_used < renderer->frame_index)
            {
                unused_slot = i;
            }
        }
        else
        {
            if (empty_slot == -1)
            {
                empty_slot = i;
            }
        }
    }

    if (!result)
    {
        // NOTE(hampus): Cache miss
        S32 slot_index_to_fill = 0;
        if (empty_slot == -1)
        {
            // NOTE(hampus): The cache is full. Try to
            // find a font we can evict
            assert(unused_slot != -1 && "Cache is hot and full");
            slot_index_to_fill = unused_slot;
            render_destroy_font(renderer, renderer->font_cache->fonts[slot_index_to_fill]);
        }
        else
        {
            slot_index_to_fill = empty_slot;
        renderer->font_cache->slots_used++;
        }

        renderer->font_cache->fonts[slot_index_to_fill] = render_make_font(renderer, (S32) font_key.font_size, font_key.path, R_FontRenderMode_LCD);
        result = renderer->font_cache->fonts[slot_index_to_fill];
    }

    #if 0
    R_FontHash hash = {hash_str8(str8_cstr((CStr)&font_key))};
    U64 slot_index = hash.hash % R_FONT_CACHE_SIZE;
    R_Font *font = renderer->font_cache->fonts[slot_index];

     while (font)
    {
        if (str8_equal(font->path, font_key.path) &&
            font->font_size == font_key.font_size)
        {
            break;
        }

        slot_index += 1;
        slot_index %= R_FONT_CACHE_SIZE;
        font = renderer->font_cache->fonts[slot_index];
    }

    if (!font)
    {
         // NOTE(hampus): Cache miss. Load in the font
        if (renderer->font_cache->slots_used == (R_FONT_CACHE_SIZE * 3) / 4)
        {
            B32 evicted = false;
            U64 i;
            for (i = 0; i < R_FONT_CACHE_SIZE; ++i)
            {
                if (renderer->font_cache->fonts[i])
                {
                    if (renderer->font_cache->fonts[i]->last_frame_index_used < renderer->frame_index)
                    {
                        render_destroy_font(renderer, renderer->font_cache->fonts[i]);
                        renderer->font_cache->slots_used--;
                        renderer->font_cache->fonts[i] = 0;
                        evicted = true;
                        break;
                    }
                }
            }
            assert(evicted);
            renderer->font_cache->fonts[i] = render_make_font(renderer, font_key.font_size, font_key.path);
            font = renderer->font_cache->fonts[i];
        }
        else
        {
            for (U64 i = slot_index; i < (slot_index+R_FONT_CACHE_SIZE); ++i)
            {
                U32 index = i % R_FONT_CACHE_SIZE;
                if (!renderer->font_cache->fonts[index])
                {
                    renderer->font_cache->fonts[index] = render_make_font(renderer, font_key.font_size, font_key.path);
                    font = renderer->font_cache->fonts[index];
                    break;
                }
            }
        }
        renderer->font_cache->slots_used++;
    }
    #endif
    result->last_frame_index_used = renderer->frame_index;

    return(result);
}

internal R_FontKey
render_key_from_font(Str8 path, U32 font_size)
{
    R_FontKey result = {0};
    result.path = path;
    result.font_size = font_size;
    return(result);
}

internal Void
render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = render_font_from_key(renderer, font_key);
	for (U64 i = 0; i < text.size; ++i)
	{
            U32 codepoint = text.data[i];

            U32 index = render_glyph_index_from_codepoint(font, codepoint);

            assert(index != 0);
			R_Glyph *glyph = font->glyphs + index;

			F32 xpos = min.x + glyph->bearing_in_pixels.x;
			F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

			F32 width = (F32) glyph->size_in_pixels.x;
			F32 height = (F32) glyph->size_in_pixels.y;

			render_rect(renderer,
									v2f32(xpos, ypos),
									v2f32(xpos + width,
												ypos + height),
									.slice = glyph->slice,
						.color = color,
						.is_subpixel_text = R_USE_SUBPIXEL_RENDERING);
			min.x += (glyph->advance_width);
		}
}

internal Void
render_multiline_text(R_Context *renderer, Vec2F32 min, Str8 text, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = render_font_from_key(renderer, font_key);
    Vec2F32 origin = min;
	for (U64 i = 0; i < text.size; ++i)
	{
        U32 codepoint = text.data[i];

        if (codepoint == '\n')
        {
            min.x = origin.x;
            min.y += font->line_height;
        }
        else
        {
            U32 index = render_glyph_index_from_codepoint(font, codepoint);

            assert(index != 0);
            R_Glyph *glyph = font->glyphs + index;

            F32 xpos = min.x + glyph->bearing_in_pixels.x;
            F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

            F32 width = (F32) glyph->size_in_pixels.x;
            F32 height = (F32) glyph->size_in_pixels.y;

            render_rect(renderer,
                        v2f32(xpos, ypos),
                        v2f32(xpos + width,
                              ypos + height),
                        .slice = glyph->slice,
                        .color = color,
                        .is_subpixel_text = R_USE_SUBPIXEL_RENDERING);
            min.x += (glyph->advance_width);
        }
    }
}

internal Void
render_character(R_Context *renderer, Vec2F32 min, U32 codepoint, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = render_font_from_key(renderer, font_key);
        U32 index = render_glyph_index_from_codepoint(font, codepoint);

        R_Glyph *glyph = font->glyphs + index;

        F32 xpos = min.x + glyph->bearing_in_pixels.x;
        F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

        F32 width = (F32) glyph->size_in_pixels.x;
        F32 height = (F32) glyph->size_in_pixels.y;

        render_rect(renderer,
                    v2f32(xpos, ypos),
                    v2f32(xpos + width,
                          ypos + height),
                    .slice = glyph->slice,
                    .color = color,
                    .is_subpixel_text = R_USE_SUBPIXEL_RENDERING);
}

internal Vec2F32
render_measure_text(R_Font *font, Str8 text)
{
    S32 length = (S32)text.size;
	Vec2F32 result = { 0 };
	for (S32 i = 0; i < length; ++i)
	{
        U32 index = render_glyph_index_from_codepoint(font, text.data[i]);
			R_Glyph *glyph = font->glyphs + index;
        result.x += (glyph->advance_width);
	}

	result.y = font->line_height;
	return(result);
}

internal Vec2F32
render_measure_multiline_text(R_Font *font, Str8 text)
{
    S32 length = (S32)text.size;
	Vec2F32 result = { 0 };
    F32 max_row_width = 0;
    F32 row_width = 0;
	for (S32 i = 0; i < length; ++i)
	{
        if (text.data[i] == '\n')
        {
            max_row_width = f32_max(row_width, max_row_width);
            row_width = 0;
            result.y += font->line_height;
        }
        else
        {
            U32 index = render_glyph_index_from_codepoint(font, text.data[i]);
            R_Glyph *glyph = font->glyphs + index;
            row_width += (glyph->advance_width);
        }
	}

	result.y += font->line_height;
    result.x = max_row_width;
	return(result);
}
