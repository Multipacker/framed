#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_BMP
#define STBI_NO_STDIO

#if COMPILER_CLANG
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wconversion"
#endif
#include "stb_image.h"
#if COMPILER_CLANG
# pragma clang diagnostic pop
#endif

internal R_TextureSlice
render_slice_from_texture(R_Texture texture, RectF32 uv)
{
	R_TextureSlice result = { uv, texture };
	return(result);
}

internal R_TextureSlice
render_slice_from_texture_region(R_Texture texture, RectU32 region)
{
#if 0
	RectF32 uv = rectf32_from_rectu32(region);
	R_TextureSlice result = { region, texture };
	return(result);
#endif
	R_TextureSlice result = { 0 };
	return(result);
}

internal R_TextureSlice
render_create_texture_slice(R_Context *renderer, Str8 path, R_ColorSpace color_space)
{
	R_TextureSlice result;
	result.texture = render_create_texture(renderer, path, color_space);
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

internal R_RenderStats
render_get_stats(R_Context *renderer)
{
	return(renderer->render_stats[1]);
}

internal R_Context *
render_init(Gfx_Context *gfx)
{
	Arena *arena = arena_create();
	R_Context *renderer = push_struct(arena, R_Context);
	renderer->gfx = gfx;
	renderer->permanent_arena       = arena;
	renderer->frame_arena = arena_create();
	renderer->backend = render_backend_init(renderer);

	renderer->font_atlas = render_make_font_atlas(renderer, v2u32(2048, 2048));
	renderer->font_cache = push_struct(arena, R_FontCache);
	for (U64 i = 0; i < R_FONT_CACHE_SIZE; ++i)
	{
		R_Font *font = renderer->font_cache->entries + i;
		Arena *font_arena = arena_create();
		font->arena = font_arena;
	}

	renderer->font_queue = push_struct(arena, R_FontQueue);
	renderer->font_queue->queue = push_array(arena, R_FontQueueEntry, FONT_QUEUE_SIZE);

	renderer->dirty_font_region_queue = push_struct(arena, R_DirtyFontRegionQueue);
	renderer->dirty_font_region_queue->queue = push_array(arena, RectU32, FONT_QUEUE_SIZE);

	os_semaphore_create(&renderer->font_loader_semaphore, 0);

	os_thread_create(render_font_loader_thread, renderer);

	return(renderer);
}

internal Void
render_begin(R_Context *renderer)
{
	render_backend_begin(renderer);
}

internal Void
render_end(R_Context *renderer)
{
	R_DirtyFontRegionQueue *dirty_font_region_queue = renderer->dirty_font_region_queue;
	if ((dirty_font_region_queue->queue_read_index - dirty_font_region_queue->queue_write_index) != 0)
	{
		RectU32 *entry = &dirty_font_region_queue->queue[dirty_font_region_queue->queue_read_index & FONT_QUEUE_MASK];

		render_update_texture(renderer, renderer->font_atlas->texture, renderer->font_atlas->memory, renderer->font_atlas->dim.width, renderer->font_atlas->dim.height, 0);

		memory_fence();

		++dirty_font_region_queue->queue_read_index;
	}

	render_backend_end(renderer);
	renderer->frame_index++;
	arena_pop_to(renderer->frame_arena, 0);
}