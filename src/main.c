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

#if !BUILD_MODE_RELEASE
#    define LOG(...) printf(__VA_ARGS__)
#else
#    define LOG(...)
#endif

// rate = (1 + ε) - 2^(log2(ε) * (dt / animation_duration))

internal S32
os_main(Str8List arguments)
{
#if 0
	log_init(str8_lit(""));

	U32 test = 32;
	log_info("Test: %u", test);

	log_flush();
	return 0;
#endif

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
					LOG("Gfx_EventKind_KeyPress: %d\n", event->key);
					if (event->key == Gfx_Key_F11)
					{
						gfx_toggle_fullscreen(&gfx);
					}
				} break;

				case Gfx_EventKind_KeyRelease:
				{
					LOG("Gfx_EventKind_KeyRelease: %d\n", event->key);
				} break;

				case Gfx_EventKind_Char:
				{
					LOG("Gfx_EventKind_Char\n");
				} break;

				case Gfx_EventKind_Scroll:
				{
					LOG("Gfx_EventKind_Scroll: %d\n", (U32) event->scroll.y);
				} break;

				case Gfx_EventKind_Resize:
				{
					LOG("Gfx_EventKind_Resize\n");
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

		render_push_clip(renderer, v2f32(0, 0), v2f32(150, 150), false);
		// NOTE(simon): Giving true here should only allow a 50x50 rectangle to
		// show, false should give a 100x100.
		render_rect(renderer, v2f32(10, 10), v2f32(20, 20), .softness = 1);

		render_rect(renderer, v2f32(64, 64), v2f32(128, 128), .slice = texture, .radius = 10, .softness = 1);

		render_push_clip(renderer, v2f32(50, 50), v2f32(150, 150), false);

		render_rect(renderer, v2f32_sub_f32(mouse, 30.0f), v2f32_add_f32(mouse, 30.0f));

		R_RenderStats stats = render_get_stats(renderer);
#if 0
		LOG("Stats:\n");
		LOG("\tRect count:  %"PRIU64"\n", stats.rect_count);
		LOG("\tBatch count: %"PRIU64"\n", stats.batch_count);
#endif
		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}
