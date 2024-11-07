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

typedef struct R_Shape R_Shape;
struct R_Shape
{
    Vec2F32 min;
    Vec2F32 max;
    Vec2F32 min_uv;
    Vec2F32 max_uv;
    Vec4F32 colors[4];
    F32 radies[4];
    F32 softness;
    F32 border_thickness;
    F32 omit_texture;
    F32 is_subpixel_text;
    F32 use_nearest;
};

typedef struct R_ShapeChunk R_ShapeChunk;
struct R_ShapeChunk
{
    R_ShapeChunk *next;
    R_Shape *shapes;
    U64 count;
    U64 capacity;
};

typedef struct R_ShapeList R_ShapeList;
struct R_ShapeList
{
    R_ShapeChunk *first;
    R_ShapeChunk *last;
    U64 shape_count;
    U64 chunk_count;
};

typedef struct R_Batch R_Batch;
struct R_Batch
{
    R_Batch *next;
    RectF32 clip_rect;
    R_Texture texture;
    R_ShapeList shapes;
};

typedef struct R_BatchList R_BatchList;
struct R_BatchList
{
    R_Batch *first;
    R_Batch *last;
    U64 count;
};

internal R_Texture      r_texture_zero(Void);
internal B32            r_texture_equal(R_Texture a, R_Texture b);
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

internal Void r_init(Void);
internal Void r_begin(Void);
internal Void r_submit(R_BatchList batches);
internal Void r_end(Void);

#endif // RENDER_CORE_H
