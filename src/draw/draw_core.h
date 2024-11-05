#ifndef DRAW_CORE_H
#define DRAW_CORE_H

// NOTE(simon): Initialization
internal Void draw_init(Void);

// NOTE(simon): Frame markers
internal Void draw_begin_frame(Void);
internal Void draw_submit(Void);

// NOTE(simon): Draw commands
typedef struct Draw_RectParams Draw_RectParams;
struct Draw_RectParams
{
    Vec4F32 color;
    F32 radius;
    F32 softness;
    F32 border_thickness;
    Render_TextureSlice slice;
    B32 is_subpixel_text;
    B32 use_nearest;
};

internal Render_RectInstance *draw_rect_(Vec2F32 min, Vec2F32 max, Draw_RectParams *parameters);

#define draw_rect(min, max, ...)    draw_rect_(min, max, &(Draw_RectParams){.color = v4f32(1, 1, 1, 1), __VA_ARGS__})
#define draw_circle(center, r, ...) draw_rect_(v2f32_sub_f32(center, r), v2f32_add_f32(center, r), &(Draw_RectParams){.color = v4f32(1, 1, 1, 1), .radius = r, __VA_ARGS__})

internal Void draw_push_clip(Vec2F32 min, Vec2F32 max);
internal Void draw_pop_clip(Void);

internal Void draw_character_internal(Vec2F32 min, U32 codepoint, Render_Font *font, Vec4F32 color);
internal Void draw_text_internal(Vec2F32 min, Str8 text, Render_Font *font, Vec4F32 color);

internal Vec2F32 draw_measure_character(Render_Font *font, U32 codepoint);
internal Vec2F32 draw_measure_text(Render_Font *font, Str8 text);
internal Vec2F32 draw_measure_text_length(Render_Font *font, Str8 text, U64 length);

#endif // DRAW_CORE_H
