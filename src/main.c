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

    Renderer *renderer = render_init(&gfx);

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

        render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
	}

	return(0);
}
