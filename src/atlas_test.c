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

    R_FontKey font = render_key_from_font(str8_lit("data/fonts/segoeuib.ttf"), 25);

    U32 frame_index = 0;

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

#if 0
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

        Str8 string = str8_lit("Hello, world!\nHello, world!");

        Vec2F32 dim = render_measure_multiline_text(render_font_from_key(renderer, font), string);
        render_rect(renderer, v2f32(100, 100), v2f32_add_v2f32(v2f32(100, 100), dim));

        render_multiline_text(renderer, v2f32(100, 100), string, font, v4f32(1, 0, 0, 1));
        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

        ++frame_index;
	}
	return(0);
}
