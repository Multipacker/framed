typedef struct LogUI_Thread LogUI_Thread;
struct LogUI_Thread
{
	LogUI_Thread *next;
	Str8 name;
	B32 show;
};

global LogUI_Thread *log_ui_previous_threads = 0;
global LogUI_Thread *log_ui_current_threads  = 0;
global Log_QueueEntry *log_ui_entries = 0;
global U32 log_ui_entry_count = 0;
global U32 log_ui_previous_entry_count = 0;
global B32 log_ui_freeze = false;

internal Void ui_log_keep_alive(Arena *frame_arena)
{
	if (!log_ui_freeze)
	{
		log_update_entries(1000);
	}

	log_ui_previous_entry_count = log_ui_entry_count;
	log_ui_entries = log_get_entries(&log_ui_entry_count);

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
	R_FontKey mono = render_key_from_font(str8_lit("data/fonts/liberation-mono.ttf"), 7);

	ui_next_child_layout_axis(Axis2_X);

	UI_Box *log_window = ui_box_make(
									 UI_BoxFlag_DrawBackground |
									 UI_BoxFlag_AnimateHeight,
									 str8_lit("LogWindow")
									 );

	ui_parent(log_window)
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_extra_box_flags(UI_BoxFlag_AnimateHeight);
		UI_ScrollabelRegion entries_list = ui_push_scrollable_region(str8_lit("LogEntries"));

		if (!log_ui_freeze && log_ui_entry_count != log_ui_previous_entry_count)
		{
			ui_scrollabel_region_set_scroll(entries_list, F32_MAX);
		}

		ui_push_font(mono);

		local B32 only_info = false;

		U32 entry_count = 0;
		Log_QueueEntry *entries = log_get_entries(&entry_count);

		B32 all_unselected = true;
		for (LogUI_Thread *thread = log_ui_current_threads; thread; thread = thread->next)
		{
			all_unselected &= !thread->show;
		}

		for (U32 i = 0; i < log_ui_entry_count; ++i)
		{
			Log_QueueEntry *entry = &log_ui_entries[i];

			B32 show = true;
			if (!all_unselected)
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

			if (!show)
			{
				continue;
			}

			Vec4F32 color = { 0 };
			switch (entry->level)
			{
				case Log_Level_Info:    color = v4f32_mul_f32(v4f32(229.0f, 229.0f, 229.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Warning: color = v4f32_mul_f32(v4f32(229.0f, 227.0f,  91.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Error:   color = v4f32_mul_f32(v4f32(229.0f, 100.0f,  91.0f, 255.0f), 1.0f / 255.0f); break;
				case Log_Level_Trace:   color = v4f32_mul_f32(v4f32(121.4f, 229.0f,  91.4f, 255.0f), 1.0f / 255.0f); break;
				invalid_case;
			}

			// NOTE(simon): Skip the trailing new-line.
			Str8 message = str8_chop(log_format_entry(ui_frame_arena(), entry), 1);
			ui_next_text_color(color);
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
		ui_pop_scrollable_region();

		ui_next_width(ui_children_sum(1));
		ui_next_height(ui_fill());
		ui_next_extra_box_flags(UI_BoxFlag_AnimateHeight);
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
		}
		ui_pop_scrollable_region();
	}
}
