#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "image/image_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "image/image_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"
#include "log_ui.c"
#include "texture_ui.c"

// rate = (1 + ε) - 2^(log2(ε) * (dt / animation_duration))

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	Render_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Arena *arena = arena_create();
	Str8 path = str8_lit("data/test.png");
	B32 loaded_image = false;
	Render_TextureSlice image_texture = { 0 };

	Str8 image_contents = { 0 };
	if (os_file_read(arena, path, &image_contents))
	{
		loaded_image = image_load(arena, renderer, image_contents, &image_texture);
	}
	else
	{
		log_error("Could not load file '%"PRISTR8"'", str8_expand(path));
	}

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	gfx_show_window(&gfx);
	B32 running = true;
	B32 show_log = false;
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
					else if (event->key == Gfx_Key_F1)
					{
						show_log = !show_log;
					}
				} break;

				default:
				{
				} break;
			}
		}

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);
		
		Render_FontKey font = render_key_from_font(str8_lit("data/fonts/Inter-Regular.ttf"), 7);
		ui_push_font(font);
		
		ui_log_keep_alive(current_arena);
		if (show_log)
		{
			ui_next_width(ui_fill());
			ui_next_height(ui_pct(0.25, 1));
			ui_logger();
		}

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_texture_view(image_texture);

		ui_end();

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;
	}

	return(0);
}
