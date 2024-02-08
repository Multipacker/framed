#define DEBUG_STAT_FRAMES 200
#define DEBUG_STAT_LOCATIONS 100

typedef struct Debug_Statistics Debug_Statistics;
struct Debug_Statistics
{
	CStr    file[DEBUG_STAT_LOCATIONS];
	U32     line[DEBUG_STAT_LOCATIONS];
	Str8    name[DEBUG_STAT_LOCATIONS];
	Vec4F32 colors[DEBUG_STAT_LOCATIONS];
	U32     total_time_ns[DEBUG_STAT_FRAMES][DEBUG_STAT_LOCATIONS];
	U32     hit_count[DEBUG_STAT_FRAMES][DEBUG_STAT_LOCATIONS];
	U64     latest_frame_index;
	U32     count;
};

typedef struct DebugMemoryStatistics DebugMemoryStatistics;
struct DebugMemoryStatistics
{
	Arena *arena;
	U64    max;
	U64    current;
	U64    change_count;
};

global DebugMemoryStatistics debug_arenas[100];
global U32                   debug_arena_count;
global Debug_Statistics ui_debug_stats;
global B32              ui_debug_freeze;
global Debug_MemoryBuffer *ui_debug_memory;

typedef struct LogUI_Thread LogUI_Thread;
struct LogUI_Thread
{
	LogUI_Thread *next;
	Str8 name;
	B32 show;
};

typedef struct LogUI_State LogUI_State;
struct LogUI_State {
	Arena *perm_arena;
	LogUI_Thread *threads;

	B32 compact_display;
	B32 freeze;
	B32 level_filters[Log_Level_COUNT];

	U32 entry_count;
	Log_QueueEntry entries[1000];
	U32 has_new_entries;
};

internal B32
ui_logger_entry_passes_filter(LogUI_State *state, Log_QueueEntry *entry)
{
	B32 show_all_threads = true;
	for (LogUI_Thread *thread = state->threads; thread; thread = thread->next)
	{
		show_all_threads &= !thread->show;
	}

	B32 show_all_levels = true;
	for (Log_Level level = 0; level < Log_Level_COUNT; ++level)
	{
		show_all_levels &= !state->level_filters[level];
	}

	B32 show = true;
	if (!show_all_threads)
	{
		Str8 thread_name = str8_cstr((CStr) entry->thread_name);
		for (LogUI_Thread *thread = state->threads; thread; thread = thread->next)
		{
			if (str8_equal(thread_name, thread->name))
			{
				show = thread->show;
				break;
			}
		}
	}

	if (!show_all_levels)
	{
		show &= state->level_filters[entry->level];
	}

	return(show);
}

internal Void
ui_logger_update_entries(LogUI_State *state)
{
	Log_EntryBuffer new_entries = *log_get_new_entries();

	if (state->entry_count + new_entries.count > array_count(state->entries))
	{
		U32 entries_to_keep     = (U32) s64_max(0, (S64) array_count(state->entries) - (S64) new_entries.count);
		U32 first_entry_to_keep = state->entry_count - entries_to_keep;
		memory_move(state->entries, &state->entries[first_entry_to_keep], entries_to_keep * sizeof(*state->entries));
		state->entry_count = entries_to_keep;
	}

	U32 entries_to_copy     = u32_min(array_count(state->entries) - state->entry_count, new_entries.count);
	U32 first_entry_to_copy = new_entries.count - entries_to_copy;
	memory_copy(&state->entries[state->entry_count], &new_entries.buffer[first_entry_to_copy], entries_to_copy * sizeof(*new_entries.buffer));

	// NOTE(simon): Look for any new threads.
	for (U32 i = state->entry_count; i < state->entry_count + entries_to_copy; ++i)
	{
		Str8 thread_name = str8_cstr((CStr) state->entries[i].thread_name);

		B32 is_new_thread = true;
		for (LogUI_Thread *thread = state->threads; thread; thread = thread->next)
		{
			if (str8_equal(thread_name, thread->name))
			{
				is_new_thread = false;
				break;
			}
		}

		if (is_new_thread)
		{
			LogUI_Thread *thread = push_struct(state->perm_arena, LogUI_Thread);
			thread->name = thread_name;
			thread->show = false;
			stack_push(state->threads, thread);
		}
	}

	// NOTE(simon): It takes one frame for us to get the correct sizes for clamping the scroll, so call it twice.
	state->has_new_entries |= (entries_to_copy != 0 ? 2 : 0);
	state->entry_count += entries_to_copy;
}

internal Void
ui_logger_checkbox(Str8 name, B32 *value) {
	ui_row()
	{
		ui_spacer(ui_em(0.4f, 1));
		ui_check(value, name);
		ui_spacer(ui_em(0.4f, 1));
		ui_text(name);
	}
}

internal Str8
ui_logger_format_entry(LogUI_State *state, Log_QueueEntry *entry)
{
	Str8 message = { 0 };

	if (state->compact_display)
	{
		Str8 path_display = str8_cstr(entry->file);
		U64 slash_index = 0;
		if (str8_last_index_of(path_display, PATH_SEPARATOR, &slash_index))
		{
			++slash_index;
		}
		path_display = str8_skip(path_display, slash_index);

		message = str8_pushf(
			ui_frame_arena(),
			"%.2u:%.2u:%.2u.%03u %s %"PRISTR8":%u: %s",
			entry->time.hour, entry->time.minute, entry->time.second, entry->time.millisecond,
			entry->thread_name,
			str8_expand(path_display), entry->line,
			entry->message
		);
	}
	else
	{
		// NOTE(simon): Skip the trailing newline.
		message = str8_chop(log_format_entry(ui_frame_arena(), entry), 1);
	}

	return(message);
}

internal Void
ui_logger(LogUI_State *state)
{
	if (!state->freeze)
	{
		ui_logger_update_entries(state);
	}

	ui_next_child_layout_axis(Axis2_Y);
	UI_Box *log_window = ui_box_make(0, str8_lit("LogWindow"));
	ui_parent(log_window)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_em(10, 1));
		ui_scrollable_region(str8_lit("LogControls"))
			ui_row()
		{
			ui_column()
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Freeze entries"), &state->freeze);
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Compact display"), &state->compact_display);
			}

			ui_spacer(ui_em(0.4f, 1));

			ui_column()
			{
				ui_text(str8_lit("Threads:"));

				for (LogUI_Thread *thread = state->threads; thread; thread = thread->next)
				{
					ui_spacer(ui_em(0.4f, 1));
					ui_logger_checkbox(thread->name, &thread->show);
				}
			}

			ui_spacer(ui_em(0.4f, 1));

			ui_column()
			{
				ui_text(str8_lit("Level filter:"));
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Infos"), &state->level_filters[Log_Level_Info]);
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Warnings"), &state->level_filters[Log_Level_Warning]);
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Errors"), &state->level_filters[Log_Level_Error]);
				ui_spacer(ui_em(0.4f, 1));
				ui_logger_checkbox(str8_lit("Traces"), &state->level_filters[Log_Level_Trace]);
			}
		}

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_extra_box_flags(UI_BoxFlag_DrawBorder);
		UI_ScrollabelRegion entries_list = ui_push_scrollable_region(str8_lit("LogEntries"));

		// NOTE(simon): It takes one frame for us to get the correct sizes for clamping the scroll, so call it twice.
		if (!state->freeze && state->has_new_entries)
		{
			ui_scrollabel_region_set_scroll(entries_list, F32_MAX);
			--state->has_new_entries;
		}

		ui_push_font(str8_lit("data/fonts/liberation-mono.ttf"));
		ui_push_font_size(11);

		for (U32 i = 0; i < state->entry_count; ++i)
		{
			Log_QueueEntry *entry = &state->entries[i];

			if (!ui_logger_entry_passes_filter(state, entry))
			{
				continue;
			}

			Str8 message = ui_logger_format_entry(state, entry);

			Vec4F32 level_colors[] = {
				[Log_Level_Info]    = v4f32_mul_f32(v4f32(229.0f, 229.0f, 229.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Warning] = v4f32_mul_f32(v4f32(229.0f, 227.0f, 91.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Error]   = v4f32_mul_f32(v4f32(229.0f, 100.0f, 91.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Trace]   = v4f32_mul_f32(v4f32(121.4f, 229.0f, 91.4f, 255.0f), 1.0f / 255.0f),
			};

			ui_next_text_color(level_colors[entry->level]);
			UI_Box *log_entry = ui_box_make(
				UI_BoxFlag_DrawText |
				UI_BoxFlag_HotAnimation |
				UI_BoxFlag_ActiveAnimation |
				UI_BoxFlag_Clickable,
				str8_pushf(ui_frame_arena(), "LogEntry%p", entry)
			);
			ui_box_equip_display_string(log_entry, message);
			UI_Comm entry_comm = ui_comm_from_box(log_entry);

			if (entry_comm.hovering)
			{
				log_entry->flags |= UI_BoxFlag_DrawBackground;
			}
			if (entry_comm.clicked)
			{
				arena_scratch(0, 0)
				{
					Str8List arguments = { 0 };
					str8_list_push(scratch, &arguments, str8_cstr(entry->file));
					str8_list_push(scratch, &arguments, str8_pushf(scratch, "%"PRIU32, entry->line));

#if OS_WINDOWS
					os_run(str8_lit("./script/log_open.bat"), arguments);
#elif OS_LINUX
					os_run(str8_lit("./script/log_open.sh"), arguments);
#endif
				}
			}
		}

		ui_pop_font();
		ui_pop_font_size();
		ui_pop_scrollable_region();
	}
}

internal Void
ui_debug_keep_alive(U32 frame_index)
{
	frame_index %= DEBUG_STAT_FRAMES;
	Debug_TimeBuffer *buffer = debug_get_times();
	ui_debug_memory = debug_get_memory();

	if (ui_debug_freeze)
	{
		return;
	}

	ui_debug_stats.latest_frame_index = frame_index;
	for (U32 stat_index = 0; stat_index < DEBUG_STAT_LOCATIONS; ++stat_index)
	{
		ui_debug_stats.total_time_ns[frame_index][stat_index] = 0;
		ui_debug_stats.hit_count[frame_index][stat_index]     = 0;
	}

	for (U32 i = 0; i < buffer->count; ++i)
	{
		Debug_TimeEntry *entry = &buffer->buffer[i];

		U32 stat_index = 0;
		for (; stat_index < ui_debug_stats.count; ++stat_index)
		{
			if (ui_debug_stats.file[stat_index] == entry->file && ui_debug_stats.line[stat_index] == entry->line)
			{
				break;
			}
		}

		if (stat_index == ui_debug_stats.count)
		{
			if (stat_index < DEBUG_STAT_LOCATIONS)
			{
				ui_debug_stats.file[stat_index] = entry->file;
				ui_debug_stats.line[stat_index] = entry->line;
				ui_debug_stats.name[stat_index] = str8_cstr(entry->name);

				U64 hash = hash_str8(ui_debug_stats.name[stat_index]);
				ui_debug_stats.colors[stat_index].rgb = rgb_from_hsv(v3f32((F32) hash / (F32) U16_MAX, 1, 1));
				ui_debug_stats.colors[stat_index].a   = 1.0f;

				ui_debug_stats.total_time_ns[frame_index][stat_index] = (U32) (entry->end_ns - entry->start_ns);
				ui_debug_stats.hit_count[frame_index][stat_index]     = 1;
				++ui_debug_stats.count;
			}
		}
		else
		{
			ui_debug_stats.total_time_ns[frame_index][stat_index] += (U32) (entry->end_ns - entry->start_ns);
			++ui_debug_stats.hit_count[frame_index][stat_index];
		}
	}

	for (U32 i = 0; i < debug_arena_count; ++i)
	{
		debug_arenas[i].change_count = 0;
	}
	for (U32 i = 0; ui_debug_memory && i < ui_debug_memory->count; ++i)
	{
		Debug_MemoryEntry *entry = &ui_debug_memory->buffer[i];
		U32 stat_index = 0;
		for (; stat_index < debug_arena_count; ++stat_index)
		{
			if (debug_arenas[stat_index].arena == entry->arena)
			{
				break;
			}
		}

		if (stat_index == debug_arena_count)
		{
			if (stat_index < array_count(debug_arenas) && entry->position != DEBUG_MEMORY_DELETED)
			{
				debug_arenas[stat_index].arena        = entry->arena;
				debug_arenas[stat_index].current      = entry->position;
				debug_arenas[stat_index].max          = entry->position;
				debug_arenas[stat_index].change_count = 1;
				++debug_arena_count;
			}
		}
		else if (entry->position == DEBUG_MEMORY_DELETED)
		{
			debug_arenas[stat_index] = debug_arenas[--debug_arena_count];
		}
		else
		{
			debug_arenas[stat_index].current = entry->position;
			debug_arenas[stat_index].max     = u64_max(debug_arenas[stat_index].max, entry->position);
			++debug_arenas[stat_index].change_count;
		}
	}
}

internal Void
framed_ui_setup_percentage_sort_columns(Str8 *column_names, F32 *splits, UI_Box **columns, U32 column_count, U32 *sort_column, B32 *reverse)
{
	F32 drag_delta = 0.0f;
	U32 drag_index = 0;
	ui_corner_radius(0)
		ui_color(v4f32(0.4f, 0.4f, 0.4f, 1.0f))
	{
		for (U32 i = 0; i < column_count; ++i)
		{
			ui_next_width(ui_pct(splits[i], 0));
			ui_next_extra_box_flags(UI_BoxFlag_Clip);
			columns[i] = ui_named_column_beginf("column%"PRIU32, i);
			ui_push_seed(columns[i]->key);

			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_children_sum(1));
			ui_next_child_layout_axis(Axis2_X);

			UI_Box *header = ui_box_make(
				UI_BoxFlag_Clickable |
				UI_BoxFlag_HotAnimation |
				UI_BoxFlag_ActiveAnimation,
				str8_lit("Header")
			);

			UI_Comm header_comm = ui_comm_from_box(header);
			if (header_comm.clicked)
			{
				*reverse = (i == *sort_column ? !*reverse : false);
				*sort_column = i;
			}

			if (header_comm.hovering)
			{
				header->flags |= UI_BoxFlag_DrawBackground;
			}

			ui_parent(header)
			{
				ui_text(column_names[i]);

				ui_spacer(ui_fill());

				if (i == *sort_column)
				{
					ui_next_width(ui_em(1, 1));
					ui_next_height(ui_em(1, 1));
					ui_next_icon(*reverse ? RENDER_ICON_UP : RENDER_ICON_DOWN);
					ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
					ui_spacer(ui_em(0.2f, 1));
				}
			}

			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_em(0.2f, 1));
			ui_next_color(v4f32(0.4f, 0.4f, 0.4f, 1.0f));
			ui_next_corner_radius(0);
			ui_box_make(UI_BoxFlag_DrawBackground, str8_lit(""));

			ui_pop_seed();
			ui_named_column_end();

			if (i + 1 < column_count)
			{
				ui_next_width(ui_em(0.2f, 1));
				ui_next_height(ui_pixels(columns[i]->fixed_size.height, 1));
				ui_next_hover_cursor(Gfx_Cursor_SizeWE);
				UI_Box *draggable_box = ui_box_make(
					UI_BoxFlag_Clickable |
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
}

UI_CUSTOM_DRAW_PROC(time_graph_custom_draw)
{
	Render_Context *renderer = ui_renderer();

	UI_RectStyle *rect_style = &root->rect_style;
	UI_TextStyle *text_style = &root->text_style;

	if (ui_box_has_flag(root, UI_BoxFlag_DrawDropShadow))
	{
		Vec2F32 min = v2f32_sub_v2f32(root->fixed_rect.min, v2f32(10, 10));
		Vec2F32 max = v2f32_add_v2f32(root->fixed_rect.max, v2f32(15, 15));
		// TODO(hampus): Make softness em dependent
		Render_RectInstance *instance = render_rect(
			renderer, min, max,
			.softness = 15,
			.color = v4f32(0, 0, 0, 1)
		);
		memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBackground))
	{
		F32 total = 16e6f;

		RectF32 rect = root->fixed_rect;
		F32 rect_width = rect.max.x - rect.min.x;
		F32 rect_height = rect.max.y - rect.min.y;
		F32 x = rect.min.x;
		F32 width = rect_width / (F32) DEBUG_STAT_FRAMES;
		for (U32 frame_index = 0; frame_index < DEBUG_STAT_FRAMES; ++frame_index)
		{
			F32 y = rect.max.y;

			for (U32 i = 0; i < ui_debug_stats.count; ++i)
			{
				F32 height = rect_height * (F32) ui_debug_stats.total_time_ns[frame_index][i] / (F32) total;

				Vec4F32 color = ui_debug_stats.colors[i];
				render_rect(
					renderer,
					v2f32(f32_floor(x), f32_floor(y - height)),
					v2f32(f32_floor(x + width), f32_floor(y)),
					.color = color
				);

				y -= height;
			}

			x += width;
		}
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBorder))
	{
		Render_RectInstance *instance = render_rect(
			renderer, root->fixed_rect.min, root->fixed_rect.max,
			.border_thickness = rect_style->border_thickness,
			.color = rect_style->border_color
		);
	}
}

internal Void
ui_debug(Void *data)
{
	UI_ColorPickerData *color_picker_data = data;

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_scrollable_region_axis(str8_lit("DebugScroll"), Axis2_Y)
	{
		ui_spacer(ui_em(0.5f, 1));
		local U8 text_buffer[256] = { 0 };
		local UI_TextEditState edit_state = { 0 };
		local U64 string_length = 0;
		ui_next_width(ui_em(30, 1));
		ui_line_edit(&edit_state, text_buffer, array_count(text_buffer), &string_length, str8_lit("LineEditTest"));

		ui_spacer(ui_em(0.5f, 1));

		ui_row()
		{
			ui_spacer(ui_em(0.4f, 1));
			ui_check(&ui_debug_freeze, str8_lit("DebugFreeze"));
			ui_spacer(ui_em(0.4f, 1));
			ui_text(str8_lit("Freeze"));
		}

		ui_spacer(ui_em(0.4f, 1));

		ui_next_width(ui_fill());
		ui_named_row(str8_lit("DebugTimes"))
		{
			Str8 headers[] =
			{
				str8_lit("Name"),
				str8_lit("Total time"),
				str8_lit("Avg. time / hit"),
				str8_lit("Hit count"),
			};

			local F32 splits[] = { 0.25f, 0.25f, 0.25f, 0.25f };
			UI_Box *columns[4] = { 0 };
			local B32 reverse = true;
			local U32 sort_column = 1;
			framed_ui_setup_percentage_sort_columns(headers, splits, columns, 4, &sort_column, &reverse);

			U64 frame_index = ui_debug_stats.latest_frame_index;

			U32 time_index[DEBUG_STAT_LOCATIONS];
			for (U32 i = 0; i < DEBUG_STAT_LOCATIONS; ++i)
			{
				time_index[i] = i;
			}

			// NOTE(simon): Bubble sort for the win!
			if (sort_column == 0)
			{
				for (U32 i = 0; i < ui_debug_stats.count; ++i)
				{
					for (U32 j = 0; j < ui_debug_stats.count - i - 1; ++j)
					{
						Str8 this_name = ui_debug_stats.name[time_index[j + 0]];
						Str8 next_name = ui_debug_stats.name[time_index[j + 1]];
						if (str8_are_codepoints_earliear(next_name, this_name))
						{
							swap(time_index[j], time_index[j + 1], U32);
						}
					}
				}
			}
			else if (sort_column == 1)
			{
				for (U32 i = 0; i < ui_debug_stats.count; ++i)
				{
					for (U32 j = 0; j < ui_debug_stats.count - i - 1; ++j)
					{
						U64 this_value = ui_debug_stats.total_time_ns[frame_index][time_index[j + 0]];
						U64 next_value = ui_debug_stats.total_time_ns[frame_index][time_index[j + 1]];
						if (this_value > next_value)
						{
							swap(time_index[j], time_index[j + 1], U32);
						}
					}
				}
			}
			else if (sort_column == 2)
			{
				for (U32 i = 0; i < ui_debug_stats.count; ++i)
				{
					for (U32 j = 0; j < ui_debug_stats.count - i - 1; ++j)
					{
						F64 this_avg_time = (F64) ui_debug_stats.total_time_ns[frame_index][time_index[j + 0]] / (F64) ui_debug_stats.hit_count[frame_index][time_index[j + 0]];
						F64 next_avg_time = (F64) ui_debug_stats.total_time_ns[frame_index][time_index[j + 1]] / (F64) ui_debug_stats.hit_count[frame_index][time_index[j + 1]];
						if (this_avg_time > next_avg_time)
						{
							swap(time_index[j], time_index[j + 1], U32);
						}
					}
				}
			}
			else if (sort_column == 3)
			{
				for (U32 i = 0; i < ui_debug_stats.count; ++i)
				{
					for (U32 j = 0; j < ui_debug_stats.count - i - 1; ++j)
					{
						U64 this_value = ui_debug_stats.hit_count[frame_index][time_index[j + 0]];
						U64 next_value = ui_debug_stats.hit_count[frame_index][time_index[j + 1]];
						if (this_value > next_value)
						{
							swap(time_index[j], time_index[j + 1], U32);
						}
					}
				}
			}

			if (reverse)
			{
				for (U32 i = 0, j = ui_debug_stats.count; i < j; ++i, --j)
				{
					swap(time_index[i], time_index[j - 1], U32);
				}
			}

			UI_Key debug_time_color_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("DebugTimeColorCtxMenu"));

			local Vec4F32 *selected_color = 0;
			ui_ctx_menu(debug_time_color_ctx_menu)
			{
				color_picker_data->rgba = selected_color;
				ui_color_picker(color_picker_data);
			}

			ui_parent(columns[0])
			{
				for (U32 i = 0; i < ui_debug_stats.count; ++i)
				{
					ui_row()
					{
						ui_spacer(ui_em(0.1f, 1));
						ui_column()
						{
							ui_spacer(ui_em(0.1f, 1));
							ui_next_color(ui_debug_stats.colors[time_index[i]]);
							ui_next_hover_cursor(Gfx_Cursor_Hand);
							ui_next_corner_radius(5);
							ui_next_width(ui_em(0.8f, 1));
							ui_next_height(ui_em(0.8f, 1));
							UI_Box *box = ui_box_make(
								UI_BoxFlag_DrawBackground |
								UI_BoxFlag_DrawBorder |
								UI_BoxFlag_Clickable |
								UI_BoxFlag_HotAnimation |
								UI_BoxFlag_ActiveAnimation,
								ui_debug_stats.name[time_index[i]]
							);
							UI_Comm comm = ui_comm_from_box(box);
							if (comm.clicked)
							{
								ui_ctx_menu_open(box->key, v2f32(0, 0), debug_time_color_ctx_menu);
								selected_color = &ui_debug_stats.colors[time_index[i]];
							}
						}
						ui_spacer(ui_em(0.1f, 1));

						ui_next_text_padding(Axis2_X, 0);
						ui_text(ui_debug_stats.name[time_index[i]]);
					}
				}
			}
			ui_parent(columns[1])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < ui_debug_stats.count; ++i)
					{
						TimeInterval total_time = time_interval_from_ns((F64) ui_debug_stats.total_time_ns[frame_index][time_index[i]]);
						ui_textf("%.2f%"PRISTR8, total_time.amount, str8_expand(total_time.unit));
					}
				}
			}
			ui_parent(columns[2])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < ui_debug_stats.count; ++i)
					{
						U32 total_time_ns = ui_debug_stats.total_time_ns[frame_index][time_index[i]];
						U32 hit_count = ui_debug_stats.hit_count[frame_index][time_index[i]];
						TimeInterval average_time = time_interval_from_ns((F64) total_time_ns / (F64) hit_count);
						ui_textf("%.2f%"PRISTR8, average_time.amount, str8_expand(average_time.unit));
					}
				}
			}
			ui_parent(columns[3])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < ui_debug_stats.count; ++i)
					{
						ui_textf("%"PRIU32, ui_debug_stats.hit_count[frame_index][time_index[i]]);
					}
				}
			}
		}

		ui_spacer(ui_em(0.5f, 1));

		ui_next_width(ui_fill());
		ui_next_height(ui_em(5, 1));
		UI_Box *time_graph = ui_box_make(UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawBackground, str8_lit(""));
		ui_box_equip_custom_draw_proc(time_graph, time_graph_custom_draw);

		ui_spacer(ui_em(0.5f, 1));
		ui_text(str8_lit("Arena Stats"));
		ui_spacer(ui_em(0.5f, 1));

		U64 total_allocated = 0;
		for (U32 i = 0; i < debug_arena_count; ++i)
		{
			total_allocated += debug_arenas[i].current;
		}
		MemorySize total_size = memory_size_from_bytes(total_allocated);
		ui_textf("Total allocated: %.2f%"PRISTR8, total_size.amount, str8_expand(total_size.unit));

		ui_spacer(ui_em(0.5f, 1));

		ui_next_width(ui_fill());
		ui_named_row(str8_lit("DebugMemory"))
		{
			ui_push_seed(ui_top_parent()->key);
			Str8 headers[] = {
				str8_lit("Arena"),
				str8_lit("Max / frame"),
				str8_lit("End of frame"),
				str8_lit("Changes / frame"),
			};

			local F32 splits[] = { 0.25f, 0.25f, 0.25f, 0.25f };
			UI_Box *columns[4] = { 0 };
			local B32 reverse = true;
			local U32 sort_column = 1;
			framed_ui_setup_percentage_sort_columns(headers, splits, columns, 4, &sort_column, &reverse);

			// NOTE(simon): Bubble sort for the win!
			if (sort_column == 0)
			{
				for (U32 i = 0; i < debug_arena_count; ++i)
				{
					for (U32 j = 0; j < debug_arena_count - i - 1; ++j)
					{
						Str8 this_name = debug_arenas[j + 0].arena->name;
						Str8 next_name = debug_arenas[j + 1].arena->name;
						if (str8_are_codepoints_earliear(next_name, this_name))
						{
							swap(debug_arenas[j], debug_arenas[j + 1], DebugMemoryStatistics);
						}
					}
				}
			}
			else if (sort_column == 1)
			{
				for (U32 i = 0; i < debug_arena_count; ++i)
				{
					for (U32 j = 0; j < debug_arena_count - i - 1; ++j)
					{
						if (debug_arenas[j].max > debug_arenas[j + 1].max)
						{
							swap(debug_arenas[j], debug_arenas[j + 1], DebugMemoryStatistics);
						}
					}
				}
			}
			else if (sort_column == 2)
			{
				for (U32 i = 0; i < debug_arena_count; ++i)
				{
					for (U32 j = 0; j < debug_arena_count - i - 1; ++j)
					{
						if (debug_arenas[j].current > debug_arenas[j + 1].current)
						{
							swap(debug_arenas[j], debug_arenas[j + 1], DebugMemoryStatistics);
						}
					}
				}
			}
			else if (sort_column == 3)
			{
				for (U32 i = 0; i < debug_arena_count; ++i)
				{
					for (U32 j = 0; j < debug_arena_count - i - 1; ++j)
					{
						if (debug_arenas[j].change_count > debug_arenas[j + 1].change_count)
						{
							swap(debug_arenas[j], debug_arenas[j + 1], DebugMemoryStatistics);
						}
					}
				}
			}

			if (reverse)
			{
				for (U32 i = 0, j = debug_arena_count; i < j; ++i, --j)
				{
					swap(debug_arenas[i], debug_arenas[j - 1], DebugMemoryStatistics);
				}
			}

			ui_parent(columns[0])
			{
				for (U32 i = 0; i < debug_arena_count; ++i)
				{
					DebugMemoryStatistics *entry = &debug_arenas[i];
					ui_text(entry->arena->name);
				}
			}

			ui_parent(columns[1])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < debug_arena_count; ++i)
					{
						DebugMemoryStatistics *entry = &debug_arenas[i];
						MemorySize max = memory_size_from_bytes(debug_arenas[i].max);
						ui_textf("%.2f%"PRISTR8, max.amount, str8_expand(max.unit));
					}
				}
			}

			ui_parent(columns[2])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < debug_arena_count; ++i)
					{
						DebugMemoryStatistics *entry = &debug_arenas[i];
						MemorySize current = memory_size_from_bytes(debug_arenas[i].current);
						ui_textf("%.2f%"PRISTR8, current.amount, str8_expand(current.unit));
					}
				}
			}

			ui_parent(columns[3])
			{
				ui_width(ui_fill())
					ui_text_align(UI_TextAlign_Right)
				{
					for (U32 i = 0; i < debug_arena_count; ++i)
					{
						DebugMemoryStatistics *entry = &debug_arenas[i];
						ui_textf("%"PRIU64, entry->change_count);
					}
				}
			}
			ui_pop_seed();
		}

		ui_spacer(ui_em(0.5f, 1));
		ui_text(str8_lit("UI Stats"));
		ui_spacer(ui_em(0.5f, 1));
		ui_textf("Permanent arena: %"PRIU64"kB", ui_permanent_arena()->pos/1024);
		ui_textf("Frame arena: %"PRIU64"kB", ui_frame_arena()->pos/1024);

#if UI_GATHER_STATS
		UI_Stats *ui_stats = ui_get_prev_stats();
		ui_textf("Num hashed boxes: %"PRIU64, ui_stats->num_hashed_boxes);
		ui_textf("Num transient boxes: %"PRIU64, ui_stats->num_transient_boxes);
		ui_textf("Rect style push count: %"PRIU64, ui_stats->rect_style_push_count);
		ui_textf("Layout style push count: %"PRIU64, ui_stats->layout_style_push_count);
		ui_textf("Text style push count: %"PRIU64, ui_stats->text_style_push_count);
		ui_textf("Parent push count: %"PRIU64, ui_stats->parent_push_count);
		ui_textf("Seed push count: %"PRIU64, ui_stats->seed_push_count);
		ui_textf("Total chain count: %"PRIU64, ui_stats->box_chain_count);
		ui_textf("Max box chain count: %"PRIU64, ui_stats->max_box_chain_count);
#endif
	}
}

global B32 texture_view_ui_filtering_mode = Render_TextureFilter_Nearest;

internal Void
ui_texture_view(Render_TextureSlice atlas)
{
	ui_next_child_layout_axis(Axis2_X);
	UI_Box *texture_viewer = ui_box_make(0, str8_lit(""));

	ui_parent(texture_viewer)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_color(v4f32(0.5, 0.5, 0.5, 1));
		UI_Box *atlas_parent = ui_box_make(
			UI_BoxFlag_DrawBackground |
			UI_BoxFlag_Clip |
			UI_BoxFlag_Clickable |
			UI_BoxFlag_ViewScroll,
			str8_lit("TextureViewer")
		);
		ui_parent(atlas_parent)
		{
			local Vec2F32 offset = { 0 };
			local F32 scale = 1;

			ui_next_relative_pos(Axis2_X, offset.x);
			ui_next_relative_pos(Axis2_Y, offset.y);
			ui_next_width(ui_pct(1.0f / scale, 1));
			ui_next_height(ui_pct(1.0f / scale, 1));

			ui_next_texture_filter(texture_view_ui_filtering_mode);
			ui_next_slice(atlas);
			ui_next_color(v4f32(1, 1, 1, 1));

			ui_next_corner_radius(0);
			UI_Box *atlas_box = ui_box_make(
				UI_BoxFlag_FloatingPos |
				UI_BoxFlag_DrawBackground,
				str8_lit("Texture")
			);

			UI_Comm atlas_comm = ui_comm_from_box(atlas_parent);

			F32 old_scale = scale;
			scale *= f32_pow(2, atlas_comm.scroll.y * 0.1f);

			// TODO(simon): Slightly broken math, but mostly works.
			Vec2F32 scale_offset = v2f32_mul_f32(v2f32_sub_v2f32(atlas_comm.rel_mouse, offset), scale / old_scale - 1.0f);
			offset = v2f32_add_v2f32(v2f32_sub_v2f32(offset, atlas_comm.drag_delta), scale_offset);
		}

		ui_column()
		{
			Str8 names[Render_TextureFilter_COUNT] =
			{
				[Render_TextureFilter_Bilinear] = str8_lit("Bilinear"),
				[Render_TextureFilter_Nearest]  = str8_lit("Nearest"),
			};

			ui_combo_box(str8_lit("Filtering:"), &texture_view_ui_filtering_mode, names, array_count(names));
		}
	}
}
