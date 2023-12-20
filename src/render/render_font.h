#ifndef RENDER_FONT_H
#define RENDER_FONT_H

typedef enum R_FontRenderMode R_FontRenderMode;
enum R_FontRenderMode
{
	R_FontRenderMode_Normal, // Default render mode. 8-bit AA bitmaps
	R_FontRenderMode_LCD,    // Subpixel rendering for horizontally decimated LCD displays
	R_FontRenderMode_LCD_V,  // Subpixel rendering for vertically decimated LCD displays
};

#define R_USE_SUBPIXEL_RENDERING 0

typedef struct R_FontAtlasRegionNode R_FontAtlasRegionNode;
struct R_FontAtlasRegionNode
{
	R_FontAtlasRegionNode *next_free;
	R_FontAtlasRegionNode *prev_free;

	R_FontAtlasRegionNode *parent;
	R_FontAtlasRegionNode *children[Corner_COUNT];
	RectU32 region;

	// NOTE(hampus): This will be true if either this
	// node is used or one of it descendants are used
	// TODO(hampus): Test performance when this is
	// a bitmask instead
	B32 used;
};

typedef struct R_FontAtlasRegion R_FontAtlasRegion;
struct R_FontAtlasRegion
{
	R_FontAtlasRegionNode *node;
	RectU32 region;
};

typedef struct R_FontAtlas R_FontAtlas;
struct R_FontAtlas
{
	R_FontAtlasRegionNode *first_free_region;
	R_FontAtlasRegionNode *last_free_region;
	Vec2U32 dim;
	Void *memory;
	U64 num_free_regions;
	R_Texture texture;
};

typedef struct R_Glyph R_Glyph;
struct R_Glyph
{
	Vec2F32 size_in_pixels;
	Vec2F32 bearing_in_pixels;
	F32 advance_width;
	R_TextureSlice slice;
	// NOTE(hampus): This is needed here so we can easily
	// free the region again
	R_FontAtlasRegion font_atlas_region;
};

typedef struct R_Font R_Font;
struct R_Font
{
	R_Glyph glyphs[128];
	F32 font_size;
	F32 line_height;        // NOTE(hampus): How much vertical spaces a line occupy
	F32 underline_position; // NOTE(hampus): Relative to the baseline
	F32 max_advance_width;
	F32 max_ascent;
	F32 max_descent;
	F32 space_width;
	B32 has_kerning;
	Str8 family_name;
	Str8 style_name;
};

internal R_Font *render_make_font(Arena *arena, S32 font_size, R_Context *renderer, Str8 path);
internal void    render_destroy_font(R_Context *renderer, R_Font *font);

internal void    render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color);
internal Vec2F32 render_measure_text(R_Font *font, Str8 text);

#endif