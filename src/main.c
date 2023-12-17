#include "base/base_inc.h"
#include "os/os_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
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
	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));
	gfx_show_window(&gfx);

    R_Context *renderer = render_init(&gfx);

	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

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
                    LOG("Gfx_EventKind_Scroll: %d\n", (U32)event->scroll.y);
				} break;

				case Gfx_EventKind_Resize:
				{
                    LOG("Gfx_EventKind_Resize\n");
				} break;

                invalid_case;
			}
		}

        render_begin(renderer);

		Vec2F32 mouse = gfx_get_mouse_pos(&gfx);

		render_push_clip(renderer, v2f32(0, 0), v2f32(150, 150), false);
		// NOTE(simon): Giving true here should only allow a 50x50 rectangle to
		// show, false should give a 100x100.
		render_rect(renderer, v2f32(10, 10), v2f32(20, 20));

		render_push_clip(renderer, v2f32(50, 50), v2f32(150, 150), false);

		render_rect(renderer, v2f32_sub_f32(mouse, 30.0f), v2f32_add_f32(mouse, 30.0f));

		R_RenderStats stats = render_get_stats(renderer);
		printf("Stats:\n");
		printf("\tRect count:  %lu\n", stats.rect_count);
		printf("\tBatch count: %lu\n", stats.batch_count);

        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}
