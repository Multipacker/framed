#ifndef RENDER_CORE_H
#define RENDER_CORE_H

typedef struct R_Texture R_Texture;
struct R_Texture
{
    U64 u64[4];
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
    Vec2F32 min_uv;
    Vec2F32 max_uv;
    // NOTE(hampus): [c00, c10, c11, c01]
    Vec4F32 colors[4];
    F32 radies[4];
    F32 softness;
    F32 border_thickness;
    F32 omit_texture;
    F32 is_subpixel_text;
    F32 use_nearest;
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

typedef enum R_ColorSpace R_ColorSpace;
enum R_ColorSpace
{
    R_ColorSpace_sRGB,
    R_ColorSpace_Linear,

    R_ColorSpace_COUNT,
};

typedef enum R_TextureFilter R_TextureFilter;
enum R_TextureFilter
{
    R_TextureFilter_Bilinear,
    R_TextureFilter_Nearest,

    R_TextureFilter_COUNT
};

typedef struct R_RectParams R_RectParams;
struct R_RectParams
{
    Vec4F32 color;
    F32 radius;
    F32 softness;
    F32 border_thickness;
    R_TextureSlice slice;
    B32 is_subpixel_text;
    B32 use_nearest;
};

// NOTE(simon): This might not always be fully cleared to 0.
global R_RectInstance r_rect_instance_null;

internal R_RectInstance *r_rect_(Vec2F32 min, Vec2F32 max, R_RectParams *params);

internal R_TextureSlice r_slice_from_texture(R_Texture texture, RectF32 uv);
internal R_TextureSlice r_slice_from_texture_region(R_Texture texture, RectU32 region);
internal R_TextureSlice r_create_texture_slice(Str8 path);
internal R_Texture      r_create_texture(Str8 path);
internal R_Texture      r_create_texture_from_bitmap(Void *data, U32 width, U32 height, R_ColorSpace color_space);
internal Void           r_update_texture(R_Texture texture, Void *memory, U32 width, U32 height, U32 offset);

internal F32     f32_srgb_to_linear(F32 value);
internal Vec4F32 vec4f32_srgb_to_linear(Vec4F32 srgb);
internal F32     f32_linear_to_srgb(F32 value);
internal Vec4F32 vec4f32_linear_to_srgb(Vec4F32 linear);

internal R_RenderStats r_get_stats(Void);

internal Void r_push_clip(Vec2F32 min, Vec2F32 max, B32 clip_to_parent);
internal Void r_pop_clip(Void);

internal Void r_init(Void);
internal Void r_begin(Void);
internal Void r_end(Void);

#endif // RENDER_CORE_H
