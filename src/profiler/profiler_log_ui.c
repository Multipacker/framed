typedef struct LogUI_Thread LogUI_Thread;
struct LogUI_Thread
{
	LogUI_Thread *next;
	Str8 name;
	B32 show;
};

global LogUI_Thread *log_ui_previous_threads = 0;
global LogUI_Thread *log_ui_current_threads  = 0;
global Log_QueueEntry log_ui_entries[1000];
global U32 log_ui_entry_count = 0;
global U32 log_ui_has_new_entries = 0;
global B32 log_ui_freeze = false;
global B32 log_ui_compact_display = false;
global B32 log_ui_level_fillters[Log_Level_COUNT] = { 0 };

internal Void
ui_log_keep_alive(Arena *frame_arena)
{
	if (!log_ui_freeze)
	{
		Log_EntryBuffer new_entries = *log_get_new_entries();

		if (log_ui_entry_count + new_entries.count > array_count(log_ui_entries))
		{
			U32 entries_to_keep     = (U32) s64_max(0, (S64) array_count(log_ui_entries) - (S64) new_entries.count);
			U32 first_entry_to_keep = log_ui_entry_count - entries_to_keep;
			memory_move(log_ui_entries, &log_ui_entries[first_entry_to_keep], entries_to_keep * sizeof(*log_ui_entries));
			log_ui_entry_count = entries_to_keep;
		}

		U32 entries_to_copy     = u32_min(array_count(log_ui_entries) - log_ui_entry_count, new_entries.count);
		U32 first_entry_to_copy = new_entries.count - entries_to_copy;
		memory_copy(&log_ui_entries[log_ui_entry_count], &new_entries.buffer[first_entry_to_copy], entries_to_copy * sizeof(*new_entries.buffer));
		log_ui_entry_count += entries_to_copy;

		// NOTE(simon): It takes one frame for us to get the correct sizes for clamping the scroll, so call it twice.
		log_ui_has_new_entries |= (entries_to_copy != 0 ? 2 : 0);
	}

	// NOTE(simon): Find threads.
	log_ui_current_threads = 0;
	for (U32 i = 0; i < log_ui_entry_count; ++i)
	{
		Str8 thread_name = str8_cstr((CStr) log_ui_entries[i].thread_name);

		B32 unique = true;
		for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
		{
			if (str8_equal(thread_name, thread->name))
			{
				unique = false;
				break;
			}
		}

		if (unique)
		{
			LogUI_Thread *thread = push_struct(frame_arena, LogUI_Thread);
			thread->name = thread_name;
			thread->show = false;
			stack_push(log_ui_current_threads, thread);
		}
	}

	// NOTE(simon): Copy over state from previous frame.
	for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
	{
		for (LogUI_Thread *old_thread = log_ui_previous_threads; old_thread; old_thread = old_thread->next)
		{
			if (str8_equal(thread->name, old_thread->name))
			{
				thread->show = old_thread->show;
				break;
			}
		}
	}

	log_ui_previous_threads = log_ui_current_threads;
}

internal Void
ui_logger(Void)
{
	Render_FontKey mono = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 11);

	ui_next_child_layout_axis(Axis2_X);

	UI_Box *log_window = ui_box_make(0, str8_lit("LogWindow"));

	ui_parent(log_window)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_extra_box_flags(UI_BoxFlag_DrawBorder);
		UI_ScrollabelRegion entries_list = ui_push_scrollable_region(str8_lit("LogEntries"));

		// NOTE(simon): It takes one frame for us to get the correct sizes for clamping the scroll, so call it twice.
		if (!log_ui_freeze && log_ui_has_new_entries)
		{
			ui_scrollabel_region_set_scroll(entries_list, F32_MAX);
			--log_ui_has_new_entries;
		}

		ui_push_font(mono);

		B32 show_all_threads = true;
		for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
		{
			show_all_threads &= !thread->show;
		}

		B32 show_all_levels = true;
		for (Log_Level level = 0; level < Log_Level_COUNT; ++level)
		{
			show_all_levels &= !log_ui_level_fillters[level];
		}

		for (U32 i = 0; i < log_ui_entry_count; ++i)
		{
			Log_QueueEntry *entry = &log_ui_entries[i];

			B32 show = true;
			if (!show_all_threads)
			{
				Str8 thread_name = str8_cstr((CStr) entry->thread_name);
				for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
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
				show &= log_ui_level_fillters[entry->level];
			}

			if (!show)
			{
				continue;
			}

			Str8 message = { 0 };
			if (log_ui_compact_display)
			{
				Str8 path_display = str8_cstr(entry->file);
				U64 slash_index = 0;
				if (str8_last_index_of(path_display, PATH_SEPARATOR, &slash_index))
				{
					++slash_index;
				}
				path_display = str8_skip(path_display, slash_index);

				message = str8_pushf(ui_frame_arena(),
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

			Vec4F32 level_colors[] = {
				[Log_Level_Info]    = v4f32_mul_f32(v4f32(229.0f, 229.0f, 229.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Warning] = v4f32_mul_f32(v4f32(229.0f, 227.0f,  91.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Error]   = v4f32_mul_f32(v4f32(229.0f, 100.0f,  91.0f, 255.0f), 1.0f / 255.0f),
				[Log_Level_Trace]   = v4f32_mul_f32(v4f32(121.4f, 229.0f,  91.4f, 255.0f), 1.0f / 255.0f),
			};

			ui_next_text_color(level_colors[entry->level]);
			UI_Box *log_entry = ui_box_make(UI_BoxFlag_DrawText |
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
		ui_pop_scrollable_region();

		ui_next_width(ui_em(10, 1));
		ui_next_height(ui_fill());
		ui_push_scrollable_region(str8_lit("LogControls"));
		ui_column()
		{
			ui_spacer(ui_em(0.4f, 1));

			ui_row()
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_check(&log_ui_freeze, str8_lit("LogFreeze"));
				ui_spacer(ui_em(0.4f, 1));
				ui_text(str8_lit("Freeze entries"));
			}

			ui_spacer(ui_em(0.4f, 1));

			ui_row()
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_check(&log_ui_compact_display, str8_lit("LogCompact"));
				ui_spacer(ui_em(0.4f, 1));
				ui_text(str8_lit("Compact display"));
			}

			ui_spacer(ui_em(0.4f, 1));

			ui_text(str8_lit("Threads:"));

			for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_row()
				{
					ui_spacer(ui_em(0.8f, 1));
					ui_check(&thread->show, thread->name);
					ui_spacer(ui_em(0.4f, 1));
					ui_text(thread->name);
				}
			}

			ui_spacer(ui_em(0.4f, 1));

			ui_text(str8_lit("Level filter:"));

			Str8 level_names[] = {
				[Log_Level_Info]    = str8_lit("Infos"),
				[Log_Level_Warning] = str8_lit("Warnings"),
				[Log_Level_Error]   = str8_lit("Errors"),
				[Log_Level_Trace]   = str8_lit("Traces"),
			};
			for (Log_Level level = 0; level < Log_Level_COUNT; ++level)
			{
				ui_spacer(ui_em(0.4f, 1));
				ui_row()
				{
					B32 test = false;
					ui_spacer(ui_em(0.8f, 1));
					ui_check(&log_ui_level_fillters[level], level_names[level]);
					ui_spacer(ui_em(0.4f, 1));
					ui_text(level_names[level]);
				}
			}
		}
		ui_pop_scrollable_region();
	}
}
