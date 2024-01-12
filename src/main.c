#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"

// rate = (1 + ε) - 2^(log2(ε) * (dt / animation_duration))

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	gfx_show_window(&gfx);

	Render_Context *renderer = render_init(&gfx);

	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Render_TextureSlice texture = render_create_texture_slice(renderer, str8_lit("data/test.png"), Render_ColorSpace_sRGB);

	Arena *perm_arena = arena_create();

	Render_FontKey font = render_key_from_font(str8_lit("data/fonts/Inter-Regular.ttf"), 16);
	B32 show_log = false;
	F32 log_offset = 0;

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
					else if (event->key == Gfx_Key_F1)
					{
						show_log = !show_log;
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
					//log_info("Gfx_EventKind_Scroll: %d", (U32) event->scroll.y);
					log_offset = f32_max(0, log_offset + 10 * event->scroll.y);
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

		if (show_log)
		{
			Vec2U32 client_area = gfx_get_window_client_area(&gfx);

			Vec2F32 log_pos  = v2f32(0.0, 0.0);
			Vec2F32 log_size = v2f32((F32) client_area.width, 300.0);

			render_rect(renderer, log_pos, v2f32_add_v2f32(log_pos, log_size), .color = vec4f32_srgb_to_linear(v4f32(0.5, 0.5, 0.5, 1.0)));
			render_push_clip(renderer, log_pos, v2f32_add_v2f32(log_pos, log_size), false);
			render_rect(renderer, v2f32(0, 0), log_size, .color = vec4f32_srgb_to_linear(v4f32(0.5, 0.5, 0.5, 1.0)));

			Render_Font *real_font = render_font_from_key(renderer, font);
			F32 y_offset = log_size.height - real_font->line_height + log_offset;

			U32 entry_count = 0;
			Log_QueueEntry *entries = log_get_entries(&entry_count);
			for (S32 i = (S32) entry_count - 1; i >= 0; --i)
			{
				Str8 message = str8_cstr((CStr) entries[i].message);
				--message.size; // NOTE(simon): Remove the newline.

				render_text(renderer, v2f32_add_v2f32(log_pos, v2f32(0, y_offset)), message, font, v4f32(1, 1, 1, 1));
				Vec2F32 size = render_measure_text(real_font, message);
				y_offset -= size.height;
			}

			render_pop_clip(renderer);
		}

		log_update_entries(1000);

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}
