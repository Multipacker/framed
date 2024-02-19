#ifndef RENDER_CORE_H
#define RENDER_CORE_H

// TODO(hampus):
// [x] - Textures
// [ ] - Finish text rendering
// [ ] - Lines
// [ ] - Push/Pop matrices
// [ ] - Premultiplied alpha

typedef struct Render_Texture Render_Texture;
struct Render_Texture
{
    U64 u64[4];
};

typedef struct Render_TextureSlice Render_TextureSlice;
struct Render_TextureSlice
{
    RectF32 region;
    Render_Texture texture;
};

typedef struct Render_RectInstance Render_RectInstance;
struct Render_RectInstance
{
    Vec2F32 min;
    Vec2F32 max;
    Vec2F32 min_uv;
    Vec2F32 max_uv;
    // NOTE(hampus): [c00, c10, c11, c01]
    Vec4F32 colors[4];
    F32     radies[4];
    F32     softness;
    F32     border_thickness;
    F32     omit_texture;
    F32     is_subpixel_text;
    F32     use_nearest;
};

typedef struct Render_RenderStats Render_RenderStats;
struct Render_RenderStats
{
    U64 draw_time;
    U64 scissor_count;
    U64 rect_count;
    U64 batch_count;
    U64 peak_gpu_memory;
    U64 frame_gpu_memory;
};

typedef enum Render_ColorSpace Render_ColorSpace;
enum Render_ColorSpace
{
    Render_ColorSpace_sRGB,
    Render_ColorSpace_Linear,

    Render_ColorSpace_COUNT,
};

typedef enum Render_TextureFilter Render_TextureFilter;
enum Render_TextureFilter
{
    Render_TextureFilter_Bilinear,
    Render_TextureFilter_Nearest,

    Render_TextureFilter_COUNT
};

typedef struct Render_BackendContext    Render_BackendContext;
typedef struct Render_FontAtlas         Render_FontAtlas;
typedef struct Render_FontCache         Render_FontCache;
typedef struct Render_FontQueue         Render_FontQueue;

typedef struct Render_Context Render_Context;
struct Render_Context
{
    Arena *permanent_arena;
    Arena *frame_arena;
    Gfx_Context *gfx;
    Render_RenderStats render_stats[2]; // [0] is current frame, [1] is previous frame

    Render_FontAtlas *font_atlas;
    Render_FontCache *font_cache;
    Render_FontQueue *font_queue;
    OS_Mutex font_atlas_mutex;

    U64 frame_index;
    Render_BackendContext *backend;
};

// NOTE(simon): This might not always be fully cleared to 0.
global Render_RectInstance render_rect_instance_null;

internal Render_Context *render_init(Gfx_Context *gfx);

internal Void render_begin(Render_Context *renderer);
internal Void render_end(Render_Context *renderer);

typedef struct Render_RectParams Render_RectParams;
struct Render_RectParams
{
    Vec4F32        color;
    F32            radius;
    F32            softness;
    F32            border_thickness;
    Render_TextureSlice slice;
    B32            is_subpixel_text;
    B32            use_nearest;
};

// TODO(hampus): Test performance with/without passing by pointer
#define render_rect(renderer, min, max, ...)    render_rect_(renderer, min, max, &(Render_RectParams) { .color = v4f32(1, 1, 1, 1), __VA_ARGS__ })
#define render_circle(renderer, center, r, ...) render_rect_(renderer, v2f32_sub_f32(center, r), v2f32_add_f32(center, r), &(Render_RectParams) { .color = v4f32(1, 1, 1, 1), .radius = r, __VA_ARGS__ })

internal Render_RectInstance *render_rect_(Render_Context *renderer, Vec2F32 min, Vec2F32 max, Render_RectParams *params);

internal Void render_push_clip(Render_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent);
internal Void render_pop_clip(Render_Context *renderer);

internal Render_RenderStats render_get_stats(Render_Context *renderer);

internal Render_Texture      render_create_texture(Render_Context *renderer, Str8 path);
internal Render_Texture      render_create_texture_from_bitmap(Render_Context *renderer, Void *data, U32 width, U32 height, Render_ColorSpace color_space);

internal Void                render_destroy_texture(Render_Context *renderer, Render_Texture texture);
internal Render_TextureSlice render_slice_from_texture(Render_Texture texture, RectF32 region);
internal Render_TextureSlice render_create_texture_slice(Render_Context *renderer, Str8 path);
internal Void                render_update_texture(Render_Context *renderer, Render_Texture texture, Void *memory, U32 width, U32 height, U32 offset);

internal F32     f32_srgb_to_linear(F32 value);
internal Vec4F32 vec4f32_srgb_to_linear(Vec4F32 srgb);

internal F32     f32_linear_to_srgb(F32 value);
internal Vec4F32 vec4f32_linear_to_srgb(Vec4F32 linear);

internal Void render_backend_begin(Render_Context *renderer);
internal Void render_backend_end(Render_Context *renderer);

#endif //RENDER_CORE_H
