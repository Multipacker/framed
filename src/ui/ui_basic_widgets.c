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
	Arena_Temporary scratch = get_scratch(0, 0);
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(scratch.arena, fmt, args);
	comm = ui_text(string);
	va_end(args);
	release_scratch(scratch);
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
	Arena_Temporary scratch = get_scratch(0, 0);
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(scratch.arena, fmt, args);
	comm = ui_button(string);
	va_end(args);
	release_scratch(scratch);
	return(comm);
}
