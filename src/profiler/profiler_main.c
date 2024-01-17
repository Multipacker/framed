#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "image/image_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "image/image_inc.c"
#include "ui/ui_inc.c"

#include "profiler/profiler_log_ui.c"
#include "profiler/profiler_texture_ui.c"

////////////////////////////////
//~ hampus: Tab views

UI_TAB_VIEW(ui_tab_view_logger)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_logger();
}

UI_TAB_VIEW(ui_tab_view_texture_viewer)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	Render_TextureSlice texture = *(Render_TextureSlice *) data;
	ui_texture_view(texture);
}

////////////////////////////////
//~ hampus: Main

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Arena *perm_arena = arena_create();
	app_state = push_struct(perm_arena, AppState);
	app_state->perm_arena = perm_arena;

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	Render_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Render_TextureSlice image_texture = render_create_texture_slice(renderer, str8_lit("data/image.png"));

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	app_state->cmd_buffer.buffer = push_array(app_state->perm_arena, UI_Cmd, CMD_BUFFER_SIZE);
	app_state->cmd_buffer.size = CMD_BUFFER_SIZE;

	//- hampus: Build startup UI

	{
		UI_Window *master_window = ui_window_make(app_state->perm_arena, v2f32(1.0f, 1.0f));

		UI_Panel *first_panel = master_window->root_panel;

		UI_SplitPanelResult split_panel_result = ui_builder_split_panel(first_panel, Axis2_X);
		{
			{
				UI_TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, ui_tab_view_logger, 0, str8_lit("Log")),
					.panel = split_panel_result.panels[Side_Min],
				};
				ui_command_tab_attach(&attach);
			}

			{
				UI_TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, ui_tab_view_texture_viewer, &image_texture, str8_lit("Texture Viewer")),
					.panel = split_panel_result.panels[Side_Max],
				};
				ui_command_tab_attach(&attach);
			}
			{
				UI_TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, 0, 0, str8_lit("")),
					.panel = split_panel_result.panels[Side_Max],
				};
				ui_command_tab_attach(&attach);
			}
		}

		app_state->master_window = master_window;
		app_state->next_focused_panel = first_panel;
	}

	app_state->frame_index = 1;

	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		Gfx_EventList events = gfx_get_events(current_arena, &gfx);
		for (Gfx_Event *event = events.first;
			 event != 0;
			 event = event->next)
		{
			if (event->kind == Gfx_EventKind_Quit)
			{
				running = false;
			}
			else if (event->kind == Gfx_EventKind_KeyPress)
			{
				if (event->key == Gfx_Key_F11)
				{
					gfx_toggle_fullscreen(&gfx);
				}
			}
		}

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);

		UI_Key my_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu"));

		ui_ctx_menu(my_ctx_menu)
			ui_width(ui_em(4, 1))
		{
			if (ui_button(str8_lit("Test")).pressed)
			{
				printf("Hello world!");
			}

			if (ui_button(str8_lit("Test2")).pressed)
			{
				printf("Hello world!");
			}

			if (ui_button(str8_lit("Test3")).pressed)
			{
				printf("Hello world!");
			}
		}

		UI_Key my_ctx_menu2 = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu2"));

		ui_ctx_menu(my_ctx_menu2)
			ui_width(ui_em(4, 1))
		{
			if (ui_button(str8_lit("Test5")).pressed)
			{
				printf("Hello world!");
			}

			ui_row()
			{
				if (ui_button(str8_lit("Test52")).pressed)
				{
					printf("Hello world!");
				}
				if (ui_button(str8_lit("Test5251")).pressed)
				{
					printf("Hello world2!");
				}
			}
			if (ui_button(str8_lit("Test53")).pressed)
			{
				printf("Hello world!");
			}
		}

		ui_next_extra_box_flags(UI_BoxFlag_DrawBackground);
		ui_next_width(ui_fill());
		ui_corner_radius(0)
			ui_softness(0)
			ui_row()
		{
			UI_Comm comm = ui_button(str8_lit("File"));

			if (comm.hovering)
			{
				B32 ctx_menu_is_open = ui_key_match(ui_ctx_menu_key(), my_ctx_menu);
				if (ctx_menu_is_open)
				{
					ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
				}
			}
			if (comm.pressed)
			{
				ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
			}

			UI_Comm comm2 = ui_button(str8_lit("Edit"));

			if (comm2.hovering)
			{
				B32 ctx_menu2_is_open = ui_key_match(ui_ctx_menu_key(), my_ctx_menu2);
				if (ctx_menu2_is_open)
				{
					ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
				}
			}
			if (comm2.pressed)
			{
				ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
			}
			ui_button(str8_lit("View"));
			ui_button(str8_lit("Options"));
			ui_button(str8_lit("Help"));
		}

		ui_log_keep_alive(current_arena);

		ui_panel_update(renderer, &events);

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;

		app_state->frame_index++;
	}

	return(0);
}
