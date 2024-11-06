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

internal Void
r_font_init(Void)
{
    Arena *arena = arena_create("FontPerm");

    r_font_context.permanent_arena = arena;

    r_font_context.font_atlas = r_make_font_atlas(v2u32(2048, 2048));
    r_font_context.font_cache = push_struct(arena, R_FontCache);
    for (U64 i = 0; i < R_FONT_CACHE_SIZE; ++i)
    {
        r_font_context.font_cache->entries[i].arena = arena_create("FontCacheEntry%" PRIU64, i);
    }

    // NOTE(simon): This is needed for atomic reads.
    arena_align(arena, 8);
    r_font_context.font_queue        = push_struct(arena, R_FontQueue);
    r_font_context.font_queue->queue = push_array(arena, R_FontQueueEntry, FONT_QUEUE_SIZE);
    os_semaphore_create(&r_font_context.font_queue->semaphore, 0);

    os_mutex_create(&r_font_context.font_atlas_mutex);

    for (U32 i = 0; i < 4; ++i)
    {
        R_FontLoaderThreadData *data = push_struct(arena, R_FontLoaderThreadData);
        data->id                     = i;
        data->name                   = str8_pushf(arena, "FontLoader%d", i);
        os_thread_create(r_font_stream_thread, data);
    }
}

internal Void
r_font_end_frame(Void)
{
    ++r_font_context.frame_index;
}

internal B32
r_font_valid_load_params(R_FontLoadParams params)
{
    B32 result = params.size > 0 && params.path.size > 0 && params.render_mode < R_FontRenderMode_COUNT;
    return (result);
}

internal B32
r_font_is_in_queue(R_Font *font)
{
    B32 result = font->state == R_FontState_InQueue;
    return (result);
}

internal B32
r_font_is_being_loaded(R_Font *font)
{
    B32 result = font->state == R_FontState_Loading;
    return (result);
}

internal B32
r_font_is_loaded(R_Font *font)
{
    B32 result = font->state == R_FontState_Loaded;
    return (result != 0);
}

internal B32
r_font_is_unloaded(R_Font *font)
{
    B32 result = font->state == R_FontState_Unloaded;
    return (result != 0);
}

internal R_FontKey
r_key_from_font(Str8 path, U32 font_size)
{
    R_FontKey result = {0};
    result.path      = path;
    result.font_size = font_size;
    return (result);
}

internal R_KerningPair
r_kern_pair_from_glyph_indicies(R_Font *font, U32 index0, U32 index1)
{
    U64 pair = (U64) index0 << 32 | (U64) index1;
    U64 mask = font->kern_map_size - 1;
    // NOTE(simon): Pseudo fibonacci hashing.
    U64 index = (pair * 11400714819323198485LLU) & mask;
    while (font->kern_pairs[index].pair != pair && font->kern_pairs[index].value != 0.0f)
    {
        index = (index + 1) & mask;
    }

    R_KerningPair result = font->kern_pairs[index];
    return (result);
}

internal R_FontAtlas *
r_make_font_atlas(Vec2U32 dim)
{
    R_FontAtlas *result                      = push_struct(r_font_context.permanent_arena, R_FontAtlas);
    result->dim                              = dim;
    R_FontAtlasRegionNode *first_free_region = push_struct(r_font_context.permanent_arena, R_FontAtlasRegionNode);
    first_free_region->region.min            = v2u32(0, 0);
    first_free_region->region.max            = v2u32(dim.x, dim.x);
    result->memory                           = push_array(r_font_context.permanent_arena, U8, dim.x * dim.y * 4);
    r_push_free_region_to_atlas(result, first_free_region);
    result->texture = r_create_texture_from_bitmap(result->memory, result->dim.x, result->dim.y, R_ColorSpace_Linear);
    return (result);
}

internal Void
r_push_free_region_to_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node)
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
    node->used               = false;
    atlas->num_free_regions++;

    RectU32 rect_region = node->region;

    U32 width  = rect_region.x1 - rect_region.x0;
    U32 height = rect_region.y1 - rect_region.y0;

    U8 *data = (U8 *) atlas->memory + (rect_region.min.x + rect_region.min.y * atlas->dim.x) * 4;
    U8 *dst  = data;
    for (U32 y = 0; y < height; ++y)
    {
        U32 *dst_row = (U32 *) dst;
        for (U32 x = 0; x < width; ++x)
        {
            *dst_row++ = 0;
        }

        dst += atlas->dim.x * 4;
    }
}

internal Void
r_remove_free_region_from_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node)
{
    dll_remove_npz(atlas->first_free_region, atlas->last_free_region, node, next_free, prev_free, 0);
    node->next_free = 0;
    node->prev_free = 0;
    atlas->num_free_regions--;
}

internal R_FontAtlasRegion
r_alloc_font_atlas_region(R_FontAtlas *atlas, Vec2U32 dim)
{
    assert(atlas->num_free_regions > 0);
    // TODO(hampus): Benchmark and eventually optimize.
    R_FontAtlasRegionNode *first_free_region = atlas->first_free_region;
    // NOTE(hampus): Each region will always be the same size in
    // x and y, so we only need to check the width
    R_FontAtlasRegionNode *node = first_free_region;
    U32 required_size           = u32_max(dim.x, dim.y);
    U32 region_size             = node->region.max.x - node->region.min.x;
    B32 can_halve_size          = region_size >= (required_size * 2);
    B32 fits                    = region_size >= required_size;

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
        node        = node->next_free;
        region_size = node->region.max.x - node->region.min.x;
        fits        = region_size > required_size;
    }

    // NOTE(hampus): Find the best that fit
    B32 find_best_fit = true;
    if (find_best_fit)
    {
        R_FontAtlasRegionNode *next = node->next_free;
        while (next)
        {
            U32 next_region_size = next->region.max.x - next->region.min.x;
            fits                 = next_region_size >= required_size;
            if (fits)
            {
                if (next_region_size < region_size)
                {
                    node        = next;
                    region_size = node->region.max.x - node->region.min.x;
                }
            }

            next = next->next_free;
        }
    }

    can_halve_size = region_size >= (required_size * 2);

    while (can_halve_size)
    {
        node->used = true;
        // NOTE(hampus): Remove the current node, it will no longer
        // be free to take because one of its descendants will
        // be taken.
        r_remove_free_region_from_atlas(atlas, node);

        // NOTE(hampus): Allocate 4 children to replace
        // the parent

        if (!node->children[0])
        {
            R_FontAtlasRegionNode *children = push_array(r_font_context.permanent_arena, R_FontAtlasRegionNode, Corner_COUNT);

            {
                Vec2U32 bbox[Corner_COUNT] =
                    {
                        bbox[Corner_TopLeft]     = node->region.min,
                        bbox[Corner_TopRight]    = v2u32(node->region.max.x, node->region.min.y),
                        bbox[Corner_BottomLeft]  = v2u32(node->region.min.x, node->region.max.y),
                        bbox[Corner_BottomRight] = node->region.max,
                    };

                Vec2U32 middle = v2u32_div_u32(v2u32_add_v2u32(bbox[Corner_BottomRight], bbox[Corner_TopLeft]), 2);

                children[Corner_TopLeft].region = rectu32(bbox[Corner_TopLeft], middle);

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
                node->children[i]  = &children[i];
            }
        }

        for (U64 i = 0; i < Corner_COUNT; ++i)
        {
            r_push_free_region_to_atlas(atlas, node->children[3 - i]);
        }

        node = node->children[0];

        region_size    = node->region.max.x - node->region.min.x;
        can_halve_size = region_size >= (required_size * 2);
    }

    r_remove_free_region_from_atlas(atlas, node);

    node->next_free          = 0;
    node->prev_free          = 0;
    node->used               = true;
    R_FontAtlasRegion result = {node, node->region};
    return (result);
}

internal Void
r_free_atlas_region(R_FontAtlas *atlas, R_FontAtlasRegion region)
{
    R_FontAtlasRegionNode *node = region.node;
    assert(node->used);
    R_FontAtlasRegionNode *parent = node->parent;
    r_push_free_region_to_atlas(atlas, node);
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
                r_remove_free_region_from_atlas(atlas, parent->children[i]);
            }
            r_free_atlas_region(atlas, (R_FontAtlasRegion){parent, parent->region});
        }
    }
}

internal Void
r_unload_font(R_Font *font)
{
    assert(font);

    os_mutex(&r_font_context.font_atlas_mutex)
    {
        for (U64 i = 0; i < font->num_font_atlas_regions; ++i)
        {
            R_FontAtlasRegion font_atlas_region = font->font_atlas_regions[i];
            r_free_atlas_region(r_font_context.font_atlas, font_atlas_region);
        }
    }
    arena_pop_to(font->arena, 0);
    memory_zero((U8 *) font + sizeof(Arena *), member_offset(R_Font, state) - sizeof(Arena *));
}

internal B32 r_load_font_truetype(R_Font *font, R_FontLoadParams params);

internal Void
r_font_stream_thread(Void *data)
{
    R_FontLoaderThreadData *thread_data = data;
    R_FontQueue *font_queue             = r_font_context.font_queue;

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
            U32 queue_read_index   = font_queue->read_index;
            R_FontQueueEntry entry = font_queue->queue[queue_read_index & FONT_QUEUE_MASK];

            memory_fence();

            if (u32_atomic_compare_exchange(&font_queue->read_index, queue_read_index + 1, queue_read_index))
            {
                R_Font *font = entry.font;

                log_info("Starting to load in font: %" PRISTR8, str8_expand(entry.params.path));
                font->state = R_FontState_Loading;

                r_unload_font(font);

                U64 start_timer = os_now_nanoseconds();

                B32 success = r_load_font_truetype(font, entry.params);

                if (success)
                {
                    U64 end_timer = os_now_nanoseconds();
                    U64 dt        = end_timer - start_timer;
                    log_info("Successfully loaded font `%" PRISTR8 "` in %.4fms", str8_expand(entry.params.path), (F32) dt / (F32) million(1));
                }
                else
                {
                    log_warning("Failed to load font `%" PRISTR8 "`", str8_expand(entry.params.path));
                    r_unload_font(font);
                }

                memory_fence();

                r_update_texture(
                    r_font_context.font_atlas->texture,
                    r_font_context.font_atlas->memory,
                    r_font_context.font_atlas->dim.width,
                    r_font_context.font_atlas->dim.height,
                    0
                );

                if (success)
                {
                    font->state = R_FontState_Loaded;
                }
                else
                {
                    font->state = R_FontState_Unloaded;
                }
            }
        }
    }
}

internal Void
r_push_font_to_queue(R_Font *font, R_FontLoadParams params)
{
    // TODO(hampus): Check the pixel orientation of the monitor.
    assert(r_font_valid_load_params(params));

    R_FontQueue *font_queue = r_font_context.font_queue;

    // NOTE(hampus): This is so that we can recongnize that the font
    // is in the queue when we are looking in the cache
    font->state       = R_FontState_InQueue;
    font->load_params = params;

    while (font_queue->write_index - font_queue->read_index >= FONT_QUEUE_SIZE)
    {
        // NOTE(simon): The queue is currently full. Bussy wait until there is
        // space in it.
    }

    R_FontQueueEntry *entry = &font_queue->queue[font_queue->write_index & FONT_QUEUE_MASK];
    entry->font             = font;
    entry->params           = params;

    memory_fence();

    u32_atomic_add(&font_queue->write_index, 1);

    log_info("Pushed font %" PRISTR8 " to queue", str8_expand(params.path));

    os_semaphore_signal(&font_queue->semaphore);
}

internal U32
r_glyph_index_from_codepoint(R_Font *font, U32 codepoint)
{
    assert(font);

    U64 map_mask = font->codepoint_map_size - 1;
    U64 index    = ((U64) codepoint * 11400714819323198485LLU) & map_mask;
    while (font->codepoint_map[index].codepoint != codepoint && font->codepoint_map[index].codepoint != U32_MAX)
    {
        index = (index + 1) & map_mask;
    }

    return (font->codepoint_map[index].glyph_index);
}

internal R_Font *
r_font_from_key(R_FontKey font_key)
{
    profile_begin_function();
    Vec2F32 scale      = gfx_scale_from_window();
    font_key.font_size = (U32) ((F32) font_key.font_size * scale.y);
    assert(font_key.font_size > 0);
    assert(font_key.path.size > 0);
    R_Font *result = 0;

    S32 unused_slot         = -1;
    U64 current_frame_index = r_font_context.frame_index;
    for (S32 i = 0; i < R_FONT_CACHE_SIZE; ++i)
    {
        R_Font *font = r_font_context.font_cache->entries + i;
        if (str8_equal(font->load_params.path, font_key.path) &&
            font->load_params.size == font_key.font_size)
        {
            result = font;
            break;
        }

        B32 slot_is_cold = false;

        if (r_font_is_loaded(font))
        {
            slot_is_cold = font->last_frame_index_used < (current_frame_index - 1);
        }
        else if (r_font_is_unloaded(font))
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
        R_Font *empty_entry = r_font_context.font_cache->entries + unused_slot;
        R_FontLoadParams params =
            {
                .render_mode = R_FontRenderMode_LCD,
                .size   = font_key.font_size,
                .path   = font_key.path,
            };
        r_push_font_to_queue(empty_entry, params);
        result = empty_entry;
    }

    result->last_frame_index_used = r_font_context.frame_index;

    profile_end_function();
    return (result);
}

internal Void
r_glyph(Vec2F32 min, U32 index, R_Font *font, Vec4F32 color)
{
    profile_begin_function();
    R_Glyph *glyph = font->glyphs + index;

    F32 xpos = min.x + glyph->bearing_in_pixels.x;
    F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

    F32 width  = (F32) glyph->size_in_pixels.x;
    F32 height = (F32) glyph->size_in_pixels.y;

    d_rect(
        v2f32(xpos, ypos),
        v2f32(xpos + width, ypos + height),
        .slice            = glyph->slice,
        .color            = color,
        .is_subpixel_text = R_USE_SUBPIXEL_RENDERING
    );
    profile_end_function();
}

internal Void
r_text_internal(Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color)
{
    profile_begin_function();
    if (r_font_is_loaded(font))
    {
        arena_scratch(0, 0)
        {
            U32 *glyph_indicies = push_array(scratch, U32, text.size);
            U64 count           = 0;

            U8 *ptr = text.data;
            U8 *opl = text.data + text.size;

            while (ptr < opl)
            {
                StringDecode decode   = string_decode_utf8(ptr, (U64) (opl - ptr));
                glyph_indicies[count] = r_glyph_index_from_codepoint(font, decode.codepoint);

                ++count;
                ptr += decode.size;
            }

            for (U64 i = 0; i < count; ++i)
            {
                U32 index = glyph_indicies[i];

                // TODO(hampus): Remove this if
                if (i + 1 < count)
                {
                    U32 next_index             = glyph_indicies[i + 1];
                    R_KerningPair kerning_pair = r_kern_pair_from_glyph_indicies(font, index, next_index);
                    min.x += kerning_pair.value;
                }

                r_glyph(min, index, font, color);
                min.x += font->glyphs[index].advance_width;
            }
        }
    }
    profile_end_function();
}

internal Void
r_text(Vec2F32 min, Str8 text, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = r_font_from_key(font_key);
    r_text_internal(min, text, font, color);
}

internal Void
r_multiline_text(Vec2F32 min, Str8 text, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = r_font_from_key(font_key);

    if (r_font_is_loaded(font))
    {
        arena_scratch(0, 0)
        {
            U32 *codepoints     = push_array(scratch, U32, text.size);
            U32 *glyph_indicies = push_array(scratch, U32, text.size);
            U64 count           = 0;

            U8 *ptr = text.data;
            U8 *opl = text.data + text.size;

            while (ptr < opl)
            {
                StringDecode decode   = string_decode_utf8(ptr, (U64) (opl - ptr));
                codepoints[count]     = decode.codepoint;
                glyph_indicies[count] = r_glyph_index_from_codepoint(font, decode.codepoint);

                ++count;
                ptr += decode.size;
            }

            Vec2F32 origin = min;
            for (U64 i = 0; i < count; ++i)
            {
                if (codepoints[i] == '\n')
                {
                    min.x = origin.x;
                    min.y += font->line_height;
                }
                else
                {
                    U32 index = glyph_indicies[i];

                    // TODO(hampus): Remove this if
                    if (i + 1 < count)
                    {
                        U32 next_index             = glyph_indicies[i + 1];
                        R_KerningPair kerning_pair = r_kern_pair_from_glyph_indicies(font, index, next_index);
                        min.x += kerning_pair.value;
                    }

                    r_glyph(min, index, font, color);
                    min.x += font->glyphs[index].advance_width;
                }
            }
        }
    }
}

internal Void
r_character_internal(Vec2F32 min, U32 codepoint, R_Font *font, Vec4F32 color)
{
    profile_begin_function();
    if (r_font_is_loaded(font))
    {
        U32 index = r_glyph_index_from_codepoint(font, codepoint);

        R_Glyph *glyph = font->glyphs + index;

        F32 xpos = min.x + glyph->bearing_in_pixels.x;
        F32 ypos = min.y + (-glyph->bearing_in_pixels.y) + (font->max_ascent);

        F32 width  = (F32) glyph->size_in_pixels.x;
        F32 height = (F32) glyph->size_in_pixels.y;

        d_rect(
            v2f32(xpos, ypos),
            v2f32(xpos + width, ypos + height),
            .slice            = glyph->slice,
            .color            = color,
            .is_subpixel_text = R_USE_SUBPIXEL_RENDERING
        );
    }
    profile_end_function();
}

internal Void
r_character(Vec2F32 min, U32 codepoint, R_FontKey font_key, Vec4F32 color)
{
    R_Font *font = r_font_from_key(font_key);
    r_character_internal(min, codepoint, font, color);
}

internal Vec2F32
r_measure_text(R_Font *font, Str8 text)
{
    profile_begin_function();
    Vec2F32 result = {0};
    if (r_font_is_loaded(font))
    {
        arena_scratch(0, 0)
        {
            U32 *glyph_indicies = push_array(scratch, U32, text.size);
            U64 count           = 0;

            U8 *ptr = text.data;
            U8 *opl = text.data + text.size;

            while (ptr < opl)
            {
                StringDecode decode   = string_decode_utf8(ptr, (U64) (opl - ptr));
                glyph_indicies[count] = r_glyph_index_from_codepoint(font, decode.codepoint);

                ++count;
                ptr += decode.size;
            }

            for (U64 i = 0; i < count; ++i)
            {
                U32 index      = glyph_indicies[i];
                R_Glyph *glyph = font->glyphs + index;
                result.x += glyph->advance_width;

                if (i + 1 < count)
                {
                    U32 next_index             = glyph_indicies[i + 1];
                    R_KerningPair kerning_pair = r_kern_pair_from_glyph_indicies(font, index, next_index);
                    result.x += kerning_pair.value;
                }
            }
        }

        result.y = font->line_height;
    }
    profile_end_function();
    return (result);
}

internal Vec2F32
r_measure_character(R_Font *font, U32 codepoint)
{
    profile_begin_function();
    Vec2F32 result = {0};
    if (r_font_is_loaded(font))
    {
        U32 index      = r_glyph_index_from_codepoint(font, codepoint);
        R_Glyph *glyph = font->glyphs + index;
        result.x       = (glyph->advance_width);
        result.y       = font->line_height;
    }
    profile_end_function();
    return (result);
}

internal Vec2F32
r_measure_multiline_text(R_Font *font, Str8 text)
{
    profile_begin_function();

    Vec2F32 result = {0};
    if (r_font_is_loaded(font))
    {
        arena_scratch(0, 0)
        {
            U32 *codepoints     = push_array(scratch, U32, text.size);
            U32 *glyph_indicies = push_array(scratch, U32, text.size);
            U32 count           = 0;

            U8 *ptr = text.data;
            U8 *opl = text.data + text.size;

            while (ptr < opl)
            {
                StringDecode decode   = string_decode_utf8(ptr, (U64) (opl - ptr));
                codepoints[count]     = decode.codepoint;
                glyph_indicies[count] = r_glyph_index_from_codepoint(font, decode.codepoint);

                ++count;
                ptr += decode.size;
            }

            F32 max_row_width = 0;
            F32 row_width     = 0;
            for (U64 i = 0; i < count; ++i)
            {
                if (codepoints[i] == '\n')
                {
                    max_row_width = f32_max(row_width, max_row_width);
                    row_width     = 0;
                    result.y += font->line_height;
                }
                else
                {
                    U32 index           = glyph_indicies[i];
                    R_Glyph *glyph = font->glyphs + index;
                    row_width += glyph->advance_width;

                    // TODO(hampus): Remove this if
                    if (i + 1 < count)
                    {
                        U32 next_index             = glyph_indicies[i + 1];
                        R_KerningPair kerning_pair = r_kern_pair_from_glyph_indicies(font, index, next_index);
                        row_width += kerning_pair.value;
                    }
                }
            }

            result.y += font->line_height;
            result.x = f32_max(max_row_width, row_width);
        }
    }
    profile_end_function();
    return (result);
}
