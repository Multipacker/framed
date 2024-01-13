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
render_make_glyph(Render_Context *renderer, Render_Font *font, FT_Face face, U32 index, U32 codepoint, Render_FontRenderMode render_mode)
{
	assert(renderer);
	assert(font);
	
	U32 bitmap_height = 0;
	S32 bearing_left  = 0;
	S32 bearing_top   = 0;
	U32 bitmap_width  = 0;
	U8 *texture_data = 0;
	RectU32 rect_region = { 0 };
	
	B32 index_already_allocated = false;
	
	Render_GlyphBucket *bucket = font->glyph_bucket + (codepoint % GLYPH_BUCKETS_ARRAY_SIZE);
	for (Render_GlyphIndexNode *node = bucket->first;
		 node != 0;
		 node = node->next)
	{
		if (node->index == index)
		{
			index_already_allocated = true;
			break;
		}
	}
	
	Render_GlyphIndexNode *node = push_struct(font->arena, Render_GlyphIndexNode);
	node->codepoint = codepoint;
	node->index = index;
	sll_push_front(bucket->first, node);
	
	Str8 error = { 0 };
	Render_Glyph *glyph = font->glyphs + node->index;
	// NOTE(hampus): Some codepoints may map to the same index
	if (!index_already_allocated)
	{
		FT_Int32 ft_load_flags = 0;
		FT_Render_Mode ft_render_flags = 0;
		Render_FontAtlasRegion *atlas_region;
		switch (render_mode)
		{
			case Render_FontRenderMode_Normal:
			{
				ft_load_flags = FT_LOAD_DEFAULT;
				ft_render_flags = FT_RENDER_MODE_NORMAL;
			} break;
			
			case Render_FontRenderMode_LCD:
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
					case Render_FontRenderMode_Normal:
					{
						empty = face->glyph->bitmap.buffer == 0;
						if (!empty)
						{
							bitmap_height = face->glyph->bitmap.rows;
							bearing_left  = face->glyph->bitmap_left;
							bearing_top   = face->glyph->bitmap_top;
							bitmap_width  = face->glyph->bitmap.width;
							
							os_mutex(&renderer->font_atlas_mutex)
							{
								atlas_region = &font->font_atlas_regions[font->num_font_atlas_regions++];
								*atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas,
																			   v2u32(bitmap_width+2, bitmap_height+2));
							}
							
							rect_region = atlas_region->region;
							
							// NOTE(hampus): Add some extra padding
							rect_region.min.x += 1;
							rect_region.min.y += 1;
							
							// TODO(hampus): SIMD (or check that the compiler actually SIMD's this)
							// NOTE(hampus): Convert from 8 bit to 32 bit
							texture_data = (U8 *) renderer->font_atlas->memory + (rect_region.min.x + rect_region.min.y * renderer->font_atlas->dim.x)*4;
							U32 *dst = (U32 *) texture_data;
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
							rect_region = font->font_atlas_regions[0].region;
						}
						
					} break;
					
					case Render_FontRenderMode_LCD:
					{
						empty = face->glyph->bitmap.buffer == 0;
						if (!empty)
						{
							bitmap_height = face->glyph->bitmap.rows;
							bitmap_width = face->glyph->bitmap.width / 3;
							bearing_left  = face->glyph->bitmap_left;
							bearing_top   = face->glyph->bitmap_top;
							// NOTE(hampus): We now have 3 "pixels" for each value.
							
							os_mutex(&renderer->font_atlas_mutex)
							{
								atlas_region = &font->font_atlas_regions[font->num_font_atlas_regions++];
								*atlas_region = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32(bitmap_width+2, bitmap_height+2));
							}
							
							rect_region = atlas_region->region;
							
							// NOTE(hampus): Add some extra padding
							rect_region.min.x += 1;
							rect_region.min.y += 1;
							
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
							rect_region = font->font_atlas_regions[0].region;
						}
					} break;
					invalid_case;
				}
			}
			else
			{
				error = render_get_ft_error_message(ft_render_glyph_error);
				log_warning("Freetype: %"PRISTR8, str8_expand(error));
				// TODO(hampus): What to do here?
			}
		}
		else
		{
			error = render_get_ft_error_message(ft_load_glyph_error);
			log_warning("Freetype: %"PRISTR8, str8_expand(error));
			// TODO(hampus): What to do here?
		}
		
		// NOTE(hampus): Adjust the rect region to only cover the bitmap
		// and not more
		RectU32 adjusted_rect_region =
		{
			.min = rect_region.min,
			.max = v2u32(rect_region.min.x + bitmap_width,
						 rect_region.min.y + bitmap_height),
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
		glyph->size_in_pixels    = v2f32((F32) bitmap_width, (F32) bitmap_height);
		glyph->bearing_in_pixels = v2f32((F32) bearing_left, (F32) bearing_top);
		glyph->advance_width     = (F32) (face->glyph->advance.x >> 6);
	}
	
	B32 result = error.size == 0;
	
	return(result);
}

internal S32
render_pixels_from_font_unit(S32 funit, FT_Fixed scale)
{
	S32 result = 0;
	result = (S32) ((F32) funit * ((F32) scale / 65536.0f)) >> 6;
	return(result);
}

internal B32
render_load_font(Render_Context *renderer, Render_Font *font, Render_FontLoadParams params)
{
	assert(font);
	Arena_Temporary scratch = get_scratch(0, 0);

	FT_Library ft;
	// NOTE(hampus): 0 indicates a success in freetype, otherwise error
	Str8 error = { 0 };
	FT_Error ft_init_error = FT_Init_FreeType(&ft);
	if (!ft_init_error)
	{
		S32 major_version, minor_version, patch;
		FT_Library_Version(ft, &major_version, &minor_version, &patch);
		
		Str8 file_read_result = { 0 };
		if (os_file_read(scratch.arena, params.path, &file_read_result))
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
				font->load_params = params;
				
				FT_Select_Charmap(face, ft_encoding_unicode);
				U64 num_glyphs_to_load = 128;
				font->font_atlas_regions = push_array(font->arena, Render_FontAtlasRegion, num_glyphs_to_load+1);
				font->glyphs             = push_array(font->arena, Render_Glyph, (U64) face->num_glyphs);
				font->num_glyphs         = (U32) face->num_glyphs;

				U32 *glyph_indicies = push_array(scratch.arena, U32, num_glyphs_to_load);
				U32 *codepoints     = push_array(scratch.arena, U32, num_glyphs_to_load);
				U32 glyph_count = 0;

				for (
					U32 index = 0, codepoint = (U32) FT_Get_First_Char(face, &index);
					index != 0 && glyph_count < num_glyphs_to_load;
					codepoint = (U32) FT_Get_Next_Char(face, codepoint, &index)
				)
				{
					glyph_indicies[glyph_count] = index;
					codepoints[glyph_count]     = codepoint;
					++glyph_count;
				}
				
				Vec2F32 dpi = gfx_get_dpi(renderer->gfx);
				FT_Error ft_set_pixel_sizes_error = FT_Set_Char_Size(face, (U32) params.size << 6, (U32) params.size << 6, (U32) dpi.x, (U32) dpi.y);
				if (!ft_set_pixel_sizes_error)
				{
					font->line_height         = (F32) render_pixels_from_font_unit(face->height, face->size->metrics.y_scale);
					font->underline_position  = (F32) render_pixels_from_font_unit(face->underline_position, face->size->metrics.y_scale);
					font->max_advance_width   = (F32) render_pixels_from_font_unit(face->max_advance_width, face->size->metrics.x_scale);
					font->max_ascent          = (F32) render_pixels_from_font_unit(face->ascender, face->size->metrics.y_scale);
					font->max_descent         = (F32) render_pixels_from_font_unit(face->descender, face->size->metrics.y_scale);
					font->has_kerning         = FT_HAS_KERNING(face);
					font->family_name         = str8_copy_cstr(font->arena, (U8 *) face->family_name);
					font->style_name          = str8_copy_cstr(font->arena, (U8 *) face->style_name);
					
					os_mutex(&renderer->font_atlas_mutex)
					{
						// NOTE(hampus): Make an empty glyph that the empty glyphs will use
						font->font_atlas_regions[font->num_font_atlas_regions++] = render_alloc_font_atlas_region(renderer, renderer->font_atlas, v2u32(1, 1));
					}
					
					// NOTE(hampus): Get the rectangle glyph which represents
					// an missing charcter
					render_make_glyph(renderer, font, face, 0, 0, params.render_mode);
					font->num_loaded_glyphs++;
					
					for (U32 i = 0; i < glyph_count; ++i)
					{
						if (render_make_glyph(renderer, font, face, glyph_indicies[i], codepoints[i], params.render_mode))
						{
							font->num_loaded_glyphs++;
							if (font->num_loaded_glyphs == num_glyphs_to_load)
							{
								break;
							}
							// TODO(hampus): We don't have the declaration for
							// this function for some reason
							// FT_Done_Glyph(face->glyph);
						}
					}
				}
				else
				{
					error = render_get_ft_error_message(ft_set_pixel_sizes_error);
					log_error("Freetype: %"PRISTR8, str8_expand(error));
					// TODO(hampus): What to do here?
				}
				
				if (font->has_kerning)
				{
					// NOTE(simon): Count the number of kerning pairs.
					U64 kerning_pairs = 0;
					for (U32 i = 0; i < glyph_count; ++i)
					{
						for (U32 j = 0; j < glyph_count; ++j)
						{
							FT_Vector kerning;
							FT_Get_Kerning(face, glyph_indicies[i], glyph_indicies[j], FT_KERNING_DEFAULT, &kerning);
							if (kerning.x)
							{
								++kerning_pairs;
							}
						}
					}

					// NOTE(simon): Open addressed hash table with at least 75% fill.
					font->kern_map_size = u64_ceil_to_power_of_2((kerning_pairs * 4 + 3) / 3);
					font->kern_pairs    = push_array_zero(font->arena, Render_KerningPair, font->kern_map_size);

					for (U32 i = 0; i < glyph_count; ++i)
					{
						for (U32 j = 0; j < glyph_count; ++j)
						{
							FT_Vector kerning;
							FT_Get_Kerning(face, glyph_indicies[i], glyph_indicies[j], FT_KERNING_DEFAULT, &kerning);
							if (kerning.x)
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
								font->kern_pairs[index].value = (F32) kerning.x / 65536.0f;
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
				
				FT_Done_Face(face);
			}
			else
			{
				error = render_get_ft_error_message(ft_open_face_error);
				log_error("Freetype: %"PRISTR8, str8_expand(error));
				// TODO(hampus): What to do here?
			}
		}
		else
		{
			log_error("Failed to load font file `%"PRISTR8"`", str8_expand(params.path));
			// TODO(hampus): What to do here?
		}
		
		FT_Done_FreeType(ft);
	}
	else
	{
		error = render_get_ft_error_message(ft_init_error);
		// TODO(hampus): What to do here?
		log_error("Freetype: %"PRISTR8, str8_expand(error));
	}
	
	release_scratch(scratch);
	
	return(font != 0);
}
