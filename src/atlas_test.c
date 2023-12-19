#include "base/base_inc.h"
#include "os/os_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"

typedef struct AtlasRegionNode AtlasRegionNode;
struct AtlasRegionNode
{
    AtlasRegionNode *next_free;
    AtlasRegionNode *prev_free;

    AtlasRegionNode *parent;
    AtlasRegionNode *children[Corner_COUNT];
    RectF32 region;

    // NOTE(hampus): This will be true if either this
    // node is used or one of it descendants are used
    B32 used;
};

typedef struct AtlasRegion AtlasRegion;
struct AtlasRegion
{
    AtlasRegionNode *node;
    RectF32 region;
};

typedef struct Atlas Atlas;
struct Atlas
{
    AtlasRegionNode *first_free_region;
    AtlasRegionNode *last_free_region;
    Vec2U32 dim;
    U64 num_free_regions;
};

internal void
render_push_free_region_to_atlas(Atlas *atlas, AtlasRegionNode *node)
{
    dll_push_back_np(atlas->first_free_region, atlas->last_free_region, node, next_free, prev_free);
    node->used = false;
    atlas->num_free_regions++;
}

internal void
render_remove_free_region_from_atlas(Atlas *atlas, AtlasRegionNode *node)
{
    dll_remove_npz(atlas->first_free_region, atlas->last_free_region, node, next_free, prev_free, check_null, set_null);
    node->used = true;
    atlas->num_free_regions--;
}

internal Atlas
render_make_atlas(Arena *arena, Vec2U32 dim)
{
    Atlas result = {0};
    result.dim = dim;
    AtlasRegionNode *first_free_region = push_struct(arena, AtlasRegionNode);
    first_free_region->region.min = v2f32(0, 0);
    first_free_region->region.max = v2f32((F32)dim.x, (F32)dim.x);
    render_push_free_region_to_atlas(&result, first_free_region);
    return(result);
}

internal AtlasRegion
render_alloc_atlas_region(Arena *arena, Atlas *atlas, Vec2U32 dim)
{
    AtlasRegionNode *first_free_region = atlas->first_free_region;
    // NOTE(hampus): Each region will always be the same size in
    // x and y, so we only need to check the width
        AtlasRegionNode *node = first_free_region;
    F32 required_size = (F32)u32_max(dim.x, dim.y);
    F32 region_size = node->region.max.x - node->region.min.x;
    B32 can_halve_size = region_size >= (required_size*2);
        while (can_halve_size)
    {
        // NOTE(hampus): Remove the current node, it will no longer
        // be free to take because one of its descendants will
        // be taken.
        render_remove_free_region_from_atlas(atlas, node);

        // NOTE(hampus): Allocate 4 children to replace
        // the parent

        AtlasRegionNode *children = push_array(arena, AtlasRegionNode, Corner_COUNT);

        {
            Vec2F32 bbox[Corner_COUNT] =
            {
                bbox[Corner_TopLeft] = node->region.min,
                bbox[Corner_TopRight] = v2f32(node->region.max.x, node->region.min.y),
                bbox[Corner_BottomLeft] = v2f32(node->region.min.x, node->region.max.y),
                bbox[Corner_BottomRight] = node->region.max,
            };

            Vec2F32 middle = v2f32_div_f32(v2f32_sub_v2f32(bbox[Corner_BottomRight], bbox[Corner_TopLeft]), 2.0f);

            children[Corner_TopLeft].region     = rectf32(bbox[Corner_TopLeft], middle);

            children[Corner_TopRight].region.min = v2f32(middle.x, bbox[Corner_TopRight].y);
            children[Corner_TopRight].region.max = v2f32(bbox[Corner_TopRight].x, middle.y);

            children[Corner_BottomLeft].region.min = v2f32(bbox[Corner_TopLeft].x, middle.y);
            children[Corner_BottomLeft].region.max = v2f32(middle.x, bbox[Corner_BottomRight].y);

            children[Corner_BottomRight].region = rectf32(middle, bbox[Corner_BottomRight]);
        }

        // NOTE(hampus): Push back the new children to
        // the free list and link them into the quad-tree.
        for (U64 i = 0; i < Corner_COUNT; ++i)
        {
            children[i].parent = node;
            node->children[i] = &children[i];
            render_push_free_region_to_atlas(atlas, children + i);
        }

        node = &children[0];

        region_size = node->region.max.x - node->region.min.x;
        can_halve_size = region_size >= (required_size*2);
    }

    render_remove_free_region_from_atlas(atlas, node);
    node->next_free = 0;
    node->prev_free = 0;
    AtlasRegion result = {node, node->region};
    return(result);
}

internal void
render_free_atlas_region(Atlas *atlas, AtlasRegion region)
{
    AtlasRegionNode *node = region.node;
    AtlasRegionNode *parent = node->parent;
    render_push_free_region_to_atlas(atlas, node);
    if (parent)
    {
        node->used = false;
        // NOTE(hampus): Lets check if we can combine
        // any empty nodes to larger nodes. Otherwise
        // we just put it back into the free list
        B32 used = false;
        for (U64 i = 0; i < Corner_COUNT; ++i)
        {
            AtlasRegionNode *child = parent->children[i];
            used = child->used;
        }

        if (!used)
        {
            // NOTE(hampus): All the siblings are marked as empty.
            // Remove the children from the free list and push
            // their parent.
            for (U64 i = 0; i < Corner_COUNT; ++i)
            {
                render_remove_free_region_from_atlas(atlas, parent->children[i]);
            }
            render_free_atlas_region(atlas, (AtlasRegion){parent, parent->region});
        }
    }
}

internal S32
os_main(Str8List arguments)
{
	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));
	gfx_show_window(&gfx);

    R_Context *renderer = render_init(&gfx);

	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

    Arena *perm_arena = arena_create();

    Atlas atlas = render_make_atlas(perm_arena, v2u32(128, 128));

    // NOTE(hampus): This should first create 4 regions with 64x64, leaving the atlas empty.
    // When freeing them, the atlas should only have 1 128x128 node in its free list.
    AtlasRegion region0 = render_alloc_atlas_region(perm_arena, &atlas, v2u32(64, 64));
    AtlasRegion region1 = render_alloc_atlas_region(perm_arena, &atlas, v2u32(64, 64));
    AtlasRegion region2 = render_alloc_atlas_region(perm_arena, &atlas, v2u32(64, 64));
    AtlasRegion region3 = render_alloc_atlas_region(perm_arena, &atlas, v2u32(64, 64));
    render_free_atlas_region(&atlas, region0);
    render_free_atlas_region(&atlas, region1);
    render_free_atlas_region(&atlas, region2);
    render_free_atlas_region(&atlas, region3);

    B32 running = true;
	while (running)
	{
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		Gfx_EventList events = gfx_get_events(current_arena, &gfx);
		for (Gfx_Event *event = events.first;
             event != 0;
             event = event->next)
		{
			switch (event->kind)
			{
				case Gfx_EventKind_Quit:
				{
                    running = false;
				} break;

				case Gfx_EventKind_KeyPress:
				{
                    if (event->key == Gfx_Key_F11)
                    {
                        gfx_toggle_fullscreen(&gfx);
                    }
				} break;

                invalid_case;
			}
		}

        render_begin(renderer);

        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}
