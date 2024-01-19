internal Render_TextureSlice
render_slice_from_texture(Render_Texture texture, RectF32 uv)
{
	Render_TextureSlice result = { uv, texture };
	return(result);
}

internal Render_TextureSlice
render_slice_from_texture_region(Render_Texture texture, RectU32 region)
{
#if 0
	RectF32 uv = rectf32_from_rectu32(region);
	Render_TextureSlice result = { region, texture };
	return(result);
#endif
	Render_TextureSlice result = { 0 };
	return(result);
}

internal Render_TextureSlice
render_create_texture_slice(Render_Context *renderer, Str8 path)
{
	Render_TextureSlice result;
	result.texture = render_create_texture(renderer, path);
	result.region.min = v2f32(0, 0);
	result.region.max = v2f32(1, 1);
	return(result);
}

internal F32
f32_srgb_to_linear(F32 value)
{
	F32 result = 0.0f;
	if (value < 0.04045f)
	{
		result = value / 12.92f;
	}
	else
	{
		result = f32_pow((value + 0.055f) / 1.055f, 2.4f);
	}
	return(result);
}

internal Vec4F32
vec4f32_srgb_to_linear(Vec4F32 srgb)
{
	Vec4F32 result = v4f32(
						   f32_srgb_to_linear(srgb.r),
						   f32_srgb_to_linear(srgb.g),
						   f32_srgb_to_linear(srgb.b),
						   srgb.a
						   );
	return(result);
}

internal F32
f32_linear_to_srgb(F32 value)
{
	F32 result = 0.0f;
	if (value < 0.0031308f)
	{
		result  = value * 12.92f;
	}
	else
	{
		result = 1.055f * f32_pow(value, 1.0f / 2.4f) - 0.055f;
	}
	return(result);
}

internal Vec4F32
vec4f32_linear_to_srgb(Vec4F32 linear)
{
	Vec4F32 result = v4f32(
						   f32_linear_to_srgb(linear.r),
						   f32_linear_to_srgb(linear.g),
						   f32_linear_to_srgb(linear.b),
						   linear.a
						   );
	return(result);
}

internal Render_RenderStats
render_get_stats(Render_Context *renderer)
{
	return(renderer->render_stats[1]);
}

typedef struct Render_FontLoaderThreadData Render_FontLoaderThreadData;
struct Render_FontLoaderThreadData
{
	Render_Context *renderer;
	U32 id;
	Str8 name;
};

internal Render_Context *
render_init(Gfx_Context *gfx)
{
	Arena *arena = arena_create();
	Render_Context *renderer = push_struct(arena, Render_Context);
	renderer->gfx             = gfx;
	renderer->permanent_arena = arena;
	renderer->frame_arena     = arena_create();
	renderer->backend         = render_backend_init(renderer);

	renderer->font_atlas = render_make_font_atlas(renderer, v2u32(2048, 2048));
	renderer->font_cache = push_struct(arena, Render_FontCache);
	for (U64 i = 0; i < RENDER_FONT_CACHE_SIZE; ++i)
	{
		renderer->font_cache->entries[i].arena = arena_create();
	}

	// NOTE(simon): This is needed for atomic reads.
	arena_align(arena, 8);
	renderer->font_queue = push_struct(arena, Render_FontQueue);
	renderer->font_queue->queue = push_array(arena, Render_FontQueueEntry, FONT_QUEUE_SIZE);
	os_semaphore_create(&renderer->font_queue->semaphore, 0);

	os_mutex_create(&renderer->font_atlas_mutex);

	for (U32 i = 0; i < 4; ++i)
	{
		Render_FontLoaderThreadData *data = push_struct( renderer->permanent_arena, Render_FontLoaderThreadData);
		data->id = i;
		data->renderer = renderer;
		data->name = str8_pushf(renderer->permanent_arena, "FontLoader%d", i);
		os_thread_create(render_font_stream_thread, data);
	}

	return(renderer);
}

internal Void
render_begin(Render_Context *renderer)
{
	render_backend_begin(renderer);
}

internal Void
render_end(Render_Context *renderer)
{
	debug_function()
	{
		render_backend_end(renderer);
		renderer->frame_index++;
		arena_pop_to(renderer->frame_arena, 0);
	}
}
