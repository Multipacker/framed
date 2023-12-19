#include "base/base_inc.h"
#include "os/os_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"

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

    renderer->font_atlas = render_make_atlas(perm_arena, v2u32(1024, 1024));

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

        for (R_FontAtlasRegionNode *node = renderer->font_atlas->first_free_region;
             node != 0;
             node = node->next_free)
        {
            RectF32 rect = node->region;
            render_rect(renderer, rect.min, rect.max, .border_thickness = 1.0f, .color = v4f32(1.0f, 0, 0, 1.0f));
        }

        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}
	return(0);
}
