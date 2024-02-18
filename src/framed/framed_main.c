////////////////////////////////
// Stuff to get done before going public
//
// [ ] Settings tab for font size, ...
// [ ] Finish basic zone profiling
//	 	 [ ] Add a concept of frames
//     [ ] Macro for turning on/off profiling
// [ ] Macro for making functions static
// [ ] Move log file to temporary folder

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

// NOTE(hampus): A global frame counter that everyone can read from
global U64 framed_frame_counter;

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

typedef struct ZoneBlock ZoneBlock;
struct ZoneBlock
{
	Str8 name;
	U64 tsc_elapsed;
	U64 tsc_elapsed_root;
	U64 tsc_elapsed_children;
	U64 hit_count;
};
typedef struct ZoneStack ZoneStack;
struct ZoneStack
{
	Str8 name;
	U64 tsc_start;
	U64 old_tsc_elapsed_root;
	U64 tsc_elapsed_children;
};

typedef struct ZoneBlockPacket ZoneBlockPacket;
struct ZoneBlockPacket
{
	U64 tsc;
	U64 name_length;
	U8 name[256];
};

global ZoneBlock zone_blocks[4096] = {0};
global ZoneBlock *current_zone_block = zone_blocks;

// NOTE(simon): Don't nest more than 1024 blocks, please
global ZoneStack zone_stack[1024] = {0};
global U32 zone_stack_size = 1;

internal ZoneBlock *
framed_get_zone_block(Arena *arena, Str8 name)
{
	U64 hash = hash_str8(name);
	U64 slot_index = hash % array_count(zone_blocks);
	while (zone_blocks[slot_index].name.data)
	{
		if (str8_equal(zone_blocks[slot_index].name, name))
		{
			break;
		}

		slot_index = (slot_index + 1) % array_count(zone_blocks);
	}

	ZoneBlock *result = &zone_blocks[slot_index];

	if (!result->name.data)
	{
		result->name = str8_copy(arena, name);
	}

	return(result);
}

FRAME_UI_TAB_VIEW(framed_ui_tab_view_counters)
{
	ui_push_font(str8_lit("data/fonts/liberation-mono.ttf"));
	Arena_Temporary scratch = get_scratch(0, 0);

	F32 name_column_width_em = 15;
	F32 cycles_column_width_em = 8;
	F32 cycles_children_column_width_em = 12;
	F32 hit_count_column_width_em = 8;
	ui_row()
	{
		ui_next_width(ui_em(name_column_width_em, 1));
		ui_row()
		{
			ui_next_width(ui_fill());
			ui_next_text_align(UI_TextAlign_Left);
			ui_text(str8_lit("Name"));
		}
		ui_next_width(ui_em(cycles_column_width_em, 1));
		ui_row()
		{
			ui_next_width(ui_fill());
			ui_next_text_align(UI_TextAlign_Left);
			ui_text(str8_lit("Cycles"));
		}
		ui_next_width(ui_em(cycles_children_column_width_em, 1));
		ui_row()
		{
			ui_next_width(ui_fill());
			ui_next_text_align(UI_TextAlign_Left);
			ui_text(str8_lit("Cycles w/ children"));
		}
		ui_next_width(ui_em(hit_count_column_width_em, 1));
		ui_row()
		{
			ui_next_width(ui_fill());
			ui_next_text_align(UI_TextAlign_Left);
			ui_text(str8_lit("Hit count"));
		}
	}
	ui_spacer(ui_em(0.3f, 1));
	for (U64 i = 0; i < array_count(zone_blocks); ++i)
	{
		ZoneBlock *zone_block = zone_blocks + i;
		if (zone_block->tsc_elapsed)
		{
			U64 tsc_without_children = zone_block->tsc_elapsed - zone_block->tsc_elapsed_children;

			ui_row()
			{
				ui_next_width(ui_em(name_column_width_em, 1));
				ui_row()
				{
					ui_next_width(ui_fill());
					ui_next_text_align(UI_TextAlign_Left);
					ui_text(zone_block->name);
				}
				ui_next_width(ui_em(cycles_column_width_em, 1));
				ui_row()
				{
					ui_next_width(ui_fill());
					ui_next_text_align(UI_TextAlign_Right);
					ui_textf("%"PRIU64, tsc_without_children);
				}
				ui_next_width(ui_em(cycles_children_column_width_em, 1));
				ui_row()
				{
					ui_next_width(ui_fill());
					ui_next_text_align(UI_TextAlign_Right);
					ui_textf("%"PRIU64, zone_block->tsc_elapsed_root);
				}
				ui_next_width(ui_em(hit_count_column_width_em, 1));
				ui_row()
				{
					ui_next_width(ui_fill());
					ui_next_text_align(UI_TextAlign_Right);
					ui_textf("%"PRIU64, zone_block->hit_count);
				}
			}
		}
	}
	release_scratch(scratch);
	ui_pop_font();
}

////////////////////////////////
// hampus: Main

internal S32
os_main(Str8List arguments)
{
	debug_init();
	log_init(str8_lit("log.txt"));

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
	net_socket_bind(socket, address);;
	net_socket_set_blocking_mode(socket, false);

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
	FramedUI_Window *master_window = framed_ui_window_make(v2f32(0, 0), monitor_dim);

	{
		// NOTE(hampus): Setup master window
		FramedUI_Panel *first_panel = master_window->root_panel;
		FramedUI_SplitPanelResult split_panel_result = framed_ui_builder_split_panel(first_panel, Axis2_X);
		{
			FramedUI_TabAttach attach =
			{
				.tab = framed_ui_tab_make(framed_ui_tab_view_theme, 0, str8_lit("Theme")),
				.panel = split_panel_result.panels[Side_Min],
			};
			framed_ui_command_tab_attach(&attach);
		}
		{
			FramedUI_TabAttach attach =
			{
				.tab = framed_ui_tab_make(framed_ui_tab_view_counters, 0, str8_lit("Counters")),
				.panel = split_panel_result.panels[Side_Max],
			};
			framed_ui_command_tab_attach(&attach);
		}

		framed_ui_state->master_window = master_window;
		framed_ui_state->next_focused_panel = first_panel;
	}

	gfx_set_window_maximized(&gfx);
	gfx_show_window(&gfx);

	B32 running = true;
	B32 found_connection = false;
	Net_AcceptResult accept_result = {0};
	FramedUI_Window *debug_window = 0;
	while (running)
	{
		Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		framed_ui_state->frame_arena = current_arena;

		// NOTE(hampus): Gather events

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
					if (debug_window)
					{
						framed_ui_window_close(debug_window);
						debug_window = 0;
					}
					else
					{
						debug_window = framed_ui_window_make(v2f32(0, 50), v2f32(500, 500));
						{
							// NOTE(hampus): Setup debug window
							{
								FramedUI_TabAttach attach =
								{
									.tab = framed_ui_tab_make(framed_ui_tab_view_debug, 0, str8_lit("Debug")),
									.panel = debug_window->root_panel,
								};
								framed_ui_command_tab_attach(&attach);
							}
							{
								FramedUI_TabAttach attach =
								{
									.tab = framed_ui_tab_make(framed_ui_tab_view_logger, 0,
									str8_lit("Log")),
									.panel = debug_window->root_panel,
								};
								framed_ui_command_tab_attach(&attach);
							}
							{
								FramedUI_TabAttach attach =
								{
									.tab = framed_ui_tab_make(framed_ui_tab_view_texture_viewer, &image_texture, str8_lit("Texture Viewer")),
									.panel = debug_window->root_panel,
								};
								framed_ui_command_tab_attach(&attach);
							}
						}
					}
					// TODO(hampus): Make debug window size dependent on window
					// size. We can't go maximized and then query the window
					// size, because on windows going maximized also shows the
					// window which we don't want. So this will be a temporary
					// solution
				}
			}
		}

		// NOTE(hampus): Check for connection

		if (!net_socket_connection_is_alive(accept_result.socket))
		{
			if (found_connection)
			{
				// NOTE(hampus): The client disconnected
				log_info("Disconnected from client");
				found_connection = false;
			}
			accept_result = net_socket_accept(socket);
			found_connection = accept_result.succeeded;
			if (found_connection)
			{
				log_info("Connected to client");
				net_socket_set_blocking_mode(accept_result.socket, false);
				memory_zero_array(zone_blocks);
			}
		}

		// NOTE(hampus): Gather zone data from client

		if (net_socket_connection_is_alive(accept_result.socket))
		{
			Arena_Temporary scratch = get_scratch(0, 0);
			U64 buffer_size = 4096*4096;
			U8 *buffer = push_array(scratch.arena, U8, buffer_size);
			Net_RecieveResult recieve_result = net_socket_recieve(accept_result.socket, buffer, buffer_size);
			U8 *buffer_pointer = buffer;
			U8 *buffer_opl = buffer + recieve_result.bytes_recieved;
			while (buffer_pointer < buffer_opl)
			{
				// TODO(simon): Make sure we don't try to read past the end of the buffer.
				ZoneBlockPacket *packet = (ZoneBlockPacket *) buffer_pointer;
				buffer_pointer += packet->name_length + sizeof(U64)*2;

				B32 opening_block = packet->name_length != 0;
				if (opening_block)
				{
					Str8 name = str8(packet->name, packet->name_length);

					assert(zone_stack_size < array_count(zone_stack));
					ZoneStack *opening = &zone_stack[zone_stack_size++];
					memory_zero_struct(opening);

					ZoneBlock *zone = framed_get_zone_block(perm_arena, name);

					opening->name = zone->name;
					opening->tsc_start = packet->tsc;
					opening->old_tsc_elapsed_root = zone->tsc_elapsed_root;
				}
				// NOTE(simon): If we haven't opened any zones, skip closing events
				else
				{
					ZoneStack *opening = &zone_stack[--zone_stack_size];
					ZoneBlock *zone = framed_get_zone_block(perm_arena, opening->name);

					U64 tsc_elapsed = packet->tsc - opening->tsc_start;

					zone->tsc_elapsed += tsc_elapsed;
					zone->tsc_elapsed_root = opening->old_tsc_elapsed_root + tsc_elapsed;
					zone->tsc_elapsed_children += opening->tsc_elapsed_children;
					++zone->hit_count;
					zone_stack[zone_stack_size - 1].tsc_elapsed_children += tsc_elapsed;
				}
			}
			release_scratch(scratch);
		}

		// NOTE(hampus): UI pass

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);
		ui_push_font(str8_lit("data/fonts/Inter-Regular.ttf"));
		ui_push_font_size(framed_ui_state->settings.font_size);

		// NOTE(hampus): Menu bar

		ui_next_extra_box_flags(UI_BoxFlag_DrawBackground);
		ui_next_width(ui_fill());
		ui_corner_radius(0)
			ui_softness(0)
			ui_row()
		{
			ui_button(str8_lit("File"));
			ui_button(str8_lit("Edit"));
			ui_button(str8_lit("View"));
			ui_button(str8_lit("Options"));
			ui_button(str8_lit("Help"));
		}

		// NOTE(hampus): Update panels

		framed_ui_update(renderer, &events);

		// NOTE(hampus): Status bar

		Str8 status_text = str8_lit("Not connected");
		if (net_socket_connection_is_alive(accept_result.socket))
		{
			ui_next_color(v4f32(0, 0.5f, 0, 1));
			status_text = str8_lit("Connected");
		}
		else
		{
			ui_next_color(v4f32(1, 0.5f, 0, 1));
		}
		ui_next_width(ui_pct(1, 1));
		ui_next_height(ui_em(1, 1));
		ui_next_text_align(UI_TextAlign_Left);
		UI_Box *status_bar_box = ui_box_make(UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawText, str8_lit(""));
		ui_box_equip_display_string(status_bar_box, status_text);

		ui_end();

		render_end(renderer);

		ui_debug_keep_alive((U32) framed_frame_counter);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);

		start_counter = end_counter;
		framed_frame_counter++;
	}

	return(0);
}
