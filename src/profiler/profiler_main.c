#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "debug/debug_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "image/image_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "debug/debug_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "image/image_inc.c"
#include "ui/ui_inc.c"

#include "profiler/profiler_ui.h"

#include "profiler/profiler_ui.c"
#include "profiler/profiler_log_ui.c"
#include "profiler/profiler_texture_ui.c"

////////////////////////////////
//~ hampus: Tab views

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_debug)
{
	Debug_TimeBuffer *buffer = debug_get_times();
	ui_column()
	{
		for (U32 i = 0; i < buffer->count; ++i)
		{
			Debug_TimeEntry *entry = &buffer->buffer[i];
			ui_textf("%s: %.2fms", entry->name, (F32) (entry->end_ns - entry->start_ns) / (F32) million(1));
		}
	}
}

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_logger)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_logger();
}

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_texture_viewer)
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
	debug_init();
	log_init(str8_lit("log.txt"));

	Arena *perm_arena = arena_create();
	profiler_ui_state = push_struct(perm_arena, ProfilerUI_State);
	profiler_ui_state->perm_arena = perm_arena;

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	Render_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Render_TextureSlice image_texture = render_create_texture_slice(renderer, str8_lit("data/image.png"));

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	profiler_ui_state->cmd_buffer.buffer = push_array(profiler_ui_state->perm_arena, ProfilerUI_Command, CMD_BUFFER_SIZE);
	profiler_ui_state->cmd_buffer.size = CMD_BUFFER_SIZE;

	//- hampus: Build startup UI

	profiler_ui_set_color(ProfilerUI_Color_Panel, v4f32(0.2f, 0.2f, 0.2f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactivePanelBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_ActivePanelBorder, v4f32(1.0f, 0.8f, 0.0f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactivePanelOverlay, v4f32(0, 0, 0, 0.3f));
	profiler_ui_set_color(ProfilerUI_Color_TabBar, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_ActiveTab, v4f32(0.3f, 0.3f, 0.3f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactiveTab, v4f32(0.1f, 0.1f, 0.1f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabTitle, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabBarButtons, v4f32(0.1f, 0.1f, 0.1f, 1.0f));

	{
		ProfilerUI_Window *master_window = profiler_ui_window_make(profiler_ui_state->perm_arena, v2f32(1.0f, 1.0f));

		ProfilerUI_Panel *first_panel = master_window->root_panel;

		ProfilerUI_SplitPanelResult split_panel_result = profiler_ui_builder_split_panel(first_panel, Axis2_X);
		{
			{
				ProfilerUI_TabAttach attach =
				{
					.tab = profiler_ui_tab_make(profiler_ui_state->perm_arena,
												profiler_ui_tab_view_debug,
												0, str8_lit("Debug")),
					.panel = split_panel_result.panels[Side_Min],
				};
				profiler_ui_command_tab_attach(&attach);
			}
			{
				ProfilerUI_TabAttach attach =
				{
					.tab = profiler_ui_tab_make(profiler_ui_state->perm_arena,
												profiler_ui_tab_view_logger,
												0,
												str8_lit("Log")),
					.panel = split_panel_result.panels[Side_Min],
				};
				profiler_ui_command_tab_attach(&attach);
			}
			{
				ProfilerUI_TabAttach attach =
				{
					.tab = profiler_ui_tab_make(profiler_ui_state->perm_arena,
												profiler_ui_theme_tab,
												0, str8_lit("Theme")),
					.panel = split_panel_result.panels[Side_Min],
				};
				profiler_ui_command_tab_attach(&attach);
			}
			{
				ProfilerUI_TabAttach attach =
				{
					.tab = profiler_ui_tab_make(profiler_ui_state->perm_arena,
												profiler_ui_tab_view_texture_viewer,
												&image_texture,
												str8_lit("Texture Viewer")),
					.panel = split_panel_result.panels[Side_Max],
				};
				profiler_ui_command_tab_attach(&attach);
			}
			{
				ProfilerUI_TabAttach attach =
				{
					.tab = profiler_ui_tab_make(profiler_ui_state->perm_arena, 0, 0, str8_lit("")),
					.panel = split_panel_result.panels[Side_Max],
				};
				profiler_ui_command_tab_attach(&attach);
			}
		}

		profiler_ui_state->master_window = master_window;
		profiler_ui_state->next_focused_panel = first_panel;
	}

	profiler_ui_state->frame_index = 1;

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
			}

			if (ui_button(str8_lit("Test2")).pressed)
			{
			}

			if (ui_button(str8_lit("Test3")).pressed)
			{
			}
		}

		UI_Key my_ctx_menu2 = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu2"));

		ui_ctx_menu(my_ctx_menu2)
			ui_width(ui_em(4, 1))
		{
			if (ui_button(str8_lit("Test5")).pressed)
			{
			}

			ui_row()
			{
				if (ui_button(str8_lit("Test52")).pressed)
				{
				}
				if (ui_button(str8_lit("Test5251")).pressed)
				{
				}
			}
			if (ui_button(str8_lit("Test53")).pressed)
			{
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

		profiler_ui_update(renderer, &events);

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;

		profiler_ui_state->frame_index++;
	}

	return(0);
}
