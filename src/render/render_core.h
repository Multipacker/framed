#ifndef RENDER_CORE_H
#define RENDER_CORE_H

// TODO:
// - Textures
// - Text rendering
// - Lines
// - Icons

typedef struct R_RectInstance R_RectInstance;
struct R_RectInstance
{
	Vec2F32 min;
	Vec2F32 max;
	Vec4F32 colors[4];
	F32     radies[4];
	F32     softness;
	F32     border_thickness;
};

typedef struct R_RenderStats R_RenderStats;
struct R_RenderStats
{
	U64 draw_time;
	U64 scissor_count;
	U64 rect_count;
	U64 batch_count;
	U64 draw_call_count;
	U64 peak_gpu_memory;
	U64 frame_gpu_memory;
};

typedef struct Renderer Renderer;

internal Renderer render_init(Gfx_Context *gfx);

internal Void render_begin(Renderer *renderer);
internal Void render_end(Renderer *renderer);

typedef struct R_RectParams R_RectParams;
struct R_RectParams
{
	Vec4F32 color;
	F32     radius;
	F32     softness;
	F32     border_thickness;
};

// TODO(hampus): Test performance with/without passing by pointer
#define render_rect(renderer, min, max, ...)    render_rect_(renderer, min, max, &(R_RectParams) { .color = v4f32(1, 1, 1, 1), __VA_ARGS__ })
#define render_circle(renderer, center, r, ...) render_rect_(renderer, v2f32_sub_f32(center, r), v2f32_add_f32(center, r), &(R_RectParams) { .color = v4f32(1, 1, 1, 1), .radius = radius, __VA_ARGS__ })

internal R_RectInstance *render_rect_(Renderer *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params);

internal Void render_push_clip(Renderer *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent);
internal Void render_pop_clip(Renderer *renderer)

internal RenderStats render_get_stats(Renderer *renderer);

#endif //RENDER_CORE_H