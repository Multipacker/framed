#ifndef RENDER_FONT_H
#define RENDER_FONT_H

typedef enum Render_FontRenderMode Render_FontRenderMode;
enum Render_FontRenderMode
{
	Render_FontRenderMode_Normal, // Default render mode. 8-bit AA bitmaps
	Render_FontRenderMode_LCD,    // Subpixel rendering for horizontally decimated LCD displays
	Render_FontRenderMode_LCD_V,  // Subpixel rendering for vertically decimated LCD displays
	
	Render_FontRenderMode_COUNT,
};

#define RENDER_USE_SUBPIXEL_RENDERING 1

typedef struct Render_FontAtlasRegionNode Render_FontAtlasRegionNode;
struct Render_FontAtlasRegionNode
{
	Render_FontAtlasRegionNode *next_free;
	Render_FontAtlasRegionNode *prev_free;

	Render_FontAtlasRegionNode *parent;
	Render_FontAtlasRegionNode *children[Corner_COUNT];
	RectU32 region;

	// NOTE(hampus): This will be true if either this
	// node is used or one of it descendants are used
	// TODO(hampus): Test performance when this is
	// a bitmask instead
	B32 used;
};

typedef struct Render_FontAtlasRegion Render_FontAtlasRegion;
struct Render_FontAtlasRegion
{
	Render_FontAtlasRegionNode *node;
	RectU32 region;
};

typedef struct Render_FontAtlas Render_FontAtlas;
struct Render_FontAtlas
{
	Render_FontAtlasRegionNode *first_free_region;
	Render_FontAtlasRegionNode *last_free_region;
	Vec2U32 dim;
	Void *memory;
	U64 num_free_regions;
	Render_Texture texture;
};

typedef struct Render_Glyph Render_Glyph;
struct Render_Glyph
{
	Vec2F32 size_in_pixels;
	Vec2F32 bearing_in_pixels;
	F32 advance_width;
	Render_TextureSlice slice;
};

typedef struct Render_GlyphIndexNode Render_GlyphIndexNode;
struct Render_GlyphIndexNode
{
	Render_GlyphIndexNode *next;
	U32 codepoint;
	U32 index; // NOTE(hampus): The index into the glyphs array in the font
};

typedef struct Render_GlyphBucket Render_GlyphBucket;
struct Render_GlyphBucket
{
	Render_GlyphIndexNode *first;
	Render_GlyphIndexNode *last;
};

#define GLYPH_BUCKETS_ARRAY_SIZE 128
#define RENDER_FONT_CACHE_SIZE   8

typedef struct Render_KerningPair Render_KerningPair;
struct Render_KerningPair
{
	U64 pair;
	F32 value;
};

typedef enum Render_FontState Render_FontState;
enum Render_FontState 
{
	Render_FontState_Unloaded,
	
	Render_FontState_InQueue,
	Render_FontState_Loading,
	Render_FontState_Loaded,
};

typedef struct Render_FontLoadParams Render_FontLoadParams;
struct Render_FontLoadParams
{
	Render_FontRenderMode render_mode;
	U32              size;
	Str8             path;
};

typedef struct Render_Font Render_Font;
struct Render_Font
{
	// TODO(hampus): Remove the arena from here and
	// try to allocate from the renderer arena
	Arena *arena;

	Render_GlyphBucket glyph_bucket[GLYPH_BUCKETS_ARRAY_SIZE];
	Render_Glyph *glyphs;

	U64 kern_map_size;
	Render_KerningPair *kern_pairs;

	U32 num_font_atlas_regions;
	Render_FontAtlasRegion *font_atlas_regions;
	F32 max_ascent;
	F32 max_descent;

	F32 line_height;        // NOTE(hampus): How much vertical spaces a line occupy
	F32 max_advance_width;
	F32 underline_position; // NOTE(hampus): Relative to the baseline
	B32 has_kerning;
	U32 num_glyphs;
	U32 num_loaded_glyphs;
	
	Str8 family_name;
	Str8 style_name;

	U64 last_frame_index_used;
	
	Render_FontState state;
	Render_FontLoadParams load_params;
};

typedef struct Render_FontCache Render_FontCache;
struct Render_FontCache
{
	U32 entries_used;
	Render_Font entries[RENDER_FONT_CACHE_SIZE];
};

typedef struct Render_FontKey Render_FontKey;
struct Render_FontKey
{
	U32 font_size;
	Str8 path;
};

typedef struct Render_FontQueueEntry Render_FontQueueEntry;
struct Render_FontQueueEntry
{
	Render_FontLoadParams params;
	Render_Font *font;
};

#define FONT_QUEUE_SIZE (1 << 6)
#define FONT_QUEUE_MASK (FONT_QUEUE_SIZE - 1)

typedef struct Render_FontQueue Render_FontQueue;
struct Render_FontQueue
{
	Render_FontQueueEntry *queue;
	U32 volatile write_index;
	U32 volatile read_index;
	OS_Semaphore semaphore;
};

internal Render_FontAtlas      *render_make_font_atlas(Render_Context *renderer, Vec2U32 dim);
internal Void                   render_push_free_region_to_atlas(Render_FontAtlas *atlas, Render_FontAtlasRegionNode *node);
internal Void                   render_remove_free_region_from_atlas(Render_FontAtlas *atlas, Render_FontAtlasRegionNode *node);
internal Render_FontAtlasRegion render_alloc_font_atlas_region(Render_Context *renderer, Render_FontAtlas *atlas, Vec2U32 dim);
internal Void                   render_free_atlas_region(Render_FontAtlas *atlas, Render_FontAtlasRegion region);

internal Void render_font_stream_thread(Void *data);

#endif
