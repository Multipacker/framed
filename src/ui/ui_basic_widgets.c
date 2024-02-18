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
	ui_default_size(ui_text_content(1), ui_text_content(1));
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
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
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground, string);
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
	UI_Box *box = ui_box_make(
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_DrawBorder |
		UI_BoxFlag_HotAnimation |
		UI_BoxFlag_ActiveAnimation |
		UI_BoxFlag_Clickable |
		UI_BoxFlag_DrawText,
		string
	);
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
	UI_Box *container = ui_box_make(
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_Clickable |
		UI_BoxFlag_HotAnimation |
		UI_BoxFlag_ActiveAnimation |
		UI_BoxFlag_DrawBorder,
		string
	);
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
		UI_Box *check = ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));
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
ui_push_scrollable_region_axis(Str8 string, Axis2 axis)
{
	ui_push_string(string);
	B32 smooth_scroll = true;
	ui_next_child_layout_axis(axis_flip(axis));
	UI_Box *container = ui_box_make(UI_BoxFlag_ViewScroll, string);
	ui_push_parent(container);

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	UI_Box *view_region = ui_box_make(UI_BoxFlag_Clip, str8_lit("ViewRegion"));

	ui_push_parent(view_region);

	ui_next_size(axis_flip(axis), ui_fill());
	ui_next_size(axis, ui_children_sum(1));
	UI_Box *content = ui_box_make((smooth_scroll ? UI_BoxFlag_AnimateScroll : 0), str8_lit("ScrollContent"));
	ui_push_parent(content);

	UI_ScrollabelRegion result;
	result.view_region = view_region;
	result.content     = content;
	return(result);
}

internal Void
ui_pop_scrollable_region_axis(Axis2 axis)
{
	UI_Box *content     = ui_pop_parent();
	UI_Box *view_region = ui_pop_parent();
	UI_Box *container   = ui_pop_parent();

	ui_push_parent(container);

	F32 percent_in_view = view_region->fixed_size.v[axis] / content->fixed_size.v[axis];
	if (percent_in_view < 1.0f)
	{
		F32 size             = container->fixed_size.v[axis] * percent_in_view;
		F32 max_pos          = container->fixed_size.v[axis] - size;
		F32 max_scroll       = content->fixed_size.v[axis] - view_region->fixed_size.v[axis];
		F32 percent_scrolled = content->scroll.v[axis] / max_scroll;
		F32 pos              = max_pos * percent_scrolled;

		ui_next_relative_pos(axis, pos);
		ui_next_color(v4f32(0.5f, 0.5f, 0.5f, 1));
		ui_next_size(axis_flip(axis), ui_em(0.4f, 1));
		ui_next_size(axis, ui_pixels(size, 1));
		UI_Box *scrollbar = ui_box_make(
			UI_BoxFlag_DrawBackground |
			(UI_BoxFlags) (UI_BoxFlag_FixedX << axis) |
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

	ui_pop_parent();

	ui_pop_string();

	UI_Comm comm = ui_comm_from_box(container);
	content->scroll.v[axis] += (F32) (comm.scroll.v[axis] * ui_dt() * 5000.0);
	content->scroll.v[axis]  = f32_clamp(0, content->scroll.v[axis], content->fixed_size.v[axis] - view_region->fixed_size.v[axis]);
}

internal UI_ScrollabelRegion
ui_push_scrollable_region(Str8 string)
{
	ui_push_string(string);
	B32 smooth_scroll = true;
	ui_next_child_layout_axis(Axis2_X);
	UI_Box *vert_container = ui_box_make(UI_BoxFlag_ViewScroll, string);
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
	UI_Box *content = ui_box_make((smooth_scroll ? UI_BoxFlag_AnimateScroll : 0), str8_lit("ScrollContent"));
	ui_push_parent(content);

	UI_ScrollabelRegion result;
	result.view_region = view_region;
	result.content     = content;
	return(result);
}

internal Void
ui_scrollable_region_set_scroll(UI_ScrollabelRegion region, F32 offset)
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
			UI_Box *scrollbar = ui_box_make(
				UI_BoxFlag_DrawBackground |
				(UI_BoxFlags) (UI_BoxFlag_FixedX << axis) |
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
		content->scroll.v[axis] += (F32) (comm.scroll.v[axis] * ui_dt() * 5000.0);
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

// TODO(simon): Maybe we only want to return true if the value changes.
internal B32
ui_combo_box_internal(Str8 name, U32 *selected_index, Str8 *item_names, U32 item_count, UI_ComboBoxParams *params)
{
	B32 result = false;

	if (params->item_size.kind == UI_SizeKind_Null)
	{
		F32 largest_width = 0.0f;
		Render_Font *font = render_font_from_key(ui_renderer(), ui_font_key_from_text_style(ui_top_text_style()));
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
		UI_Box *combo_box = ui_box_make(
			UI_BoxFlag_DrawBackground |
			UI_BoxFlag_DrawBorder |
			UI_BoxFlag_HotAnimation |
			UI_BoxFlag_ActiveAnimation |
			UI_BoxFlag_Clickable,
			name
		);
		UI_Comm comm = ui_comm_from_box(combo_box);

		ui_parent(combo_box)
		{
			UI_Key combo_box_key = ui_key_from_stringf(ui_key_null(), "%"PRISTR8"ComboItems", str8_expand(name));

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
					UI_Box *item = ui_box_make(
						UI_BoxFlag_DrawText |
						UI_BoxFlag_Clickable |
						UI_BoxFlag_HotAnimation |
						UI_BoxFlag_ActiveAnimation,
						item_names[i]
					);
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

internal Void
ui_sat_val_picker(F32 hue, F32 *out_sat, F32 *out_val, Str8 string)
{
	Render_Context *renderer = ui_renderer();
	Vec3F32 rgb = rgb_from_hsv(v3f32(hue, 1, 1));
	ui_next_colors(
		v4f32(1, 1, 1, 1),
		v4f32(rgb.x, rgb.y, rgb.z, 1),
		v4f32(0, 0, 0, 1),
		v4f32(0, 0, 0, 1)
	);
	ui_next_hover_cursor(Gfx_Cursor_Hand);
	UI_Box *sat_val_box = ui_box_make(
		UI_BoxFlag_DrawBorder |
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_Clickable,
		string
	);
	UI_Comm sat_val_comm = ui_comm_from_box(sat_val_box);

	if (sat_val_comm.dragging)
	{
		Vec2F32 rel_normalized = v2f32_hadamard_div_v2f32(sat_val_comm.rel_mouse, sat_val_box->fixed_size);
		*out_sat = rel_normalized.x;
		*out_val = 1.0f - rel_normalized.y;

		*out_sat = f32_clamp(0, *out_sat, 1);
		*out_val = f32_clamp(0, *out_val, 1);
	}

	// NOTE(hampus): Indicator
	ui_parent(sat_val_box)
	{
		Vec2F32 pos = {0};
		F32 indicator_size_px = ui_em(0.5f, 1).value;
		ui_next_width(ui_pixels(indicator_size_px, 1));
		ui_next_height(ui_pixels(indicator_size_px, 1));
		ui_next_relative_pos(Axis2_X, *out_sat * sat_val_box->fixed_size.x - indicator_size_px/2);
		ui_next_relative_pos(Axis2_Y, (1.0f - *out_val) * sat_val_box->fixed_size.y - indicator_size_px/2);
		ui_next_corner_radius(5);
		ui_box_make(
			UI_BoxFlag_DrawBorder |
			UI_BoxFlag_FixedPos,
			str8_lit("")
		);
	}
}

UI_CUSTOM_DRAW_PROC(hue_picker_custom_draw)
{
	Render_Context *renderer = ui_renderer();

	UI_RectStyle *rect_style = &root->rect_style;
	UI_TextStyle *text_style = &root->text_style;

	Render_Font *font = render_font_from_key(renderer, ui_font_key_from_text_style(text_style));

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
		F32 rect_width = root->fixed_rect.max.x - root->fixed_rect.min.x;
		F32 rect_height = root->fixed_rect.max.y - root->fixed_rect.min.y;
		RectF32 rect = {0};
		rect.min = root->fixed_rect.min;
		rect.max = v2f32(root->fixed_rect.max.x, root->fixed_rect.min.y + rect_height / 6);
		for (U32 i = 0; i < 6; ++i)
		{
			F32 hue0 = (F32) (i)/6;
			F32 hue1 = (F32) (i+1)/6;
			Vec3F32 rgb0 = rgb_from_hsv(v3f32(hue0, 1, 1));
			Vec3F32 rgb1 = rgb_from_hsv(v3f32(hue1, 1, 1));
			Vec4F32 rgba0 = v4f32(rgb0.x, rgb0.y, rgb0.z, 1);
			Vec4F32 rgba1 = v4f32(rgb1.x, rgb1.y, rgb1.z, 1);
			Vec2F32 min = root->fixed_rect.min;
			Vec2F32 max = root->fixed_rect.min;
			Render_RectInstance *instance = render_rect(
				renderer, rect.min, rect.max,
				.slice = rect_style->slice,
				.use_nearest = rect_style->texture_filter
			);
			instance->colors[Corner_TopLeft] = rgba0;
			instance->colors[Corner_TopRight] = rgba0;
			instance->colors[Corner_BottomLeft] = rgba1;
			instance->colors[Corner_BottomRight] = rgba1;
			rect.min.y += rect_height / 6;
			rect.max.y += rect_height / 6;
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
ui_hue_picker(F32 *out_hue, Str8 string)
{
	ui_next_hover_cursor(Gfx_Cursor_Hand);
	UI_Box *hue_box = ui_box_make(
		UI_BoxFlag_DrawBorder |
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_Clickable,
		string
	);
	ui_box_equip_custom_draw_proc(hue_box, hue_picker_custom_draw);

	UI_Comm hue_comm = ui_comm_from_box(hue_box);
	if (hue_comm.dragging)
	{
		Vec2F32 rel_normalized = v2f32_hadamard_div_v2f32(hue_comm.rel_mouse, hue_box->fixed_size);
		*out_hue = rel_normalized.y;
		*out_hue = f32_clamp(0, *out_hue, 1.0f);
	}

	// NOTE(hampus): Indicator
	ui_parent(hue_box)
	{
		ui_next_relative_pos(Axis2_Y, *out_hue * hue_box->fixed_size.y);
		ui_next_corner_radius(0);
		ui_next_width(ui_pct(1, 1));
		ui_next_height(ui_pixels(3, 1));
		ui_box_make(
			UI_BoxFlag_DrawBackground |
			UI_BoxFlag_FixedPos,
			str8_lit("")
		);
	}
}

internal Void
ui_alpha_picker(Vec3F32 hsv, F32 *out_alpha, Str8 string)
{
	Vec4F32 top_color = {0};
	top_color.rgb = rgb_from_hsv(hsv);
	top_color.a = 1.0f;
	Vec4F32 bottom_color = top_color;
	bottom_color.a = 0;
	ui_next_vert_gradient(top_color, bottom_color);
	ui_next_hover_cursor(Gfx_Cursor_Hand);
	UI_Box *alpha_box = ui_box_make(
		UI_BoxFlag_DrawBorder |
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_Clickable,
		string
	);

	UI_Comm alpha_comm = ui_comm_from_box(alpha_box);
	if (alpha_comm.dragging)
	{
		Vec2F32 rel_normalized = v2f32_hadamard_div_v2f32(alpha_comm.rel_mouse, alpha_box->fixed_size);
		*out_alpha = 1.0f - rel_normalized.y;
		*out_alpha = f32_clamp(0, *out_alpha, 1.0f);
	}

	// NOTE(hampus): Indicator
	ui_parent(alpha_box)
	{
		ui_next_relative_pos(Axis2_Y, (1.0f - *out_alpha) * alpha_box->fixed_size.y);
		ui_next_corner_radius(0);
		ui_next_width(ui_pct(1, 1));
		ui_next_height(ui_pixels(3, 1));
		ui_box_make(
			UI_BoxFlag_DrawBackground |
			UI_BoxFlag_FixedPos,
			str8_lit("")
		);
	}
}

internal Str8
ui_push_replace_string(Arena *arena, Str8 edit_str, Vec2S64 range, U8 *buffer, U64 buffer_size, Str8 replace_str)
{
	U64 min_range = (U64) (range.min);
	U64 max_range = (U64) (range.max);
	min_range = u64_min(min_range, edit_str.size);
	max_range = u64_min(max_range, edit_str.size);
	if (min_range > max_range)
	{
		swap(min_range, max_range, U64);
	}
	U64 replace_range_length = max_range - min_range;
	Str8 new_buffer = {0};
	U64 new_buffer_size = edit_str.size - replace_range_length + replace_str.size;
	new_buffer.data = push_array(arena, U8, new_buffer_size);
	new_buffer.size = new_buffer_size;
	Str8 before_range = str8_prefix(edit_str, min_range);
	Str8 after_range  = str8_skip(edit_str, max_range);
	if (before_range.size != 0)
	{
		memory_copy(new_buffer.data, before_range.data, before_range.size);
	}
	if (replace_str.size != 0)
	{
		memory_copy(new_buffer.data + min_range, replace_str.data, replace_str.size);
	}
	if (after_range.size != 0)
	{
		memory_copy(new_buffer.data + min_range + replace_str.size, after_range.data, after_range.size);
	}
	new_buffer.size = u64_min(new_buffer.size, buffer_size);
	return(new_buffer);
}

// TODO(simon): This scan routine will drift if the font has kerning. This
// means that the more "total kerning" we have, the more we will drift away
// from the true character the mouse is really over.
internal S64
ui_get_codepoint_index_from_mouse_pos(UI_Box *box, Str8 edit_str)
{
	S64 result = S64_MAX;
	Vec2F32 mouse_pos = ui_mouse_pos();
	if (mouse_pos.x < box->fixed_rect.x0)
	{
		result = S64_MIN;
	}

	F32 x = -box->scroll.x;
	for (U64 i = 0; i < edit_str.size;)
	{
		StringDecode decode = string_decode_utf8(&edit_str.data[i], edit_str.size - i);

		Vec2F32 dim = render_measure_character(render_font_from_key(ui_renderer(), ui_top_font_key()), decode.codepoint);
		RectF32 character_rect = box->fixed_rect;
		character_rect.min.x = x;
		character_rect.max.x = character_rect.min.x + dim.x;
		if (mouse_pos.x >= character_rect.x0 && mouse_pos.x < character_rect.x1)
		{
			result = (S64) i;
			break;
		}

		x += dim.x;
		i += decode.size;
	}

	return(result);
}

internal UI_Comm
ui_line_edit(UI_TextEditState *edit_state, U8 *buffer, U64 buffer_size, U64 *string_length, Str8 string)
{
	UI_Comm comm = {0};
	ui_seed(string)
	{
		Str8 buffer_str8 = str8(buffer, buffer_size);
		ui_next_child_layout_axis(Axis2_X);
		ui_next_hover_cursor(Gfx_Cursor_Beam);
		UI_Box *box = ui_box_make(
			UI_BoxFlag_DrawBackground |
			UI_BoxFlag_HotAnimation |
			UI_BoxFlag_FocusAnimation |
			UI_BoxFlag_Clickable |
			UI_BoxFlag_DrawBorder |
			UI_BoxFlag_Clip |
			UI_BoxFlag_AnimateScroll,
			string
		);

		Str8 edit_str = (Str8) {buffer, *string_length};

		comm = ui_comm_from_box(box);
		if (comm.pressed)
		{
			edit_state->cursor = edit_state->mark = (S64) ui_get_codepoint_index_from_mouse_pos(box, edit_str);
			edit_state->mark = s64_clamp(0, edit_state->mark, (S64) edit_str.size);
		}
		if (comm.dragging)
		{
			edit_state->cursor = (S64) ui_get_codepoint_index_from_mouse_pos(box, edit_str);
		}
		edit_state->cursor = s64_clamp(0, edit_state->cursor, (S64) edit_str.size);

		if (ui_box_is_focused(box))
		{
			UI_TextActionList text_actions = ui_text_action_list_from_events(ui_frame_arena(), ui_events());
			for (UI_TextActionNode *node = text_actions.first; node != 0; node = node->next)
			{
				UI_TextAction action = node->action;
				UI_TextOp op = ui_text_op_from_state_and_action(ui_frame_arena(), edit_str, edit_state, &action);
				if (op.copy_string.size)
				{
					os_set_clipboard(op.copy_string);
				}

				arena_scratch(0, 0)
				{
					Str8 new_str = ui_push_replace_string(scratch, edit_str, op.range, buffer, buffer_size, op.replace_string);
					memory_copy(buffer, new_str.data, new_str.size);
					*string_length = new_str.size;
					edit_str.size = new_str.size;
				}

				edit_state->cursor = s64_clamp(0, op.new_cursor, (S64) edit_str.size);
				edit_state->mark   = s64_clamp(0, op.new_mark,   (S64) edit_str.size);
			}

			ui_parent(box)
			{
				Vec2F32 cursor_offset = render_measure_text_length(render_font_from_key(ui_renderer(), ui_top_font_key()), buffer_str8, (U64) edit_state->cursor);
				F32 cursor_extra_offset = ui_em(0.1f, 1).value;
				ui_next_relative_pos(Axis2_X, cursor_offset.x+cursor_extra_offset);
				ui_next_height(ui_pct(1, 1));
				ui_next_width(ui_pixels(3, 1));
				ui_next_color(v4f32(0.9f, 0.9f, 0.9f, 1));
				UI_Box *cursor_box = ui_box_make(
					UI_BoxFlag_DrawBackground |
					UI_BoxFlag_FixedPos,
					str8_lit("CursorBox")
				);

				{
					Vec2F32 mark_offset = render_measure_text_length(render_font_from_key(ui_renderer(), ui_top_font_key()), buffer_str8, (U64) edit_state->mark);
					if (edit_state->mark < edit_state->cursor)
					{
						ui_next_relative_pos(Axis2_X, mark_offset.x+cursor_extra_offset);
						ui_next_width(ui_pixels(cursor_offset.x - mark_offset.x, 1));
					}
					else
					{
						ui_next_relative_pos(Axis2_X, cursor_offset.x+cursor_extra_offset);
						ui_next_width(ui_pixels(mark_offset.x - cursor_offset.x, 1));
					}
					ui_next_height(ui_pct(1, 1));
					ui_next_color(v4f32(0.5f, 0.5f, 0.9f, 0.4f));
					UI_Box *mark_box = ui_box_make(
						UI_BoxFlag_DrawBackground |
						UI_BoxFlag_FixedPos,
						str8_lit("MarkBox")
					);
				}

				// NOTE(hampus): Make sure the cursor is in view

				Vec2F32 text_size = render_measure_text(render_font_from_key(ui_renderer(), ui_top_font_key()), edit_str);
				F32 padding = ui_em(0.5f, 1).value;

				// NOTE(hampus): Scroll to the left if there is empty 
				// space to the right and there are still characters
				// outside on the left iside
				F32 content_size = text_size.x + cursor_box->fixed_size.x + padding + cursor_extra_offset;
				F32 adjustment = box->fixed_size.x - (content_size-box->scroll.x);
				adjustment = f32_clamp(0, adjustment, box->scroll.x);
				box->scroll.x -= adjustment;

				Vec2F32 cursor_visiblity_range = v2f32(
					cursor_box->rel_pos.x,
					cursor_box->rel_pos.x + cursor_box->fixed_size.x
				);

				cursor_visiblity_range.min = f32_max(0, cursor_visiblity_range.min);
				cursor_visiblity_range.max = f32_max(0, cursor_visiblity_range.max);

				Vec2F32 box_visibility_range = v2f32(box->scroll.x, box->scroll.x + box->fixed_size.x-padding);
				F32 delta_left = cursor_visiblity_range.min - box_visibility_range.min;
				F32 delta_right = cursor_visiblity_range.max - box_visibility_range.max;
				delta_left = f32_min(delta_left, 0);
				delta_right = f32_max(delta_right, 0);

				box->scroll.x += delta_left + delta_right;
				box->scroll.x = f32_max(0, box->scroll.x);
			}
		}

		ui_parent(box)
		{
			ui_next_text_padding(Axis2_X, 0);
			ui_text(edit_str);
		}
	}

	return(comm);
}

internal UI_Comm
ui_line_editf(UI_TextEditState *edit_state, U8 *buffer, U64 buffer_size, U64 *string_length, CStr format, ...)
{
	UI_Comm comm = {0};
	va_list args;
	va_start(args, format);
	Str8 string = str8_pushfv(ui_frame_arena(), format, args);
	UI_Comm result = ui_line_edit(edit_state, buffer, buffer_size, string_length, string);
	va_end(args);
	return(result);
}

internal Void
ui_color_picker(UI_ColorPickerData *data)
{
	if (data->text_buffer_size[0] == 0)
	{
		for (U64 i = 0; i < 4; ++i)
		{
			data->text_buffer_size[i] = 4;
			data->text_buffer[i] = push_array(ui_permanent_arena(), U8, data->text_buffer_size[i]);
		}
	}

	Vec4F32 *rgba = data->rgba;
	Vec3F32 hsv = hsv_from_rgb(rgba->rgb);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	UI_Box *container = ui_box_make(
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_DrawBorder,
		str8_lit("")
	);
	ui_parent(container)
	{
		ui_spacer(ui_em(0.5f, 1));
		ui_row()
		{
			ui_spacer(ui_em(0.5f, 1));
			// NOTE(hampus): Saturation and value
			ui_next_width(ui_em(10, 1));
			ui_next_height(ui_em(10, 1));
			ui_sat_val_picker(hsv.x, &hsv.y, &hsv.z, str8_lit("SatValPicker"));
			ui_spacer(ui_em(0.5f, 1));
			ui_column()
			{
				// NOTE(hampus): Hue
				ui_next_height(ui_em(10, 1));
				ui_next_width(ui_em(1, 1));
				ui_hue_picker(&hsv.x, str8_lit("HuePicker"));
			}
			ui_spacer(ui_em(0.5f, 1));
			ui_column()
			{
				// NOTE(hampus): Alpha
				ui_next_height(ui_em(10, 1));
				ui_next_width(ui_em(1, 1));
				ui_alpha_picker(hsv, &rgba->a, str8_lit("AlphaPicker"));
			}
			rgba->rgb = rgb_from_hsv(hsv);
			ui_spacer(ui_em(0.5f, 1));
		}
		ui_spacer(ui_em(0.5f, 1));
		ui_row()
			ui_width(ui_em(4, 1))
		{
			ui_textf("R: %.2f", rgba->r);
			ui_textf("G: %.2f", rgba->g);
			ui_textf("B: %.2f", rgba->b);
			ui_textf("A: %.2f", rgba->a);
		}
		ui_spacer(ui_em(0.5f, 1));
		ui_row()
			ui_width(ui_em(4, 1))
		{
			ui_textf("H: %.2f", hsv.x);
			ui_textf("S: %.2f", hsv.y);
			ui_textf("V: %.2f", hsv.z);
		}
		ui_spacer(ui_em(0.5f, 1));
		Str8 line_edit_labels[] =
		{
			str8_lit("R:"),
			str8_lit("G:"),
			str8_lit("B:"),
			str8_lit("A:"),
		};

		for (U64 i = 0; i < 4; ++i)
		{
			ui_row()
			{
				ui_spacer(ui_em(0.5f, 1));
				ui_next_width(ui_em(1, 1));
				ui_text(line_edit_labels[i]);
				ui_spacer(ui_em(0.3f, 1));
				ui_next_width(ui_em(3, 1));
				UI_Comm comm = ui_line_editf(
					data->text_edit_state + i,
					data->text_buffer[i],
					data->text_buffer_size[i],
					data->string_length + i,
					"ColorPickerLineEdit%d", i
				);

				if (ui_box_is_focused(comm.box))
				{
					F64 f64 = 0;
					f64_from_str8(str8(data->text_buffer[i], data->string_length[i]), &f64);
					rgba->v[i] = f32_clamp(0, (F32) f64, 1.0f);
				}
				else
				{
					arena_scratch(0, 0)
					{
						Str8 text_buffer_str8 = str8_pushf(scratch, "%.2f", rgba->v[i]);
						data->string_length[i] = u64_min(text_buffer_str8.size, data->text_buffer_size[i]);
						memory_copy_typed(data->text_buffer[i], text_buffer_str8.data, data->string_length[i]);
					}
				}
			}
			ui_spacer(ui_em(0.5f, 1));
		}
	}
}
