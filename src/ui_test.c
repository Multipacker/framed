#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	R_Context *renderer = render_init(&gfx);

	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	R_FontKey font = render_key_from_font(str8_lit("data/fonts/segoeuib.ttf"), 16);
	R_FontKey icon_font = render_key_from_font(str8_lit("data/fonts/fontello.ttf"), 16);

	UI_Context *ui = ui_init();

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

		ui_begin(ui, &events, renderer, font);

		ui_next_size(Axis2_X, ui_pixels(50, 1));
		ui_next_size(Axis2_Y, ui_pixels(50, 1));
		ui_box_make(UI_BoxFlag_DrawBackground |
								  UI_BoxFlag_DrawBorder,
								  str8_lit("Box"));

		ui_next_size(Axis2_X, ui_pixels(100, 1));
		ui_next_size(Axis2_Y, ui_pixels(100, 1));
		ui_box_make(UI_BoxFlag_DrawBackground |
								  UI_BoxFlag_DrawBorder,
								   str8_lit("Box2"));

		ui_next_size(Axis2_X, ui_pixels(300, 1));
		ui_next_size(Axis2_Y, ui_pixels(300, 1));
		UI_Box *parent = ui_box_make(UI_BoxFlag_DrawBackground |
									 UI_BoxFlag_DrawBorder,
									 str8_lit("Parent"));
		#if 0

		ui_push_parent(parent);



		ui_pop_parent();
		#endif

		ui_end();

		render_end(renderer);

		log_update_entries(1000);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}