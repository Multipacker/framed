#ifndef DRAW_CORE_H
#define DRAW_CORE_H

// NOTE(simon): Initialization
internal Void d_init(Void);

// NOTE(simon): Frame markers
internal Void d_begin_frame(Void);
internal Void d_submit(Void);

// NOTE(simon): Draw commands
typedef struct D_ShapeParams D_ShapeParams;
struct D_ShapeParams
{
    Vec4F32 color;
    F32 radius;
    F32 softness;
    F32 border_thickness;
    R_TextureSlice slice;
    B32 is_subpixel_text;
    B32 use_nearest;
};

internal R_Shape *d_rect_(Vec2F32 min, Vec2F32 max, D_ShapeParams *parameters);

#define d_rect(min, max, ...)    d_rect_(min, max, &(D_ShapeParams){.color = v4f32(1, 1, 1, 1), __VA_ARGS__})
#define d_circle(center, r, ...) d_rect_(v2f32_sub_f32(center, r), v2f32_add_f32(center, r), &(D_ShapeParams){.color = v4f32(1, 1, 1, 1), .radius = r, __VA_ARGS__})

internal Void d_push_clip(Vec2F32 min, Vec2F32 max);
internal Void d_pop_clip(Void);

internal Void d_character_internal(Vec2F32 min, U32 codepoint, R_Font *font, Vec4F32 color);
internal Void d_text_internal(Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color);

internal Vec2F32 d_measure_character(R_Font *font, U32 codepoint);
internal Vec2F32 d_measure_text(R_Font *font, Str8 text);
internal Vec2F32 d_measure_text_length(R_Font *font, Str8 text, U64 length);

#endif // DRAW_CORE_H
