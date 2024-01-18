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
ui_image(Render_TextureSlice slice, Str8 string)
{
	ui_next_slice(slice);
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground,
							  string);
	UI_Comm comm = ui_comm_from_box(box);
	return(comm);
}

internal UI_Comm
ui_imagef(Render_TextureSlice slice, CStr fmt, ...)
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
			ui_next_icon(RENDER_ICON_CHECK);
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
	ui_next_child_layout_axis(Axis2_X);
	UI_Box *vert_container = ui_box_make(UI_BoxFlag_ViewScroll,
	                                     string);
	ui_push_parent(vert_container);

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	UI_Box *horz_container = ui_box_make(0, str8_lit("HorzContainer"));

	ui_push_parent(horz_container);

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	UI_Box *view_region = ui_box_make(UI_BoxFlag_Clip, str8_lit("ViewRegion"));

	ui_push_parent(view_region);

	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	UI_Box *content = ui_box_make(smooth_scroll ? UI_BoxFlag_AnimatePos : 0,
	                              str8_lit("ScrollContent"));
	ui_push_parent(content);

	UI_ScrollabelRegion result;
	result.view_region = view_region;
	result.content     = content;
	return(result);
}

internal Void
ui_scrollabel_region_set_scroll(UI_ScrollabelRegion region, F32 offset)
{
	UI_Box *content     = region.content;
	UI_Box *view_region = region.view_region;
	content->scroll.y   = f32_clamp(0, offset, content->fixed_size.height - view_region->fixed_size.height);
}

internal Void
ui_pop_scrollable_region(Void)
{
	UI_Box *content        = ui_pop_parent();
	UI_Box *view_region    = ui_pop_parent();
	UI_Box *horz_container = ui_pop_parent();
	UI_Box *vert_container = ui_pop_parent();

	ui_push_parent(vert_container);
	ui_push_parent(horz_container);

	for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis)
	{
		UI_Box *parent = ui_top_parent();
		ui_push_string(parent->string);
		F32 percent_in_view = view_region->fixed_size.v[axis] / content->fixed_size.v[axis];
		if (percent_in_view < 1.0f)
		{
			F32 size = parent->fixed_size.v[axis] * percent_in_view;
			F32 max_pos  = parent->fixed_size.v[axis] - size;
			F32 max_scroll = content->fixed_size.v[axis] - view_region->fixed_size.v[axis];
			F32 percent_scrolled = content->scroll.v[axis] / max_scroll;
			F32 pos = max_pos * percent_scrolled;

			ui_next_relative_pos(axis, pos);
			ui_next_color(v4f32(0.5f, 0.5f, 0.5f, 1));
			ui_next_size(axis_flip(axis), ui_em(0.4f, 1));
			ui_next_size(axis, ui_pixels(size, 1));
			UI_Box *scrollbar = ui_box_make(UI_BoxFlag_DrawBackground |
			                                (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis) |
			                                UI_BoxFlag_HotAnimation |
			                                UI_BoxFlag_Clickable |
			                                (UI_BoxFlags) (UI_BoxFlag_AnimateWidth << axis) |
			                                UI_BoxFlag_ActiveAnimation,
			                                str8_lit("Scrollbar")
			                                );

			UI_Comm scrollbar_comm = ui_comm_from_box(scrollbar);
			if (scrollbar_comm.dragging)
			{
				content->scroll.v[axis] -= scrollbar_comm.drag_delta.v[axis] / max_pos * max_scroll;
			}
			else
			{
				scrollbar->flags |= (UI_BoxFlags) (UI_BoxFlag_AnimateX << axis);
			}
		}

		ui_pop_string();
		ui_pop_parent();
	}

	ui_pop_string();

	UI_Comm comm = ui_comm_from_box(vert_container);
	for (Axis2 axis = 0; axis < Axis2_COUNT; ++axis)
	{
		content->scroll.v[axis] += (F32)(comm.scroll.v[axis] * g_ui_ctx->dt * 5000.0);
		content->scroll.v[axis]  = f32_clamp(0, content->scroll.v[axis], content->fixed_size.v[axis] - view_region->fixed_size.v[axis]);
	}
}

internal Void
ui_spacer(UI_Size size)
{
	ui_next_size(ui_top_parent()->layout_style.child_layout_axis, size);
	ui_next_size(axis_flip(ui_top_parent()->layout_style.child_layout_axis), ui_pixels(0, 0));
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
ui_named_row_beginfv(CStr fmt, va_list args)
{
	UI_Comm comm = {0};
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	UI_Box *box = ui_named_row_begin(string);
	return(box);
}

internal UI_Box *
ui_named_row_beginf(CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	UI_Box *box = ui_named_row_beginfv(fmt, args);
	va_end(args);
	return(box);
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
ui_named_column_beginfv(CStr fmt, va_list args)
{
	UI_Comm comm = {0};
	Str8 string = str8_pushfv(ui_frame_arena(), fmt, args);
	UI_Box *box = ui_named_column_begin(string);
	return(box);
}

internal UI_Box *
ui_named_column_beginf(CStr fmt, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, fmt);
	UI_Box *box = ui_named_column_beginfv(fmt, args);
	va_end(args);
	return(box);
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

typedef struct UI_ComboBoxParams UI_ComboBoxParams;
struct UI_ComboBoxParams
{
	UI_Size item_size;
};
#define ui_combo_box(name, selected_index, item_names, item_count, ...) ui_combo_box_internal(name, selected_index, item_names, item_count, &(UI_ComboBoxParams) { 0, __VA_ARGS__ });

// TODO(simon): Maybe we only want to return true if the value changes.
internal B32
ui_combo_box_internal(Str8 name, U32 *selected_index, Str8 *item_names, U32 item_count, UI_ComboBoxParams *params)
{
	B32 result = false;

	if (params->item_size.kind == UI_SizeKind_Null)
	{
		F32 largest_width = 0.0f;
		Render_Font *font = render_font_from_key(g_ui_ctx->renderer, ui_top_text_style()->font);
		for (U32 i = 0; i < item_count; ++i)
		{
			Vec2F32 size = render_measure_text(font, item_names[i]);
			largest_width = f32_max(largest_width, size.width);
		}
		params->item_size = ui_pixels(largest_width + ui_top_text_style()->padding.width, 1);
	}

	ui_row()
	{
		ui_text(name);

		ui_next_width(ui_children_sum(1));
		ui_next_height(ui_children_sum(1));
		ui_next_child_layout_axis(Axis2_X);
		UI_Box *combo_box = ui_box_make(UI_BoxFlag_DrawBackground |
		                                UI_BoxFlag_DrawBorder |
		                                UI_BoxFlag_HotAnimation |
		                                UI_BoxFlag_ActiveAnimation |
		                                UI_BoxFlag_Clickable,
		                                name);
		UI_Comm comm = ui_comm_from_box(combo_box);

		ui_parent(combo_box)
		{
			UI_Key combo_box_key = ui_key_from_string_f(ui_key_null(), "%"PRISTR8"ComboItems", str8_expand(name));

			ui_next_width(params->item_size);
			ui_next_height(ui_text_content(1));
			ui_next_text_align(UI_TextAlign_Left);
			UI_Box *display = ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
			ui_box_equip_display_string(display, item_names[*selected_index]);

			B32 combo_box_ctx_is_open = ui_key_match(ui_ctx_menu_key(), combo_box_key);
			ui_next_height(ui_em(1, 1));
			ui_next_width(ui_em(1, 1));
			ui_next_font_size(12);
			ui_next_icon(combo_box_ctx_is_open? RENDER_ICON_DOWN_OPEN : RENDER_ICON_LEFT_OPEN);
			ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));

			if (comm.clicked ||
				(combo_box_ctx_is_open && comm.hovering))
			{
				ui_ctx_menu_open(comm.box->key, v2f32(0, 0), combo_box_key);
			}

			ui_ctx_menu(combo_box_key)
				ui_width(ui_pixels(combo_box->fixed_size.width, 1))
				ui_text_align(UI_TextAlign_Left)
			{
				for (U32 i = 0; i < item_count; ++i)
				{
					UI_Box *item = ui_box_make(UI_BoxFlag_DrawText |
					                           UI_BoxFlag_Clickable |
					                           UI_BoxFlag_HotAnimation |
					                           UI_BoxFlag_ActiveAnimation,
					                           item_names[i]);
					ui_box_equip_display_string(item, item_names[i]);

					UI_Comm item_comm = ui_comm_from_box(item);
					if (item_comm.hovering)
					{
						item->flags |= UI_BoxFlag_DrawBackground;
					}
					if (item_comm.clicked)
					{
						result = true;
						*selected_index = i;
						ui_ctx_menu_close();
					}
				}
			}
		}
	}

	return(result);
}
