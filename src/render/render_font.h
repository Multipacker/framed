#ifndef RENDER_FONT_H
#define RENDER_FONT_H

typedef struct R_FontLoaderThreadData R_FontLoaderThreadData;
struct R_FontLoaderThreadData
{
    U32 id;
    Str8 name;
};

typedef enum R_FontRenderMode R_FontRenderMode;
enum R_FontRenderMode
{
    R_FontRenderMode_Normal, // Default render mode. 8-bit AA bitmaps
    R_FontRenderMode_LCD,    // Subpixel rendering for horizontally decimated LCD displays
    R_FontRenderMode_LCD_V,  // Subpixel rendering for vertically decimated LCD displays

    R_FontRenderMode_COUNT,
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

#define R_FONT_CACHE_SIZE   8

typedef struct R_KerningPair R_KerningPair;
struct R_KerningPair
{
    U64 pair;
    F32 value;
};

typedef enum R_FontState R_FontState;
enum R_FontState
{
    R_FontState_Unloaded,

    R_FontState_InQueue,
    R_FontState_Loading,
    R_FontState_Loaded,
};

typedef struct R_FontLoadParams R_FontLoadParams;
struct R_FontLoadParams
{
    R_FontRenderMode render_mode;
    U32              size;
    Str8             path;
};

typedef struct R_CodepointMap R_CodepointMap;
struct R_CodepointMap
{
    U32 codepoint;
    U32 glyph_index;
};

typedef struct R_Font R_Font;
struct R_Font
{
    Arena *arena;

    R_CodepointMap *codepoint_map;
    U32 codepoint_map_size; // NOTE(simon): Must be a power of 2.

    R_Glyph *glyphs;

    U64 kern_map_size;
    R_KerningPair *kern_pairs;

    U32 num_font_atlas_regions;
    R_FontAtlasRegion *font_atlas_regions;
    F32 max_ascent;
    F32 max_descent;

    F32 line_height;        // NOTE(hampus): How much vertical spaces a line occupy
    U32 num_glyphs;

    U64 last_frame_index_used;

    R_FontState state;
    R_FontLoadParams load_params;
};

typedef struct R_FontCache R_FontCache;
struct R_FontCache
{
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
    R_FontLoadParams params;
    R_Font *font;
};

#define FONT_QUEUE_SIZE (1 << 6)
#define FONT_QUEUE_MASK (FONT_QUEUE_SIZE - 1)

typedef struct R_FontQueue R_FontQueue;
struct R_FontQueue
{
    R_FontQueueEntry *queue;
    U32 volatile write_index;
    U32 volatile read_index;
    OS_Semaphore semaphore;
};

typedef struct R_FontContext R_FontContext;
struct R_FontContext
{
    Arena *permanent_arena;

    R_FontAtlas *font_atlas;
    R_FontCache *font_cache;
    R_FontQueue *font_queue;
    OS_Mutex font_atlas_mutex;

    U64 frame_index;
};

global R_FontContext r_font_context;

internal Void r_font_init(Void);
internal Void r_font_end_frame(Void);

internal R_FontAtlas      *r_make_font_atlas(Vec2U32 dim);
internal Void              r_push_free_region_to_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node);
internal Void              r_remove_free_region_from_atlas(R_FontAtlas *atlas, R_FontAtlasRegionNode *node);
internal R_FontAtlasRegion r_alloc_font_atlas_region(R_FontAtlas *atlas, Vec2U32 dim);
internal Void              r_free_atlas_region(R_FontAtlas *atlas, R_FontAtlasRegion region);

internal R_Font *r_font_from_key(R_FontKey font_key);
internal B32 r_font_is_loaded(R_Font *font);

internal Void r_character_internal(Vec2F32 min, U32 codepoint, R_Font *font, Vec4F32 color);
internal Void r_text_internal(Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color);

internal Vec2F32 r_measure_character(R_Font *font, U32 codepoint);
internal Vec2F32 r_measure_text(R_Font *font, Str8 text);

internal Void r_font_stream_thread(Void *data);

#endif
