internal UI_Comm
ui_text(Str8 string)
{
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawText,
							  string);
	ui_box_equip_display_string(box, string);
	UI_Comm comm = ui_comm_from_box(box);
	return(comm);
}

internal UI_Comm
ui_textf(CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	comm = ui_text(string);
	va_end(args);
	return(comm);
}

internal UI_Comm
ui_button(Str8 string)
{
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
							  UI_BoxFlag_DrawBorder |
							  UI_BoxFlag_HotAnimation |
							  UI_BoxFlag_ActiveAnimation |
							  UI_BoxFlag_Clickable |
							  UI_BoxFlag_DrawText,
							  string);
	ui_box_equip_display_string(box, string);
	UI_Comm comm = ui_comm_from_box(box);
	return(comm);
}

internal UI_Comm
ui_buttonf(CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	comm = ui_button(string);
	va_end(args);
	return(comm);
}

internal UI_Comm
ui_check(B32 *value, Str8 string)
{
	UI_Comm comm = {0};

	ui_next_width(ui_em(1.0f, 1));
	ui_next_height(ui_em(1.0f, 1));
	UI_Box *container = ui_box_make(UI_BoxFlag_DrawBackground |
									UI_BoxFlag_Clickable |
									UI_BoxFlag_HotAnimation |
									UI_BoxFlag_ActiveAnimation |
									UI_BoxFlag_DrawBorder,
									string);
	comm = ui_comm_from_box(container);
	if (comm.pressed)
	{
		*value = !(*value);
	}
	ui_parent(container)
	{
		if (*value)
		{
			ui_next_icon(R_ICON_CHECK);
		}
		ui_next_width(ui_pct(1.0f, 1));
		ui_next_height(ui_pct(1.0f, 1));
		UI_Box *check = ui_box_make(UI_BoxFlag_DrawText,
									str8_lit(""));
	}

	return(comm);
}

internal UI_Comm
ui_checkf(B32 *value, CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	comm = ui_check(value, string);
	va_end(args);
	return(comm);
}

internal Void
ui_spacer(UI_Size size)
{
	ui_next_size(ui_top_parent()->layout_style.child_layout_axis, size);
	ui_box_make(0, str8_lit(""));
}

internal UI_Box *
ui_begin_named_row(Str8 string)
{
	ui_next_child_layout_axis(Axis2_X);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	UI_Box *box = ui_box_make(0, string);
	ui_push_parent(box);
	return(box);
}

internal Void
ui_end_named_row(Void)
{
	ui_pop_parent();
}

internal UI_Box *
ui_begin_row(Void)
{
	UI_Box *box = ui_begin_named_row(str8_lit(""));
	return(box);
}

internal Void
ui_end_row(Void)
{
	ui_end_named_row();
}

internal UI_Box *
ui_begin_named_column(Str8 string)
{
	ui_next_child_layout_axis(Axis2_Y);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	UI_Box *box = ui_box_make(0, string);
	ui_push_parent(box);
	return(box);
}

internal Void
ui_end_named_column(Void)
{
	ui_pop_parent();
}

internal UI_Box *
ui_begin_column(Void)
{
	UI_Box *box = ui_begin_named_column(str8_lit(""));
	return(box);
}

internal Void
ui_end_column(Void)
{
	ui_end_named_column();
}
