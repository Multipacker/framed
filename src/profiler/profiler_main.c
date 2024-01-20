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

typedef struct Debug_Statistics Debug_Statistics;
struct Debug_Statistics
{
	CStr file;
	U32  line;
	Str8 name;
	U64  total_time_ns;
	U64  hit_count;
};

global Debug_Statistics *ui_debug_stats;
global U32               ui_debug_stat_count;

internal Void ui_debug_keep_alive(Arena *arena)
{
	Debug_TimeBuffer *buffer = debug_get_times();

	ui_debug_stats      = push_array(arena, Debug_Statistics, buffer->count);
	ui_debug_stat_count = 0;

	for (U32 i = 0; i < buffer->count; ++i)
	{
		Debug_TimeEntry *entry = &buffer->buffer[i];

		U32 stat_index = 0;
		for (; stat_index < ui_debug_stat_count; ++stat_index)
		{
			if (ui_debug_stats[stat_index].file == entry->file && ui_debug_stats[stat_index].line == entry->line)
			{
				break;
			}
		}

		if (stat_index == ui_debug_stat_count)
		{
			ui_debug_stats[stat_index].file          = entry->file;
			ui_debug_stats[stat_index].line          = entry->line;
			ui_debug_stats[stat_index].name          = str8_cstr(entry->name);
			ui_debug_stats[stat_index].total_time_ns = entry->end_ns - entry->start_ns;
			ui_debug_stats[stat_index].hit_count     = 1;
			++ui_debug_stat_count;
		}
		else
		{
			ui_debug_stats[stat_index].total_time_ns += entry->end_ns - entry->start_ns;
			++ui_debug_stats[stat_index].hit_count;
		}
	}
}

typedef struct TimeInterval TimeInterval;
struct TimeInterval
{
	F64 amount;
	Str8 unit;
};

internal TimeInterval
time_interval_from_ns(F64 ns)
{
	Str8 units[] = {
		str8_lit("us"),
		str8_lit("ms"),
		str8_lit("s"),
		str8_lit("min"),
		str8_lit("h"),
	};

	// NOTE(simon): Divisors for going from one stage to the next.
	F64 divisors[] = {
		1000.0f,
		1000.0f,
		1000.0f,
		60.0f,
		60.0f,
	};

	F64 limits[] = {
		10000.0f,
		10000.0f,
		10000.0f,
		600.0f,
		600.0f,
	};

	TimeInterval result;
	result.amount = (F64) ns;
	result.unit   = str8_lit("ns");
	for (U32 i = 0; i < array_count(limits) && result.amount > limits[i]; ++i)
	{
		result.amount /= divisors[i];
		result.unit    = units[i];
	}

	return(result);
}

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_debug)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_named_row(str8_lit("DebugTimes"))
	{
		Str8 headers[] = {
			str8_lit("Name"),
			str8_lit("Total time"),
			str8_lit("Avg. time"),
			str8_lit("Hit count"),
		};
		local F32 splits[] = { 0.25f, 0.25f, 0.25f, 0.25f };
		UI_Box *columns[4] = { 0 };
		local B32 reverse = true;
		local U32 sort_column = 1;

		F32 drag_delta = 0.0f;
		U32 drag_index = 0;
		ui_corner_radius(0)
			ui_color(v4f32(0.4f, 0.4f, 0.4f, 1.0f))
		{
			for (U32 i = 0; i < array_count(columns); ++i)
			{
				ui_next_width(ui_pct(splits[i], 0));
				ui_next_extra_box_flags(UI_BoxFlag_Clip);
				columns[i] = ui_named_column_beginf("column%"PRIU32, i);
				ui_push_seed(columns[i]->key);

				ui_next_width(ui_pct(1, 1));
				ui_next_height(ui_children_sum(1));
				ui_next_child_layout_axis(Axis2_X);
				UI_Box *header = ui_box_make(UI_BoxFlag_Clickable |
											 UI_BoxFlag_HotAnimation |
											 UI_BoxFlag_ActiveAnimation,
											 str8_lit("Header"));
				UI_Comm header_comm = ui_comm_from_box(header);
				if (header_comm.clicked)
				{
					reverse = (i == sort_column ? !reverse : false);
					sort_column = i;
				}
				if (header_comm.hovering)
				{
					header->flags |= UI_BoxFlag_DrawBackground;
				}

				ui_parent(header)
				{
					ui_text(headers[i]);

					ui_spacer(ui_fill());

					if (i == sort_column)
					{
						ui_next_width(ui_em(1, 1));
						ui_next_height(ui_em(1, 1));
						ui_next_icon(reverse ? RENDER_ICON_UP : RENDER_ICON_DOWN);
						ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
						ui_spacer(ui_em(0.2f, 1));
					}
				}

				ui_next_width(ui_pct(1, 1));
				ui_next_height(ui_em(0.2f, 1));
				ui_next_color(v4f32(0.4f, 0.4f, 0.4f, 1.0f));
				ui_next_corner_radius(0);
				ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

				ui_named_column_end();
				ui_pop_seed();

				if (i + 1 < array_count(columns))
				{
					ui_next_width(ui_em(0.2f, 1));
					ui_next_height(ui_pixels(columns[i]->fixed_size.height, 1));
					ui_next_hover_cursor(Gfx_Cursor_SizeWE);
					UI_Box *draggable_box = ui_box_make(UI_BoxFlag_Clickable |
														UI_BoxFlag_DrawBackground,
														str8_pushf(ui_frame_arena(), "dragger%"PRIU32, i)
														);

					UI_Comm draggin_comm = ui_comm_from_box(draggable_box);
					if (draggin_comm.dragging)
					{
						drag_delta = draggin_comm.drag_delta.x / ui_top_parent()->fixed_size.width;
						drag_index = i;
					}
				}
			}
		}

		splits[drag_index + 0] = f32_clamp(0.0f, splits[drag_index + 0] - drag_delta, 1.0f);
		splits[drag_index + 1] = f32_clamp(0.0f, splits[drag_index + 1] + drag_delta, 1.0f);

		// NOTE(simon): Bubble sort for the win!
		if (sort_column == 0)
		{
			for (U32 i = 0; i < ui_debug_stat_count; ++i)
			{
				for (U32 j = 0; j < ui_debug_stat_count - i - 1; ++j)
				{
					Str8 this_name = ui_debug_stats[j + 0].name;
					Str8 next_name = ui_debug_stats[j + 1].name;
					if (str8_are_codepoints_earliear(next_name, this_name))
					{
						swap(ui_debug_stats[j], ui_debug_stats[j + 1], Debug_Statistics);
					}
				}
			}
		}
		else if (sort_column == 1)
		{
			for (U32 i = 0; i < ui_debug_stat_count; ++i)
			{
				for (U32 j = 0; j < ui_debug_stat_count - i - 1; ++j)
				{
					if (ui_debug_stats[j].total_time_ns > ui_debug_stats[j + 1].total_time_ns)
					{
						swap(ui_debug_stats[j], ui_debug_stats[j + 1], Debug_Statistics);
					}
				}
			}
		}
		else if (sort_column == 2)
		{
			for (U32 i = 0; i < ui_debug_stat_count; ++i)
			{
				for (U32 j = 0; j < ui_debug_stat_count - i - 1; ++j)
				{
					F64 this_avg_time = (F64) ui_debug_stats[j + 0].total_time_ns / (F64) ui_debug_stats[j + 0].hit_count;
					F64 next_avg_time = (F64) ui_debug_stats[j + 1].total_time_ns / (F64) ui_debug_stats[j + 1].hit_count;
					if (this_avg_time > next_avg_time)
					{
						swap(ui_debug_stats[j], ui_debug_stats[j + 1], Debug_Statistics);
					}
				}
			}
		}
		else if (sort_column == 3)
		{
			for (U32 i = 0; i < ui_debug_stat_count; ++i)
			{
				for (U32 j = 0; j < ui_debug_stat_count - i - 1; ++j)
				{
					if (ui_debug_stats[j].hit_count > ui_debug_stats[j + 1].hit_count)
					{
						swap(ui_debug_stats[j], ui_debug_stats[j + 1], Debug_Statistics);
					}
				}
			}
		}

		if (reverse)
		{
			for (U32 i = 0, j = ui_debug_stat_count; i < j; ++i, --j)
			{
				swap(ui_debug_stats[i], ui_debug_stats[j - 1], Debug_Statistics);
			}
		}

		ui_parent(columns[0])
		{
			for (U32 i = 0; i < ui_debug_stat_count; ++i)
			{
				Debug_Statistics *entry = &ui_debug_stats[i];
				ui_text(entry->name);
			}
		}
		ui_parent(columns[1])
		{
			ui_width(ui_fill())
				ui_text_align(UI_TextAlign_Right)
			{
				for (U32 i = 0; i < ui_debug_stat_count; ++i)
				{
					Debug_Statistics *entry = &ui_debug_stats[i];
					TimeInterval total_time   = time_interval_from_ns((F64) entry->total_time_ns);
					ui_textf("%.2f%"PRISTR8, total_time.amount, str8_expand(total_time.unit));
				}
			}
		}
		ui_parent(columns[2])
		{
			ui_width(ui_fill())
				ui_text_align(UI_TextAlign_Right)
			{
				for (U32 i = 0; i < ui_debug_stat_count; ++i)
				{
					Debug_Statistics *entry = &ui_debug_stats[i];
					TimeInterval average_time = time_interval_from_ns((F64) entry->total_time_ns / (F64) entry->hit_count);
					ui_textf("%.2f%"PRISTR8, average_time.amount, str8_expand(average_time.unit));
				}
			}
		}
		ui_parent(columns[3])
		{
			ui_width(ui_fill())
				ui_text_align(UI_TextAlign_Right)
			{
				for (U32 i = 0; i < ui_debug_stat_count; ++i)
				{
					Debug_Statistics *entry = &ui_debug_stats[i];
					ui_textf("%"PRIU32, entry->hit_count);
				}
			}
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

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_theme)
{
	B32 initialized = data != 0;

	typedef struct ThemeViewState ThemeViewState;
	struct ThemeViewState
	{
		Vec4F32 hsva[ProfilerUI_Color_COUNT];
	};

	ThemeViewState *theme_view_state = profiler_ui_get_or_push_view_data(data, ThemeViewState);

	if (!initialized)
	{
		for (ProfilerUI_Color color = (ProfilerUI_Color)0;
			 color < ProfilerUI_Color_COUNT;
			 ++color)
		{
			Vec4F32 rgba = profiler_ui_color_from_theme(color);
			theme_view_state->hsva[color].rgb = hsv_from_rgb(rgba.rgb);
			theme_view_state->hsva[color].a = rgba.a;
		}
	}

	UI_Key theme_color_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("ThemeColorCtxMenu"));

	local ProfilerUI_Color selected_color = 0;

	ui_ctx_menu(theme_color_ctx_menu)
	{
		Vec4F32 *hsva = theme_view_state->hsva + selected_color;
		ui_next_width(ui_children_sum(1));
		ui_next_height(ui_children_sum(1));
		UI_Box *container = ui_box_make(UI_BoxFlag_DrawBackground |
										UI_BoxFlag_DrawBorder,
										str8_lit(""));
		ui_parent(container)
		{
			ui_spacer(ui_em(0.5f, 1));
			ui_row()
			{
				ui_spacer(ui_em(0.5f, 1));
				// NOTE(hampus): Saturation and value
				ui_next_width(ui_em(10, 1));
				ui_next_height(ui_em(10, 1));
				ui_sat_val_picker(hsva->x, &hsva->y, &hsva->z, str8_lit("SatValPicker"));
				ui_spacer(ui_em(0.5f, 1));
				ui_column()
				{
					// NOTE(hampus): Hue
					ui_next_height(ui_em(10, 1));
					ui_next_width(ui_em(1, 1));
					ui_hue_picker(&hsva->x, str8_lit("HuePicker"));
				}
				ui_spacer(ui_em(0.5f, 1));
				ui_column()
				{
					// NOTE(hampus): Alpha
					ui_next_height(ui_em(10, 1));
					ui_next_width(ui_em(1, 1));
					ui_alpha_picker(hsva->rgb, &hsva->a, str8_lit("AlphaPicker"));
				}
				profiler_ui_state->theme.colors[selected_color].rgb = rgb_from_hsv(hsva->rgb);
				profiler_ui_state->theme.colors[selected_color].a = hsva->a;
				ui_spacer(ui_em(0.5f, 1));
			}
			Vec4F32 rgba = profiler_ui_state->theme.colors[selected_color];
			ui_spacer(ui_em(0.5f, 1));
			ui_row()
				ui_width(ui_em(4, 1))
			{
				ui_textf("R: %.2f", rgba.r);
				ui_textf("G: %.2f", rgba.g);
				ui_textf("B: %.2f", rgba.b);
				ui_textf("A: %.2f", rgba.a);
			}
			ui_spacer(ui_em(0.5f, 1));
			ui_row()
				ui_width(ui_em(4, 1))
			{
				ui_textf("H: %.2f", hsva->x);
				ui_textf("S: %.2f", hsva->y);
				ui_textf("V: %.2f", hsva->z);
			}
			ui_spacer(ui_em(0.5f, 1));
		}
	}

	ui_column()
	{
		for (ProfilerUI_Color color = (ProfilerUI_Color)0;
			 color < ProfilerUI_Color_COUNT;
			 ++color)
		{
			ui_next_width(ui_em(20, 1));
			ui_row()
			{
				Str8 string = profiler_ui_string_from_color(color);
				ui_text(string);
				ui_spacer(ui_fill());
				ui_next_color(profiler_ui_color_from_theme(color));
				ui_next_hover_cursor(Gfx_Cursor_Hand);
				ui_next_corner_radius(5);
				ui_next_width(ui_em(1, 1));
				ui_next_height(ui_em(1, 1));
				UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
										  UI_BoxFlag_DrawBorder |
										  UI_BoxFlag_Clickable |
										  UI_BoxFlag_HotAnimation |
										  UI_BoxFlag_ActiveAnimation,
										  string);
				UI_Comm comm = ui_comm_from_box(box);
				if (comm.clicked)
				{
					ui_ctx_menu_open(box->key, v2f32(0, 0), theme_color_ctx_menu);
					selected_color = color;
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
				for (ProfilerUI_Color color = (ProfilerUI_Color)0;
					 color < ProfilerUI_Color_COUNT;
					 ++color)
				{
					Str8 label = profiler_ui_string_from_color(color);
					Vec4F32 color_value = profiler_ui_color_from_theme(color);
					str8_list_push(scratch, &string_list, label);
					str8_list_pushf(scratch, &string_list, ": %.2f, %.2f, %.2f, %.2f\n",
									color_value.r,
									color_value.g,
									color_value.b,
									color_value.a);
				}
				Str8 dump_data = str8_join(scratch, &string_list);
#if OS_LINUX
				Str8 theme_dump_file_name = str8_lit("theme_dump");
#else
				Str8 theme_dump_file_name = str8_lit("theme_dump.txt");
#endif
				os_file_write(theme_dump_file_name, dump_data, OS_FileMode_Replace);
			}
		}
	}
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

	profiler_ui_set_color(ProfilerUI_Color_Panel,                v4f32(0.2f, 0.2f, 0.2f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactivePanelBorder,  v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_ActivePanelBorder,    v4f32(1.0f, 0.8f, 0.0f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactivePanelOverlay, v4f32(0, 0, 0, 0.3f));
	profiler_ui_set_color(ProfilerUI_Color_TabBar,               v4f32(0.15f, 0.15f, 0.15f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_ActiveTab,            v4f32(0.3f, 0.3f, 0.3f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_InactiveTab,          v4f32(0.1f, 0.1f, 0.1f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabTitle,             v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabBorder,            v4f32(0.9f, 0.9f, 0.9f, 1.0f));
	profiler_ui_set_color(ProfilerUI_Color_TabBarButtons,        v4f32(0.1f, 0.1f, 0.1f, 1.0f));

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
												profiler_ui_tab_view_theme,
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

		ui_debug_keep_alive(current_arena);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;

		profiler_ui_state->frame_index++;
	}

	return(0);
}
