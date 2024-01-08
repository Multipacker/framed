internal Void
ui_default_size(UI_Size width, UI_Size height)
{
	UI_LayoutStyle *layout_style = ui_top_layout_style();
	if (layout_style->size[Axis2_X].kind == UI_SizeKind_Null)
	{
		ui_next_width(width);
	}

	if (layout_style->size[Axis2_Y].kind == UI_SizeKind_Null)
	{
		ui_next_height(height);
	}
}

internal Void
ui_text(Str8 string)
{
	ui_next_width(ui_text_content(1));
	ui_next_height(ui_text_content(1));
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawText,
							  str8_lit(""));
	ui_box_equip_display_string(box, string);
}

internal Void
ui_textf(CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	ui_text(string);
	va_end(args);
}

internal UI_Comm
ui_image(R_TextureSlice slice, Str8 string)
{
	ui_next_slice(slice);
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground,
							  string);
	UI_Comm comm = ui_comm_from_box(box);
	return(comm);
}

internal UI_Comm
ui_imagef(R_TextureSlice slice, CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	comm = ui_image(slice, string);
	va_end(args);
	return(comm);
}

internal UI_Comm
ui_button(Str8 string)
{
	ui_next_hover_cursor(Gfx_Cursor_Hand);
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
	ui_next_hover_cursor(Gfx_Cursor_Hand);
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

internal UI_ScrollabelRegion
ui_push_scrollable_region(Str8 string)
{
	ui_push_string(string);
	B32 smooth_scroll = true;
	UI_Box *view_region = ui_box_make(UI_BoxFlag_Clip |
									  UI_BoxFlag_ViewScroll |
									  UI_BoxFlag_DrawBackground |
									  UI_BoxFlag_DrawBorder,
									  string);
	ui_push_parent(view_region);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	UI_Box *container = ui_box_make(smooth_scroll ? UI_BoxFlag_AnimatePos : 0,
									str8_lit("ScrollContainer"));
	ui_push_parent(container);

	UI_ScrollabelRegion result;
	result.view_region = view_region;
	result.container   = container;
	return(result);
}

internal Void
ui_scrollabel_region_set_scroll(UI_ScrollabelRegion region, F32 offset)
{
	UI_Box *view_region = region.view_region;
	UI_Box *container   = region.container;
	container->scroll.y = f32_clamp(0, offset, container->target_size.v[Axis2_Y] - view_region->target_size.v[Axis2_Y]);
}

internal Void
ui_pop_scrollable_region(Void)
{
	ui_pop_string();
	UI_Box *container   = ui_pop_parent();
	UI_Box *view_region = ui_pop_parent();

	UI_Comm comm = ui_comm_from_box(view_region);
	container->scroll.y += (F32)(comm.scroll.y * g_ui_ctx->dt * 5000.0);
	container->scroll.y = f32_clamp(0, container->scroll.y,
									container->target_size.v[Axis2_Y] - view_region->target_size.v[Axis2_Y]);
}

internal Void
ui_spacer(UI_Size size)
{
	ui_next_size(ui_top_parent()->layout_style.child_layout_axis, size);
	ui_next_size(!ui_top_parent()->layout_style.child_layout_axis, ui_pixels(0, 0));
	ui_box_make(0, str8_lit(""));
}

internal UI_Box *
ui_named_row_begin(Str8 string)
{
	ui_default_size(ui_children_sum(1), ui_children_sum(1));
	ui_next_child_layout_axis(Axis2_X);
	UI_Box *box = ui_box_make(0, string);
	ui_push_parent(box);
	return(box);
}

internal Void
ui_named_row_end(Void)
{
	ui_pop_parent();
}

internal UI_Box *
ui_row_begin(Void)
{
	UI_Box *box = ui_named_row_begin(str8_lit(""));
	return(box);
}

internal Void
ui_row_end(Void)
{
	ui_named_row_end();
}

internal UI_Box *
ui_named_column_begin(Str8 string)
{
	ui_default_size(ui_children_sum(1), ui_children_sum(1));
	ui_next_child_layout_axis(Axis2_Y);
	UI_Box *box = ui_box_make(0, string);
	ui_push_parent(box);
	return(box);
}

internal Void
ui_named_column_end(Void)
{
	ui_pop_parent();
}

internal UI_Box *
ui_column_begin(Void)
{
	UI_Box *box = ui_named_column_begin(str8_lit(""));
	return(box);
}

internal Void
ui_column_end(Void)
{
	ui_named_column_end();
}
