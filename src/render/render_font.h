#ifndef RENDER_FONT_H
#define RENDER_FONT_H

typedef enum R_FontRenderMode R_FontRenderMode;
enum R_FontRenderMode
{
	R_FontRenderMode_Normal, // Default render mode. 8-bit AA bitmaps
	R_FontRenderMode_LCD,    // Subpixel rendering for horizontally decimated LCD displays
	R_FontRenderMode_LCD_V,  // Subpixel rendering for vertically decimated LCD displays
};

#define R_USE_SUBPIXEL_RENDERING 1

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
};

typedef struct R_GlyphIndexNode R_GlyphIndexNode;
struct R_GlyphIndexNode
{
	R_GlyphIndexNode *next;
	U32 codepoint;
	U32 index; // NOTE(hampus): The index into the glyphs array in the font
};

typedef struct R_GlyphBucket R_GlyphBucket;
struct R_GlyphBucket
{
	R_GlyphIndexNode *first;
	R_GlyphIndexNode *last;
};

#define GLYPH_BUCKETS_ARRAY_SIZE 128
#define R_FONT_CACHE_SIZE        8

typedef struct R_KerningPair R_KerningPair;
struct R_KerningPair
{
	F32 value;
};

typedef struct R_Font R_Font;
struct R_Font
{
	// TODO(hampus): Remove the arena from here and
	// try to allocate from the renderer arena
	Arena *arena;

	R_GlyphBucket glyph_bucket[GLYPH_BUCKETS_ARRAY_SIZE];
	R_Glyph *glyphs;
	R_KerningPair *kerning_pairs;

	U32 num_font_atlas_regions;
	R_FontAtlasRegion *font_atlas_regions;
	F32 max_ascent;
	F32 max_descent;

	F32 line_height;        // NOTE(hampus): How much vertical spaces a line occupy
	F32 max_advance_width;
	F32 underline_position; // NOTE(hampus): Relative to the baseline
	B32 has_kerning;
	U32 num_glyphs;
	U32 num_loaded_glyphs;

	U32 font_size;

	// TODO(hampus): Collapse these somehow
	Str8 family_name;
	Str8 style_name;
	Str8 path;

	R_FontRenderMode render_mode;

	U64 last_frame_index_used;

	B32 loaded;
	B32 loading;
};

typedef struct R_FontCache R_FontCache;
struct R_FontCache
{
	U32 entries_used;
	R_Font entries[R_FONT_CACHE_SIZE];
};

typedef struct R_FontKey R_FontKey;
struct R_FontKey
{
	U32 font_size;
	Str8 path;
};

typedef struct R_FontQueueEntry R_FontQueueEntry;
struct R_FontQueueEntry
{
	R_FontRenderMode render_mode;
	R_Font *font;
};

#define FONT_QUEUE_SIZE (1 << 6)
#define FONT_QUEUE_MASK (FONT_QUEUE_SIZE - 1)

typedef struct R_FontQueue R_FontQueue;
struct R_FontQueue
{
	R_FontQueueEntry *queue;
	U32 volatile queue_write_index;
	U32 volatile queue_read_index;
};

typedef struct R_DirtyFontRegionQueue R_DirtyFontRegionQueue;
struct R_DirtyFontRegionQueue
{
	RectU32 *queue;
	U32 volatile queue_write_index;
	U32 volatile queue_read_index;
};

internal Void render_font_loader_thread(Void *data);

internal R_FontAtlas *render_make_font_atlas(R_Context *renderer, Vec2U32 dim);

internal Void    render_init_font(R_Context *renderer, R_Font *out_font, S32 font_size, Str8 path, R_FontRenderMode render_mode);
internal Void    render_destroy_font(R_Context *renderer, R_Font *font);

internal Void render_text(R_Context *renderer, Vec2F32 min, Str8 text, R_FontKey font_key, Vec4F32 color);
internal Vec2F32 render_measure_text(R_Font *font, Str8 text);

#endif