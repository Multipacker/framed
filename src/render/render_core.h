#ifndef RENDER_CORE_H
#define RENDER_CORE_H

// TODO:
// - Textures
// - Text rendering
// - Lines
// - Icons

typedef struct R_Texture R_Texture;
struct R_Texture
{
    U64 u64[1];
};

typedef struct R_TextureSlice R_TextureSlice;
struct R_TextureSlice
{
    RectF32 region;
    R_Texture texture;
};

typedef struct R_RectInstance R_RectInstance;
struct R_RectInstance
{
	Vec2F32 min;
	Vec2F32 max;
    // NOTE(hampus): [c00, c10, c11, c01]
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
	U64 peak_gpu_memory;
	U64 frame_gpu_memory;
};

typedef enum R_TextureFormat R_TextureFormat;
enum R_TextureFormat
{
    R_TextureFormat_sRGB,
    R_TextureFormat_Linear,

    R_TextureFormat_COUNT,
};

typedef struct R_Context R_Context;

// NOTE(simon): This might not always be fully cleared to 0.
global R_RectInstance render_rect_instance_null;

internal R_Context *render_init(Gfx_Context *gfx);

internal Void render_begin(R_Context *renderer);
internal Void render_end(R_Context *renderer);

typedef struct R_RectParams R_RectParams;
struct R_RectParams
{
	Vec4F32        color;
	F32            radius;
	F32            softness;
	F32            border_thickness;
        R_TextureSlice slice;
};

// TODO(hampus): Test performance with/without passing by pointer
#define render_rect(renderer, min, max, ...)    render_rect_(renderer, min, max, &(R_RectParams) { .color = v4f32(1, 1, 1, 1), __VA_ARGS__ })
#define render_circle(renderer, center, r, ...) render_rect_(renderer, v2f32_sub_f32(center, r), v2f32_add_f32(center, r), &(R_RectParams) { .color = v4f32(1, 1, 1, 1), .radius = r, __VA_ARGS__ })

internal R_RectInstance *render_rect_(R_Context *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params);

internal Void render_push_clip(R_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent);
internal Void render_pop_clip(R_Context *renderer);

internal R_RenderStats render_get_stats(R_Context *renderer);

internal R_Texture      render_create_texture(R_Context *renderer, Str8 path, R_TextureFormat format);
internal Void           render_destroy_texture(R_Context *renderer, R_Texture texture);
internal R_TextureSlice render_slice_from_texture(R_Texture texture, RectF32 region);
internal R_TextureSlice render_create_texture_slice(R_Context *renderer, Str8 path, R_TextureFormat format);

internal F32     f32_srgb_to_linear(F32 value);
internal Vec4F32 vec4f32_srgb_to_linear(Vec4F32 srgb);

internal F32     f32_linear_to_srgb(F32 value);
internal Vec4F32 vec4f32_linear_to_srgb(Vec4F32 linear);

#endif //RENDER_CORE_H
