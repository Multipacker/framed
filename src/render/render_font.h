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
    F32 line_height;
    Str8 font_name;
};

internal R_Font *render_font_init(Arena *arena, R_Context *renderer, Str8 path);
internal void    render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color);
internal Vec2F32 render_measure_text(R_Font *font, Str8 text);

#endif