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

	Str8 path = str8_lit("data/image.png");
	Render_TextureSlice image_texture = { 0 };

	Str8 image_contents = { 0 };
	arena_scratch(0, 0)
	{
		if (os_file_read(scratch, path, &image_contents))
		{
			image_load(app_state->perm_arena, renderer, image_contents, &image_texture);
		}
		else
		{
			log_error("Could not load file '%"PRISTR8"'", str8_expand(path));
		}
	}

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	app_state->cmd_buffer.buffer = push_array(app_state->perm_arena, Cmd, CMD_BUFFER_SIZE);
	app_state->cmd_buffer.size = CMD_BUFFER_SIZE;

	//- hampus: Build startup UI

	{
		Window *master_window = ui_window_make(app_state->perm_arena, v2f32(1.0f, 1.0f));

		Panel *first_panel = master_window->root_panel;

		SplitPanelResult split_panel_result = ui_builder_split_panel(first_panel, Axis2_X);
		{
			{
				TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, ui_tab_view_logger, 0),
					.panel = split_panel_result.panels[Side_Min],
				};
				ui_command_tab_attach(&attach);
			}

			{
				TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, 0, 0),
					.panel = split_panel_result.panels[Side_Max],
				};
				ui_command_tab_attach(&attach);
			}
		}

		app_state->master_window = master_window;
		app_state->next_focused_panel = first_panel;
	}

	app_state->frame_index = 1;

	Vec2F32 prev_mouse_pos = {0};;

	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		B32 left_mouse_released = false;
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

				case Gfx_EventKind_KeyRelease:
				{
					if (event->key == Gfx_Key_MouseLeft)
					{
						left_mouse_released = true;
					}
				} break;

				default:
				{
				} break;
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
				if (ui_ctx_menu_is_open(my_ctx_menu))
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
				if (ui_ctx_menu_is_open(my_ctx_menu2))
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

		app_state->focused_panel = app_state->next_focused_panel;
		app_state->next_focused_panel = 0;

		ui_log_keep_alive(current_arena);

		//- hampus: Update Windows

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		UI_Box *window_root_parent = ui_box_make(0, str8_lit("RootWindow"));
		app_state->window_container = window_root_parent;
		ui_parent(window_root_parent)
		{
			for (Window *window = app_state->window_list.first;
				 window != 0;
				 window = window->next)
			{
				ui_update_window(window);
			}
		}

		//- hampus: Update tab drag

		if (left_mouse_released &&
			ui_currently_dragging())
		{
			ui_drag_release();
		}

		DragData *drag_data = &app_state->drag_data;
		switch (app_state->drag_status)
		{
			case DragStatus_Inactive: {} break;

			case DragStatus_WaitingForDragThreshold:
			{
				F32 drag_threshold = ui_em(2, 1).value;
				Vec2F32 delta = v2f32_sub_v2f32(g_ui_ctx->mouse_pos, drag_data->drag_origin);
				if (f32_abs(delta.x) > drag_threshold ||
					f32_abs(delta.y) > drag_threshold)
				{
					Tab *tab = drag_data->tab;

					// NOTE(hampus): Calculate the new window size
					Vec2F32 new_window_pct = v2f32(1, 1);
					Panel *panel_child = tab->panel;
					for (Panel *panel_parent = panel_child->parent;
						 panel_parent != 0;
						 panel_parent = panel_parent->parent)
					{
						Axis2 axis = panel_parent->split_axis;
						new_window_pct.v[axis] *= panel_child->pct_of_parent;
						panel_child = panel_parent;
					}

					new_window_pct.x *= tab->panel->window->size.x;
					new_window_pct.y *= tab->panel->window->size.y;

					Panel *tab_panel = tab->panel;
					B32 create_new_window = !(tab->panel == tab->panel->window->root_panel &&
											  tab_panel->tab_group.count == 1 &&
											  tab_panel->window != app_state->master_window);
					if (create_new_window)
					{
						// NOTE(hampus): Close the tab from the old panel
						{
							TabDelete tab_close =
							{
								.tab = drag_data->tab
							};
							ui_command_tab_close(&tab_close);
						}

						Window *new_window = ui_window_make(app_state->perm_arena, new_window_pct);

						{
							TabAttach tab_attach =
							{
								.tab = drag_data->tab,
								.panel = new_window->root_panel,
								.set_active = true,
							};
							ui_command_tab_attach(&tab_attach);
						}
					}
					else
					{
						drag_data->tab->panel->sibling = 0;
						ui_window_reorder_to_front(drag_data->tab->panel->window);
					}

					Vec2F32 offset = v2f32_sub_v2f32(drag_data->drag_origin, tab->box->fixed_rect.min);
					app_state->next_focused_panel = drag_data->tab->panel;
					app_state->drag_status = DragStatus_Dragging;
					drag_data->tab->panel->window->pos = v2f32_sub_v2f32(mouse_pos, offset);
				}

			} break;

			case DragStatus_Dragging:
			{
				Window *window = drag_data->tab->panel->window;
				Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, prev_mouse_pos);
				window->pos = v2f32_add_v2f32(window->pos, mouse_delta);;
			} break;

			case DragStatus_Released:
			{
				// NOTE(hampus): If it is the last tab of the window,
				// we don't need to allocate a new panel. Just use
				// the tab's panel
				memory_zero_struct(&app_state->drag_data);
				app_state->drag_status = DragStatus_Inactive;
			} break;

			invalid_case;
		}

		if (left_mouse_released &&
			ui_drag_is_prepared())
		{
			ui_drag_end();
		}

		if (app_state->next_top_most_window)
		{
			Window *window = app_state->next_top_most_window;
			ui_window_remove_from_list(window);
			ui_window_push_to_front(window);
		}

		app_state->next_top_most_window = 0;

		ui_end();

		for (U64 i = 0; i < app_state->cmd_buffer.pos; ++i)
		{
			Cmd *cmd = app_state->cmd_buffer.buffer + i;
			switch (cmd->kind)
			{
				case CmdKind_TabAttach: ui_command_tab_attach(cmd->data); break;
				case CmdKind_TabClose:  ui_command_tab_close(cmd->data); break;

				case CmdKind_PanelSplit:          ui_command_panel_split(cmd->data);            break;
				case CmdKind_PanelSplitAndAttach: ui_command_panel_split_and_attach(cmd->data); break;
				case CmdKind_PanelSetActiveTab:   ui_command_panel_set_active_tab(cmd->data);   break;
				case CmdKind_PanelClose:          ui_command_panel_close(cmd->data);            break;

				case CmdKind_WindowRemoveFromList: ui_command_window_remove_from_list(cmd->data); break;
				case CmdKind_WindowPushToFront:    ui_command_window_push_to_front(cmd->data);    break;
			}
		}

		if (!app_state->next_focused_panel)
		{
			app_state->next_focused_panel = app_state->focused_panel;
		}

		app_state->cmd_buffer.pos = 0;

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;

		app_state->frame_index++;

		prev_mouse_pos = mouse_pos;
	}

	return(0);
}
