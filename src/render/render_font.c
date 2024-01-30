// TODO(hampus):
// [x] - Subpixel rendering
// [x] - Atlasing
// [x] - Icons
// [x] - Unicode
// [x] - Caching
// [x] - New lines
// [x] - Kerning
// [ ] - Specify min/max codepoints for loading glyphs
// [ ] - Subpixel positioning
// [ ] - Underline & strikethrough

internal B32
render_font_valid_load_params(Render_FontLoadParams params)
{
	B32 result = params.size > 0 && params.path.size > 0 && params.render_mode < Render_FontRenderMode_COUNT;
	return(result);
}

internal B32
render_font_is_in_queue(Render_Font *font)
{
	B32 result = font->state == Render_FontState_InQueue;
	return(result);
}

internal B32
render_font_is_being_loaded(Render_Font *font)
{
	B32 result = font->state == Render_FontState_Loading;
	return(result);
}

internal B32
render_font_is_loaded(Render_Font *font)
{
	B32 result = font->state == Render_FontState_Loaded;
	return(result != 0);
}

internal B32
render_font_is_unloaded(Render_Font *font)
{
	B32 result = font->state == Render_FontState_Unloaded;
	return(result != 0);
}

internal Render_FontKey
render_key_from_font(Str8 path, U32 font_size)
{
	Render_FontKey result = { 0 };
	result.path = path;
	result.font_size = font_size;
	return(result);
}

internal Render_KerningPair
render_kern_pair_from_glyph_indicies(Render_Font *font, U32 index0, U32 index1)
{
	U64 pair = (U64) index0 << 32 | (U64) index1;
	U64 mask = font->kern_map_size - 1;
	// NOTE(simon): Pseudo fibonacci hashing.
	U64 index = (pair * 11400714819323198485LLU) & mask;
	while (font->kern_pairs[index].pair != pair && font->kern_pairs[index].value != 0.0f)
	{
		index = (index + 1) & mask;
	}

	Render_KerningPair result = font->kern_pairs[index];
	return(result);
}

internal Render_FontAtlas *
render_make_font_atlas(Render_Context *renderer, Vec2U32 dim)
{
	Render_FontAtlas *result = push_struct(renderer->permanent_arena, Render_FontAtlas);
	result->dim = dim;
	Render_FontAtlasRegionNode *first_free_region = push_struct(renderer->permanent_arena, Render_FontAtlasRegionNode);
	first_free_region->region.min = v2u32(0, 0);
	first_free_region->region.max = v2u32(dim.x, dim.x);
	result->memory = push_array(renderer->permanent_arena, U8, dim.x * dim.y * 4);
	render_push_free_region_to_atlas(result, first_free_region);
	result->texture = render_create_texture_from_bitmap(renderer, result->memory, result->dim.x, result->dim.y, Render_ColorSpace_Linear);
	return(result);
}

internal Void
render_push_free_region_to_atlas(Render_FontAtlas *atlas, Render_FontAtlasRegionNode *node)
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

	U8 *data = (U8 *) atlas->memory + (rect_region.min.x + rect_region.min.y * atlas->dim.x)*4;
	U8 *dst = data;
	for (U32 y = 0; y < height; ++y)
	{
		U32 *dst_row = (U32 *) dst;
		for (U32 x = 0; x < width; ++x)
		{
			*dst_row++ = 0;
		}

		dst += (S32) (atlas->dim.x * 4);
	}
}

internal Void
render_remove_free_region_from_atlas(Render_FontAtlas *atlas, Render_FontAtlasRegionNode *node)
{
	dll_remove_npz(atlas->first_free_region, atlas->last_free_region, node, next_free, prev_free, 0);
	node->next_free = 0;
	node->prev_free = 0;
	atlas->num_free_regions--;
}

internal Render_FontAtlasRegion
render_alloc_font_atlas_region(Render_Context *renderer, Render_FontAtlas *atlas, Vec2U32 dim)
{
	assert(atlas->num_free_regions > 0);
	// TODO(hampus): Benchmark and eventually optimize.
	Render_FontAtlasRegionNode *first_free_region = atlas->first_free_region;
	// NOTE(hampus): Each region will always be the same size in
	// x and y, so we only need to check the width
	Render_FontAtlasRegionNode *node = first_free_region;
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
		Render_FontAtlasRegionNode *next = node->next_free;
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
		node->used = true;
		// NOTE(hampus): Remove the current node, it will no longer
		// be free to take because one of its descendants will
		// be taken.
		render_remove_free_region_from_atlas(atlas, node);

		// NOTE(hampus): Allocate 4 children to replace
		// the parent

		if (!node->children[0])
		{
			Render_FontAtlasRegionNode *children = push_array(renderer->permanent_arena, Render_FontAtlasRegionNode, Corner_COUNT);

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
	node->used = true;
	Render_FontAtlasRegion result = { node, node->region };
	return(result);
}

internal Void
render_free_atlas_region(Render_FontAtlas *atlas, Render_FontAtlasRegion region)
{
	Render_FontAtlasRegionNode *node = region.node;
	assert(node->used);
	Render_FontAtlasRegionNode *parent = node->parent;
	render_push_free_region_to_atlas(atlas, node);
	if (parent)
	{
		// NOTE(hampus): Lets check if we can combine
		// any empty nodes to larger nodes. Otherwise
		// we just put it back into the free list
		B32 used = false;
		for (U64 i = 0; i < Corner_COUNT; ++i)
		{
			Render_FontAtlasRegionNode *child = parent->children[i];
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
			render_free_atlas_region(atlas, (Render_FontAtlasRegion) { parent, parent->region });
		}
	}
}

internal Void
render_unload_font(Render_Context *renderer, Render_Font *font)
{
	assert(font);
	assert(renderer);

	os_mutex(&renderer->font_atlas_mutex)
	{
		for (U64 i = 0; i < font->num_font_atlas_regions; ++i)
		{
			Render_FontAtlasRegion font_atlas_region = font->font_atlas_regions[i];
			render_free_atlas_region(renderer->font_atlas, font_atlas_region);
		}
	}
	arena_pop_to(font->arena, 0);
	memory_zero((U8 *) font + sizeof(Arena *), member_offset(Render_Font, state) - sizeof(Arena *));
}

internal Void
render_font_stream_thread(Void *data)
{
	Render_FontLoaderThreadData *thread_data = data;
	Render_Context *renderer = (Render_Context *) thread_data->renderer;
	Render_FontQueue *font_queue = renderer->font_queue;

	thread_ctx_init(thread_data->name);

	for (;;)
	{
		os_semaphore_wait(&font_queue->semaphore);

		// NOTE(simon): We were awoken to load a font. Loop until there are
		// none left to load. This is because u32_atomic_compare_exchange could
		// cause us to not load a font in an iteration, which would then make
		// us wait on the semaphore.
		while (font_queue->write_index - font_queue->read_index != 0)
		{
			// NOTE(simon): Grab the entry before attempting to change the
			// index as that marks it as free for writing.
			U32 queue_read_index = font_queue->read_index;
			Render_FontQueueEntry entry = font_queue->queue[queue_read_index & FONT_QUEUE_MASK];

			memory_fence();

			if (u32_atomic_compare_exchange(&font_queue->read_index, queue_read_index + 1, queue_read_index))
			{
				Render_Font *font = entry.font;

				log_info("Starting to load in font: %"PRISTR8, str8_expand(entry.params.path));
				font->state = Render_FontState_Loading;

				render_unload_font(renderer, font);

				U64 start_timer = os_now_nanoseconds();
				B32 success = render_load_font(renderer, font, entry.params);

				if (success)
				{
					U64 end_timer = os_now_nanoseconds();
					U64 dt = end_timer - start_timer;
					log_info("Successfully loaded font `%"PRISTR8"` in %.4fms", str8_expand(entry.params.path), (F32) dt/(F32) million(1));
				}
				else
				{
					log_warning("Failed to load font `%"PRISTR8"`", str8_expand(entry.params.path));
					render_unload_font(renderer, font);
				}

				memory_fence();

				render_update_texture(
					renderer,
					renderer->font_atlas->texture,
					renderer->font_atlas->memory,
					renderer->font_atlas->dim.width,
					renderer->font_atlas->dim.height,
					0
				);

				if (success)
				{
					font->state = Render_FontState_Loaded;
				}
				else
				{
					font->state = Render_FontState_Unloaded;
				}
			}
		}
	}
}

internal Void
render_push_font_to_queue(Render_Context *renderer, Render_Font *font, Render_FontLoadParams params)
{
	// TODO(hampus): Check the pixel orientation of the monitor.
	assert(renderer);
	assert(render_font_valid_load_params(params));

	Render_FontQueue *font_queue = renderer->font_queue;

	// NOTE(hampus): This is so that we can recongnize that the font
	// is in the queue when we are looking in the cache
	font->state = Render_FontState_InQueue;
	font->load_params = params;

	while (font_queue->write_index - font_queue->read_index >= LOG_QUEUE_SIZE)
	{
		// NOTE(simon): The queue is currently full. Bussy wait until there is
		// space in it.
	}

	Render_FontQueueEntry *entry = &font_queue->queue[font_queue->write_index & FONT_QUEUE_MASK];
	entry->font   = font;
	entry->params = params;

	memory_fence();

	u32_atomic_add(&font_queue->write_index, 1);

	log_info("Pushed font %"PRISTR8" to queue", str8_expand(params.path));

	os_semaphore_signal(&font_queue->semaphore);
}

internal U32
render_glyph_index_from_codepoint(Render_Font *font, U32 codepoint)
{
	assert(font);

	U64 map_mask = font->codepoint_map_size - 1;
	U64 index = ((U64) codepoint * 11400714819323198485LLU) & map_mask;
	while (font->codepoint_map[index].codepoint != codepoint && font->codepoint_map[index].codepoint != U32_MAX)
	{
		index = (index + 1) & map_mask;
	}

	return(font->codepoint_map[index].glyph_index);
}

internal Render_Font *
render_font_from_key(Render_Context *renderer, Render_FontKey font_key)
{
	assert(renderer);
	assert(font_key.font_size > 0);
	assert(font_key.path.size > 0);
	Render_Font *result = 0;

	debug_function()
	{
		S32 unused_slot = -1;
		U64 current_frame_index = renderer->frame_index;
		for (S32 i = 0; i < RENDER_FONT_CACHE_SIZE; ++i)
		{
			Render_Font *font = renderer->font_cache->entries + i;
			if (str8_equal(font->load_params.path, font_key.path) &&
					font->load_params.size == font_key.font_size)
			{
				result = font;
				break;
			}

			B32 slot_is_cold = false;

			if (render_font_is_loaded(font))
			{
				slot_is_cold = font->last_frame_index_used < (current_frame_index-1);
			}
			else if (render_font_is_unloaded(font))
			{
				// NOTE(hampus): This would only be the case
				// if the font hasn't been initialized since
				// the program's start.
				slot_is_cold = true;
			}

			if (slot_is_cold && (unused_slot == -1))
			{
				unused_slot = i;
			}
		}

		if (!result)
		{
			assert(unused_slot != -1 && "Cache is hot and full");
			Render_Font *empty_entry = renderer->font_cache->entries + unused_slot;
			Render_FontLoadParams params =
			{
				.render_mode = Render_FontRenderMode_LCD,
				.size = font_key.font_size,
				.path = font_key.path,
			};
			render_push_font_to_queue(renderer, empty_entry, params);
			result = empty_entry;
		}

		result->last_frame_index_used = renderer->frame_index;
	}

	return(result);
}

internal void
render_glyph(Render_Context *renderer, Vec2F32 min, U32 index, Render_Font *font, Vec4F32 color)
{
	Render_Glyph *glyph = font->glyphs + index;

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
							.is_subpixel_text = RENDER_USE_SUBPIXEL_RENDERING);
}

internal Void
render_text_internal(Render_Context *renderer, Vec2F32 min, Str8 text, Render_Font *font, Vec4F32 color)
{
	debug_function()
	{
		if (render_font_is_loaded(font))
		{
			for (U64 i = 0; i < text.size; ++i)
			{
				U32 codepoint = text.data[i];
				U32 index = render_glyph_index_from_codepoint(font, codepoint);

				// TODO(hampus): Remove this if
				if ((i+1) < text.size)
				{
					U32 next_index = render_glyph_index_from_codepoint(font, text.data[i+1]);
					Render_KerningPair kerning_pair = render_kern_pair_from_glyph_indicies(font, index, next_index);
					min.x += kerning_pair.value;
				}

				render_glyph(renderer, min, index, font, color);
				min.x += (font->glyphs[index].advance_width);
			}
		}
	}
}

internal Void
render_text(Render_Context *renderer, Vec2F32 min, Str8 text, Render_FontKey font_key, Vec4F32 color)
{
	Render_Font *font = render_font_from_key(renderer, font_key);
	render_text_internal(renderer, min, text, font, color);
}

internal Void
render_multiline_text(Render_Context *renderer, Vec2F32 min, Str8 text, Render_FontKey font_key, Vec4F32 color)
{
	Render_Font *font = render_font_from_key(renderer, font_key);

	if (render_font_is_loaded(font))
	{
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

				// TODO(hampus): Remove this if
				if ((i+1) < text.size)
				{
					U32 next_index = render_glyph_index_from_codepoint(font, text.data[i+1]);
					Render_KerningPair kerning_pair = render_kern_pair_from_glyph_indicies(font, index, next_index);
					min.x += kerning_pair.value;
				}

				render_glyph(renderer, min, index, font, color);
				min.x += (font->glyphs[index].advance_width);
			}
		}
	}
}

internal Void
render_character_internal(Render_Context *renderer, Vec2F32 min, U32 codepoint, Render_Font *font, Vec4F32 color)
{
	if (render_font_is_loaded(font))
	{
		U32 index = render_glyph_index_from_codepoint(font, codepoint);

		Render_Glyph *glyph = font->glyphs + index;

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
								.is_subpixel_text = RENDER_USE_SUBPIXEL_RENDERING);
	}
}

internal Void
render_character(Render_Context *renderer, Vec2F32 min, U32 codepoint, Render_FontKey font_key, Vec4F32 color)
{
	Render_Font *font = render_font_from_key(renderer, font_key);
	render_character_internal(renderer, min, codepoint, font, color);
}

internal Vec2F32
render_measure_text_length(Render_Font *font, Str8 text, U64 length)
{
	Vec2F32 result = { 0 };
	debug_function()
	{
		if (render_font_is_loaded(font))
		{
			for (U32 i = 0; i < length; ++i)
			{
				U32 index = render_glyph_index_from_codepoint(font, text.data[i]);
				Render_Glyph *glyph = font->glyphs + index;
				result.x += (glyph->advance_width);

				if (i + 1 < length)
				{
					U32 next_index = render_glyph_index_from_codepoint(font, text.data[i + 1]);
					Render_KerningPair kerning_pair = render_kern_pair_from_glyph_indicies(font, index, next_index);
					result.x += kerning_pair.value;
				}
			}

			result.y = font->line_height;
		}
	}
	return(result);
}

internal Vec2F32
render_measure_text(Render_Font *font, Str8 text)
{
	Vec2F32 result = render_measure_text_length(font, text, text.size);
	return(result);
}

internal Vec2F32
render_measure_character(Render_Font *font, U32 codepoint)
{
	Vec2F32 result = { 0 };
	if (render_font_is_loaded(font))
	{
		U32 index = render_glyph_index_from_codepoint(font, codepoint);
		Render_Glyph *glyph = font->glyphs + index;
		result.x = (glyph->advance_width);
		result.y = font->line_height;
	}
	return(result);
}

internal Vec2F32
render_measure_multiline_text(Render_Font *font, Str8 text)
{
	Vec2F32 result = { 0 };
	if (render_font_is_loaded(font))
	{
		S32 length = (S32) text.size;
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
				Render_Glyph *glyph = font->glyphs + index;
				row_width += (glyph->advance_width);
			}
		}

		result.y += font->line_height;
		result.x = max_row_width;
	}
	return(result);
}
