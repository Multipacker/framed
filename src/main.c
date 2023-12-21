#include "base/base_inc.h".
#include "os/os_inc.h"
#include "logging/logging_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "logging/logging_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"

// rate = (1 + ε) - 2^(log2(ε) * (dt / animation_duration))

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	gfx_show_window(&gfx);

	R_Context *renderer = render_init(&gfx);

	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	R_TextureSlice texture = render_create_texture_slice(renderer, str8_lit("data/test.png"), R_ColorSpace_sRGB);

	Arena *perm_arena = arena_create();

	R_Font *font  = render_make_font(renderer, 10, str8_lit("data/fonts/arial.ttf"));

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
					log_info("Gfx_EventKind_KeyPress: %d", event->key);
					if (event->key == Gfx_Key_F11)
					{
						gfx_toggle_fullscreen(&gfx);
					}
				} break;

				case Gfx_EventKind_KeyRelease:
				{
					log_info("Gfx_EventKind_KeyRelease: %d", event->key);
				} break;

				case Gfx_EventKind_Char:
				{
					log_info("Gfx_EventKind_Char");
				} break;

				case Gfx_EventKind_Scroll:
				{
					log_info("Gfx_EventKind_Scroll: %d", (U32) event->scroll.y);
				} break;

				case Gfx_EventKind_Resize:
				{
					log_info("Gfx_EventKind_Resize");
				} break;

				invalid_case;
			}
		}

		render_begin(renderer);

		Vec2U32 screen_area = gfx_get_window_client_area(&gfx);
		render_rect(renderer, v2f32(0, 0), v2f32((F32) screen_area.width, (F32) screen_area.height), .color = v4f32(0.25, 0.25, 0.25, 1.0));

		Vec2F32 mouse = gfx_get_mouse_pos(&gfx);

		render_rect(renderer, v2f32(300, 300), v2f32(600, 370), .border_thickness = 1, .radius = 10, .softness = 1, .color = v4f32(0.5f, 0.5f, 0.5f, 1.0f));
		render_text(renderer, v2f32(300, 300), str8_lit("Hello, world!"), font, v4f32(1, 1, 1, 1));
		render_text(renderer, v2f32(400, 400), str8_lit("123456789_[]()"), font, v4f32(1, 0, 0, 0.5));

		render_rect(renderer, v2f32(64, 64), v2f32(128, 128), .slice = texture, .radius = 10, .softness = 1);

		R_RenderStats stats = render_get_stats(renderer);
#if 0
		log_info("Stats:");
		log_info("\tRect count:  %"PRIU64, stats.rect_count);
		log_info("\tBatch count: %"PRIU64, stats.batch_count);
#endif
		Log_EntryList log_entries = log_get_entries(current_arena);

		local Vec2F32 log_size = { 0 };
		render_rect(renderer, v2f32(0, 0), log_size, .color = vec4f32_srgb_to_linear(v4f32(0.5, 0.5, 0.5, 1.0)));

		F32 y_offset = 20;
		for (Log_Entry *entry = log_entries.first; entry; entry = entry->next)
		{
			render_text(renderer, v2f32(0, y_offset), entry->message, font, v4f32(1, 1, 1, 1));
			Vec2F32 size = render_measure_text(font, entry->message);
			y_offset += 20;
			log_size.width = f32_max(log_size.width, size.width);
		}
		log_size.height = y_offset;

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	log_flush();

	return(0);
}
