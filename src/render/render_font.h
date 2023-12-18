#ifndef RENDER_FONT_H
#define RENDER_FONT_H

typedef struct R_Glyph R_Glyph;
struct R_Glyph
{
    Vec2F32 size_in_pixels;
    Vec2F32 bearing_in_pixels;
     F32 advance_width;
    R_TextureSlice slice;
};

typedef struct R_Font R_Font;
struct R_Font
{
    R_Glyph glyphs[128];
    F32 size_in_pixels;
    F32 line_height;        // NOTE(hampus): How much vertical spaces a line occupy
    F32 underline_position; // NOTE(hampus): Relative to the baseline
    F32 max_advance_width;
    F32 max_ascent;
    F32 max_descent;
    B32 has_kerning;
    Str8 family_name;
    Str8 style_name;
};

internal R_Font *render_font_init(Arena *arena, R_Context *renderer, Str8 path);
internal void    render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color);
internal Vec2F32 render_measure_text(R_Font *font, Str8 text);

#endif