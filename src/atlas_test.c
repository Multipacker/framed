#include "base/base_inc.h"
#include "os/os_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"

typedef struct TestNode TestNode;
struct TestNode
{
	TestNode *next;
	TestNode *prev;
	S32 x;
};

internal S32
os_main(Str8List arguments)
{
	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	R_Context *renderer = render_init(&gfx);

    Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	gfx_show_window(&gfx);
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

				default:
				{
				} break;
			}
		}

		render_begin(renderer);

#if 1
		for (R_FontAtlasRegionNode *node = renderer->font_atlas->first_free_region;
				 node != 0;
				 node = node->next_free)
		{
			RectU32 rect = node->region;
			render_rect(renderer, v2f32_from_v2u32(rect.min), v2f32_from_v2u32(rect.max), .border_thickness = 1.0f, .color = v4f32(1.0f, 0, 0, 1.0f));
		}
		R_TextureSlice atlas_slice = render_slice_from_texture(renderer->font_atlas->texture, rectf32(v2f32(0, 0), v2f32(1, 1)));
		render_rect(renderer, v2f32(0, 0), v2f32(2048, 2048), .slice = atlas_slice, .is_subpixel_text = true);
#endif

        R_FontKey key0 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 10);
        R_FontKey key1 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 11);
        R_FontKey key2 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 12);
        R_FontKey key3 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 13);
        R_FontKey key4 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 14);
        R_FontKey key5 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 15);
        R_FontKey key6 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 16);
        R_FontKey key7 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 17);
        R_FontKey key8 = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 18);

		render_text(renderer, v2f32(100, 100), str8_lit("Hello, world!"), key0, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 120), str8_lit("Hello, world!"), key1, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 140), str8_lit("Hello, world!"), key2, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 160), str8_lit("Hello, world!"), key3, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 180), str8_lit("Hello, world!"), key4, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 200), str8_lit("Hello, world!"), key5, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 220), str8_lit("Hello, world!"), key6, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 240), str8_lit("Hello, world!"), key7, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(100, 260), str8_lit("Hello, world!"), key8, v4f32(1, 1, 1, 1));

        R_Font *font = render_make_font(renderer, 20, str8_lit("data/fonts/liberation-mono.ttf"));
        render_destroy_font(renderer, font);

        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}
	return(0);
}
