////////////////////////////////
// Stuff to get done before going public
// 
// [ ] Put all debug visualization stuff on its own toggleable window
// [ ] Add a git readme
// [ ] Create a panel for displaying clock values with the name as text

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "debug/debug_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "image/image_inc.h"
#include "ui/ui_inc.h"
#include "net/net_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "debug/debug_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "image/image_inc.c"
#include "ui/ui_inc.c"
#include "net/net_inc.c"

#include "framed/framed_ui.h"

#include "framed/framed_ui.c"

////////////////////////////////
// hampus: Tab views

FRAME_UI_TAB_VIEW(framed_ui_tab_view_logger)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	B32 view_info_data_initialized = view_info->data != 0;
	LogUI_State *log_ui = framed_ui_get_view_data(view_info, LogUI_State);
	if (!view_info_data_initialized)
	{
		log_ui->perm_arena = arena_create("LogUIPerm");
	}
	ui_logger(log_ui);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_texture_viewer)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	Render_TextureSlice texture = *(Render_TextureSlice *) view_info->data;
	ui_texture_view(texture);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_debug)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	B32 view_info_data_initialized = view_info->data != 0;
	UI_ColorPickerData *color_picker_data = framed_ui_get_view_data(view_info, UI_ColorPickerData);
	ui_debug(color_picker_data);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_theme)
{
	B32 view_info_data_initialized = view_info->data != 0;

	UI_ColorPickerData *color_picker_data = framed_ui_get_view_data(view_info, UI_ColorPickerData);

	UI_Key theme_color_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("ThemeColorCtxMenu"));

	ui_ctx_menu(theme_color_ctx_menu)
	{
		ui_color_picker(color_picker_data);
	}

	ui_column()
	{
		for (FramedUI_Color color = (FramedUI_Color) 0; color < FramedUI_Color_COUNT; ++color)
		{
			ui_next_width(ui_em(20, 1));
			ui_row()
			{
				Str8 string = framed_ui_string_from_color(color);
				ui_text(string);
				ui_spacer(ui_fill());
				ui_next_color(framed_ui_color_from_theme(color));
				ui_next_hover_cursor(Gfx_Cursor_Hand);
				ui_next_corner_radius(5);
				ui_next_width(ui_em(1, 1));
				ui_next_height(ui_em(1, 1));
				UI_Box *box = ui_box_make(
					UI_BoxFlag_DrawBackground |
					UI_BoxFlag_DrawBorder |
					UI_BoxFlag_Clickable |
					UI_BoxFlag_HotAnimation |
					UI_BoxFlag_ActiveAnimation,
					string
				);
				UI_Comm comm = ui_comm_from_box(box);
				if (comm.clicked)
				{
					ui_ctx_menu_open(box->key, v2f32(0, 0), theme_color_ctx_menu);
					color_picker_data->rgba = &framed_ui_state->theme.colors[color];
				}
			}
			ui_spacer(ui_em(0.5f, 1));
		}
		arena_scratch(0, 0)
		{
			ui_next_width(ui_text_content(1));
			if (ui_button(str8_lit("Dump theme to file")).clicked)
			{
				Str8List string_list = {0};
				for (FramedUI_Color color = (FramedUI_Color) 0; color < FramedUI_Color_COUNT; ++color)
				{
					Str8 label = framed_ui_string_from_color(color);
					Vec4F32 color_value = framed_ui_color_from_theme(color);
					str8_list_push(scratch, &string_list, label);
					str8_list_pushf(
						scratch, &string_list, ": %.2f, %.2f, %.2f, %.2f\n",
						color_value.r,
						color_value.g,
						color_value.b,
						color_value.a
					);
				}
				Str8 dump_data = str8_join(scratch, &string_list);
				Str8 theme_dump_file_name = str8_lit("theme_dump.framed");
				os_file_write(theme_dump_file_name, dump_data, OS_FileMode_Replace);
			}
		}
	}
}

////////////////////////////////
// hampus: Main

internal S32
os_main(Str8List arguments)
{
	debug_init();
	log_init(str8_lit("log.txt"));

#if 0
	B32 host = false;

	for (Str8Node *node = arguments.first; node != 0; node = node->next)
	{
		if (str8_equal(node->string, str8_lit("-host")))
		{
			host = true;
			break;
		}
	}

	net_socket_init();
	Net_Socket socket = net_socket_alloc(Net_Protocol_TCP, Net_AddressFamily_INET);
	Net_Address address =
	{
		.ip.u8[0] = 127,
		.ip.u8[1] = 0,
		.ip.u8[2] = 0,
		.ip.u8[3] = 1,
		.port = 1234,
	};
	if (host)
	{
		net_socket_bind(socket, address);
		Net_AcceptResult accept_result = net_socket_accept(socket);
		U8 buffer[256] = {0};
		net_socket_recieve_from(accept_result.socket, accept_result.address, buffer, array_count(buffer));
		log_info((CStr) buffer);
	}
	else
	{
		net_socket_connect(socket, address);
		Str8 buffer = str8_lit("Hello socket!");
		net_socket_send_to(socket, address, buffer);
	}
#endif

	Arena *perm_arena = arena_create("MainPerm");

	framed_ui_state = push_struct(perm_arena, FramedUI_State);
	framed_ui_state->perm_arena = perm_arena;

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Framed"));
	Render_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create("MainFrame0");
	frame_arenas[1] = arena_create("MainFrame1");

	Render_TextureSlice image_texture = {0};
	if (arguments.first->next)
	{
		image_texture = render_create_texture_slice(renderer, arguments.first->next->string);
	}
	else
	{
		image_texture = render_create_texture_slice(renderer, str8_lit("data/16.png"));
	}

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	// NOTE(hampus): Build startup UI

	framed_ui_set_color(FramedUI_Color_Panel, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
	framed_ui_set_color(FramedUI_Color_InactivePanelBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	framed_ui_set_color(FramedUI_Color_ActivePanelBorder, v4f32(1.0f, 0.8f, 0.0f, 1.0f));
	framed_ui_set_color(FramedUI_Color_InactivePanelOverlay, v4f32(0, 0, 0, 0.3f));
	framed_ui_set_color(FramedUI_Color_TabBar, v4f32(0.15f, 0.15f, 0.15f, 1.0f));
	framed_ui_set_color(FramedUI_Color_ActiveTab, v4f32(0.3f, 0.3f, 0.3f, 1.0f));
	framed_ui_set_color(FramedUI_Color_InactiveTab, v4f32(0.1f, 0.1f, 0.1f, 1.0f));
	framed_ui_set_color(FramedUI_Color_TabTitle, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	framed_ui_set_color(FramedUI_Color_TabBorder, v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	framed_ui_set_color(FramedUI_Color_TabBarButtons, v4f32(0.1f, 0.1f, 0.1f, 1.0f));

	Gfx_Monitor monitor = gfx_monitor_from_window(&gfx);
	Vec2F32 monitor_dim = gfx_dim_from_monitor(monitor);
	FramedUI_Window *master_window = framed_ui_window_make(framed_ui_state->perm_arena, v2f32(0, 0), monitor_dim);

	// TODO(hampus): Make debug window size dependent on window size. We can't go maxiximized and then query
	// the window size, because on windows going maximized also shows the window which we don't want. So
	// this will be a temporary solution
	FramedUI_Window *debug_window = framed_ui_window_make(framed_ui_state->perm_arena, v2f32(0, 50), v2f32(500, 500));
	{
		{
			// NOTE(hampus): Setup master window
			FramedUI_Panel *first_panel = master_window->root_panel;
			FramedUI_SplitPanelResult split_panel_result = framed_ui_builder_split_panel(first_panel, Axis2_X);
			{
				FramedUI_TabAttach attach =
				{
					.tab = framed_ui_tab_make(framed_ui_state->perm_arena,
					framed_ui_tab_view_theme,
					0, str8_lit("Theme")),
					.panel = split_panel_result.panels[Side_Min],
				};
				framed_ui_command_tab_attach(&attach);
			}
			{
				FramedUI_TabAttach attach =
				{
					.tab = framed_ui_tab_make(framed_ui_state->perm_arena, 0, 0, str8_lit("")),
					.panel = split_panel_result.panels[Side_Max],
				};
				framed_ui_command_tab_attach(&attach);
			}

			framed_ui_state->master_window = master_window;
			framed_ui_state->next_focused_panel = first_panel;
		}

		{
			// NOTE(hampus): Setup debug window
			{
				FramedUI_TabAttach attach =
				{
					.tab = framed_ui_tab_make(framed_ui_state->perm_arena, framed_ui_tab_view_debug,
					0, str8_lit("Debug")),
					.panel = debug_window->root_panel,
				};
				framed_ui_command_tab_attach(&attach);
			}
			{
				FramedUI_TabAttach attach =
				{
					.tab = framed_ui_tab_make(framed_ui_state->perm_arena, framed_ui_tab_view_logger,
					0,
					str8_lit("Log")),
					.panel = debug_window->root_panel,
				};
				framed_ui_command_tab_attach(&attach);
			}
			{
				FramedUI_TabAttach attach =
				{
					.tab = framed_ui_tab_make(framed_ui_state->perm_arena, framed_ui_tab_view_texture_viewer,
					&image_texture,
					str8_lit("Texture Viewer")),
					.panel = debug_window->root_panel,
				};
				framed_ui_command_tab_attach(&attach);
			}
			// debug_window->flags |= FramedUI_WindowFlags_Closed;
		}
	}

	framed_ui_state->frame_index = 1;
	gfx_set_window_maximized(&gfx);
	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		framed_ui_state->frame_arena = current_arena;

		Gfx_EventList events = gfx_get_events(current_arena, &gfx);
		for (Gfx_Event *event = events.first; event != 0; event = event->next)
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
				else if (event->key == Gfx_Key_F1)
				{
					// debug_window->flags ^= FramedUI_WindowFlags_Closed;
				}
			}
		}

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);
		U32 font_size = 15;
		ui_push_font(str8_lit("data/fonts/Inter-Regular.ttf"));
		ui_push_font_size(font_size);

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

		framed_ui_update(renderer, &events);

		render_end(renderer);

		ui_debug_keep_alive((U32) framed_ui_state->frame_index);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);

		start_counter = end_counter;
		framed_ui_state->frame_index++;
	}

	return(0);
}
