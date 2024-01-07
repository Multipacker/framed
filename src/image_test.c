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

// rate = (1 + ε) - 2^(log2(ε) * (dt / animation_duration))

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	R_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Arena *arena = arena_create();
	Str8 path = str8_lit("data/mid_test.png");
	B32 loaded_image = false;
	R_TextureSlice image_texture = { 0 };

	Str8 image_contents = { 0 };
	if (os_file_read(arena, path, &image_contents))
	{
		loaded_image = image_load(arena, renderer, image_contents, &image_texture);
	}
	else
	{
		log_error("Could not load file '%"PRISTR8"'", path);
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
		
		R_FontKey font = render_key_from_font(str8_lit("data/fonts/Inter-Regular.ttf"), 7);
		ui_push_font(font);
		
		ui_log_keep_alive(current_arena);
		if (show_log)
		{
			ui_next_width(ui_fill());
			ui_next_height(ui_pct(0.25, 1));
			ui_logger();
		}

		{
			ui_next_width(ui_fill());
			ui_next_height(ui_fill());
			ui_next_color(v4f32(0.5, 0.5, 0.5, 1));
			UI_Box *atlas_parent = ui_box_make(UI_BoxFlag_DrawBackground |
											   UI_BoxFlag_Clip |
											   UI_BoxFlag_Clickable |
											   UI_BoxFlag_ViewScroll |
											   UI_BoxFlag_AnimateDim,
											   str8_lit("FontParent")
											   );
			ui_parent(atlas_parent)
			{
				local Vec2F32 offset = { 0 };
				local F32 scale = 1;

				ui_next_relative_pos(Axis2_X, offset.x);
				ui_next_relative_pos(Axis2_Y, offset.y);
				ui_next_width(ui_pct(1.0f / scale, 1));
				ui_next_height(ui_pct(1.0f / scale, 1));

				ui_next_slice(image_texture);
				ui_next_color(v4f32(1, 1, 1, 1));

				ui_next_corner_radius(0);
				UI_Box *atlas_box = ui_box_make(UI_BoxFlag_FloatingPos |
												UI_BoxFlag_DrawBackground,
												str8_lit("FontAtlas")
												);

				UI_Comm atlas_comm = ui_comm_from_box(atlas_parent);

				F32 old_scale = scale;
				scale *= f32_pow(2, atlas_comm.scroll.y * 0.1f);

				// TODO(simon): Slightly broken math, but mostly works.
				Vec2F32 scale_offset = v2f32_mul_f32(v2f32_sub_v2f32(atlas_comm.rel_mouse, offset), scale / old_scale - 1.0f);
				offset = v2f32_add_v2f32(v2f32_sub_v2f32(offset, atlas_comm.drag_delta), scale_offset);
			}
		}

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
