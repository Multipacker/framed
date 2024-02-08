////////////////////////////////
//~ hampus: Short term
//
// [ ] @bug The user can drop a panel on the menu bar which will hide the tab bar
// [ ] @bug Tab offsetting looks weird if you remove any tab to the 
//          left of the active tab when the tab bar is full
// [ ] @bug New window offset is wrong if you reorder tab and then drag it out
// [ ] @code Keep a list of closed windows instead of flags

////////////////////////////////
//~ hampus: Medium term
//
// [ ] @code @feature UI startup builder
// [ ] @polish Resizing panels with tab animation doesn't look that good right now.
// [ ] @polish Adding a tab with a tab offset active doesn't look perfect

////////////////////////////////
//~ hampus: Long term
//
// [ ] @bug Close button is rendered even though the tab is outside tab bar
//            - Solved by removing the clip box flag, but this shouldn't solve it
// [ ] @feature Move around windows that have multiple panels
// [ ] @feature Be able to pin windows which disables closing
// [ ] @code @cleanup UI commands. Discriminated unions instead of data array?
// [ ] @feature Scroll tabs horizontally if there are too many to fit
//                 - Partially fixed. You can navigate tabs by pressing the arrows to the right

////////////////////////////////
//~ hampus: Globals

extern FramedUI_Tab g_nil_tab;

global FramedUI_State *framed_ui_state;

read_only FramedUI_Panel g_nil_panel =
{
	{ &g_nil_panel, &g_nil_panel },
	&g_nil_panel,
	&g_nil_panel,
	{ &g_nil_tab, &g_nil_tab, &g_nil_tab },
	&g_nil_box,
};

read_only FramedUI_Tab g_nil_tab =
{
	&g_nil_tab,
	&g_nil_tab,
	&g_nil_panel,
	&g_nil_box,
	// &g_nil_box,
};

////////////////////////////////
//~ hampus: Basic helpers

internal B32
framed_ui_panel_is_nil(FramedUI_Panel *panel)
{
	B32 result = (!panel || panel == &g_nil_panel);
	return(result);
}

internal B32
framed_ui_tab_is_nil(FramedUI_Tab *tab)
{
	B32 result = (!tab || tab == &g_nil_tab);
	return(result);
}

////////////////////////////////
//~ hampus: Theming

internal Vec4F32
framed_ui_color_from_theme(FramedUI_Color color)
{
	Vec4F32 result = framed_ui_state->theme.colors[color];
	return(result);
}

internal Void
framed_ui_set_color(FramedUI_Color color, Vec4F32 value)
{
	framed_ui_state->theme.colors[color] = value;
}

internal Str8
framed_ui_string_from_color(FramedUI_Color color)
{
	Str8 result = str8_lit("Null");
	switch (color)
	{
		case FramedUI_Color_Panel: { result = str8_lit("Panel background");       } break;
		case FramedUI_Color_InactivePanelBorder: { result = str8_lit("Inactive panel border");  } break;
		case FramedUI_Color_ActivePanelBorder: { result = str8_lit("Active panel border");    } break;
		case FramedUI_Color_InactivePanelOverlay: { result = str8_lit("Inactive panel overlay"); } break;

		case FramedUI_Color_TabBar: { result = str8_lit("Tab bar background");      } break;
		case FramedUI_Color_ActiveTab: { result = str8_lit("Active tab background");   } break;
		case FramedUI_Color_InactiveTab: { result = str8_lit("Inactive tab background"); } break;
		case FramedUI_Color_TabTitle: { result = str8_lit("Tab foreground");          } break;
		case FramedUI_Color_TabBorder: { result = str8_lit("Tab border");              } break;
		case FramedUI_Color_TabBarButtons: { result = str8_lit("Tab bar buttons background");         } break;

			invalid_case;
	}
	return(result);
}

////////////////////////////////
//~ hampus: Command

internal Void *
framed_ui_command_push(FramedUI_CommandBuffer *buffer, FramedUI_CommandKind kind)
{
	assert(buffer->pos < buffer->size);
	FramedUI_Command *result = buffer->buffer + buffer->pos;
	memory_zero_struct(result);
	result->kind = kind;
	buffer->pos++;
	return(result->data);
}

internal Void
framed_ui_attempt_to_close_tab(FramedUI_Tab *tab)
{
	if (!tab->pinned)
	{
		FramedUI_TabDelete *data = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_TabClose);
		data->tab = tab;
	}
}

internal Void
framed_ui_panel_attach_tab(FramedUI_Panel *panel, FramedUI_Tab *tab, B32 set_active)
{
	FramedUI_TabAttach *data  = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_TabAttach);
	data->tab        = tab;
	data->panel      = panel;
	data->set_active = set_active;
}

internal Void
framed_ui_panel_split(FramedUI_Panel *first, Axis2 split_axis)
{
	FramedUI_PanelSplit *data = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_PanelSplit);
	data->panel = first;
	data->axis = split_axis;
}

internal Void
framed_ui_panel_split_and_attach_tab(FramedUI_Panel *panel, FramedUI_Tab *tab, Axis2 axis, Side side)
{
	FramedUI_PanelSplitAndAttach *data  = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_PanelSplitAndAttach);
	data->tab           = tab;
	data->panel         = panel;
	data->axis          = axis;
	data->panel_side    = side;
}

internal B32
framed_ui_swap_tabs(FramedUI_Tab *tab0, FramedUI_Tab *tab1)
{
	B32 result = false;
	if (!(tab0->pinned || tab1->pinned))
	{
		framed_ui_state->swap_tab0 = tab0;
		framed_ui_state->swap_tab1 = tab1;
		result = true;
	}
	return(result);
}

internal Void
framed_ui_set_tab_to_active(FramedUI_Tab *tab)
{
	FramedUI_PanelSetActiveTab *data = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_PanelSetActiveTab);
	data->tab = tab;
	data->panel = tab->panel;
}

internal Void
framed_ui_attempt_to_close_panel(FramedUI_Panel *panel)
{
	B32 any_tab_pinned = false;
	for (FramedUI_Tab *tab = panel->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
	{
		if (tab->pinned)
		{
			any_tab_pinned = true;
			break;
		}
	}
	if (!any_tab_pinned)
	{
		FramedUI_PanelClose *data = framed_ui_command_push(&framed_ui_state->cmd_buffer, FramedUI_CommandKind_PanelClose);
		data->panel = panel;
	}
}

////////////////////////////////
//~ hampus: Tab dragging

internal Void
framed_ui_drag_begin_reordering(FramedUI_Tab *tab)
{
	if (!tab->pinned)
	{
		FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
		drag_data->tab = tab;
		framed_ui_state->drag_data.drag_origin = ui_mouse_pos();
		framed_ui_state->drag_status = FramedUI_DragStatus_Reordering;
		log_info("Drag: reordering");
	}
}

internal Void
framed_ui_wait_for_drag_threshold(Void)
{
	framed_ui_state->drag_status = FramedUI_DragStatus_WaitingForDragThreshold;
	log_info("Drag: waiting for drag threshold");
}

internal Void
framed_ui_drag_release(Void)
{
	framed_ui_state->drag_status = FramedUI_DragStatus_Released;
	log_info("Drag: released");
}

internal Void
framed_ui_drag_end(Void)
{
	framed_ui_state->drag_status = FramedUI_DragStatus_Inactive;
	memory_zero_struct(&framed_ui_state->drag_data);
	log_info("Drag: end");
}

internal B32
framed_ui_is_dragging(Void)
{
	B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Dragging;
	return(result);
}

internal B32
framed_ui_is_tab_reordering(Void)
{
	B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Reordering;
	return(result);
}

internal B32
framed_ui_is_waiting_for_drag_threshold(Void)
{
	B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_WaitingForDragThreshold;
	return(result);
}

internal B32
framed_ui_drag_is_inactive(Void)
{
	B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Inactive;
	return(result);
}

////////////////////////////////
//~ hampus: Tabs

internal FramedUI_Tab *
framed_ui_tab_alloc(Arena *arena)
{
	FramedUI_Tab *result = push_struct(arena, FramedUI_Tab);
	return(result);
}

internal Void
framed_ui_tab_equip_view_info(FramedUI_Tab *tab, FramedUI_TabViewInfo view_info)
{
	tab->view_info = view_info;
}

frame_ui_tab_view(framed_ui_tab_view_default);
internal FramedUI_Tab *
framed_ui_tab_make(Arena *arena, FramedUI_TabViewProc *function, Void *data, Str8 name)
{
	FramedUI_Tab *result = framed_ui_tab_alloc(arena);
	result->next = result->prev = &g_nil_tab;
	result->panel = &g_nil_panel;
	// result->tab_container = result->tab_box = &g_nil_box;
	if (!name.size)
	{
		// NOTE(hampus): We probably won't do this in the future because
		// you won't probably be able to have unnamed tabs.
		result->string = str8_pushf(framed_ui_state->perm_arena, "Tab%"PRIS32, framed_ui_state->num_tabs);
	}
	else
	{
		result->string = str8_copy(framed_ui_state->perm_arena, name);
	}
	// TODO(hampus): Check for name collision with other tabs
	FramedUI_TabViewInfo view_info = { function, data };
	framed_ui_tab_equip_view_info(result, view_info);
	if (!function)
	{
		// NOTE(hampus): Equip with default view function
		result->view_info.function = framed_ui_tab_view_default;
		result->view_info.data = result;
	}
	framed_ui_state->num_tabs++;
	log_info("Allocated tab: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal B32
framed_ui_tab_is_active(FramedUI_Tab *tab)
{
	B32 result = tab->panel->tab_group.active_tab == tab;
	return(result);
}

internal B32
framed_ui_tab_is_dragged(FramedUI_Tab *tab)
{
	B32 result = false;
	if (framed_ui_state->drag_status == FramedUI_DragStatus_Dragging)
	{
		if (framed_ui_state->drag_data.tab == tab)
		{
			result = true;
		}
	}
	return(result);
}

internal UI_Box *
framed_ui_tab_button(FramedUI_Tab *tab)
{
	assert(tab->frame_index != framed_ui_state->frame_index);
	tab->frame_index = framed_ui_state->frame_index;
	ui_push_string(tab->string);

	B32 active = framed_ui_tab_is_active(tab);

	F32 height_em = 1.1f;
	F32 corner_radius = (F32) ui_top_font_line_height() * 0.2f;

	Vec4F32 color = active ?
		framed_ui_color_from_theme(FramedUI_Color_ActiveTab) :
		framed_ui_color_from_theme(FramedUI_Color_InactiveTab);
	ui_next_color(color);
	ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
	ui_next_vert_corner_radius(corner_radius, 0);
	ui_next_child_layout_axis(Axis2_X);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_em(height_em, 1));
	ui_next_hover_cursor(Gfx_Cursor_Hand);
	UI_Box *title_container = ui_box_make(
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_HotAnimation |
		UI_BoxFlag_ActiveAnimation |
		UI_BoxFlag_Clickable  |
		UI_BoxFlag_DrawBorder |
		UI_BoxFlag_AnimateY,
		str8_lit("TitleContainer"));

	// tab->tab_box = title_container;
	ui_parent(title_container)
	{
		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_PIN);
		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(corner_radius, 0, 0, 0);
		ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
		ui_next_color(framed_ui_color_from_theme(FramedUI_Color_InactiveTab));
		UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
		if (tab->pinned)
		{
			pin_box_flags |= UI_BoxFlag_DrawText;
		}
		UI_Box *pin_box = ui_box_make(pin_box_flags, str8_lit("PinButton"));

		ui_next_text_color(framed_ui_color_from_theme(FramedUI_Color_TabTitle));
		UI_Box *title = ui_box_make(UI_BoxFlag_DrawText, tab->string);

		ui_box_equip_display_string(title, tab->string);

		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_CROSS);
		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(0, corner_radius, 0, 0);
		ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
		ui_next_color(framed_ui_color_from_theme(FramedUI_Color_InactiveTab));
		UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable, str8_lit("CloseButton"));
		// TODO(hampus): We shouldn't need to do this here
		// since there shouldn't even be any input events
		// left in the queue if dragging is ocurring.
		if (framed_ui_is_tab_reordering())
		{
			FramedUI_Tab *drag_tab = framed_ui_state->drag_data.tab;
			if (tab != drag_tab)
			{
				Vec2F32 mouse_pos = ui_mouse_pos();
				F32 center = (title_container->parent->fixed_rect.x0 +  title_container->parent->fixed_rect.x1) / 2;
				B32 hovered  = mouse_pos.x >= center - ui_top_font_line_height() * 0.2f && mouse_pos.x <= center + ui_top_font_line_height() * 0.2f;
				if (hovered)
				{
					if (f32_abs(drag_tab->tab_container->rel_pos.x - drag_tab->tab_container->rel_pos_animated.x) <= 0.5f && f32_abs(tab->tab_container->rel_pos.x - tab->tab_container->rel_pos_animated.x) <= 0.5f)
					{
						framed_ui_swap_tabs(framed_ui_state->drag_data.tab, tab);
					}
				}
			}
		}

		if (framed_ui_drag_is_inactive())
		{
			UI_Comm pin_box_comm   = ui_comm_from_box(pin_box);
			UI_Comm close_box_comm = { 0 };
			if (!tab->pinned)
			{
				close_box_comm = ui_comm_from_box(close_box);
			}

			UI_Comm title_comm = ui_comm_from_box(title_container);
			if (title_comm.pressed)
			{
				framed_ui_drag_begin_reordering(tab);
				framed_ui_set_tab_to_active(tab);
			}

			// NOTE(hampus): Icon appearance
			UI_BoxFlags icon_hover_flags =
				UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation |
				UI_BoxFlag_ActiveAnimation | UI_BoxFlag_DrawText;

			if (pin_box_comm.hovering)
			{
				pin_box->flags |= icon_hover_flags;
			}

			if (close_box_comm.hovering)
			{
				if (!tab->pinned)
				{
					close_box->flags |= icon_hover_flags;
				}
			}

			if (close_box_comm.hovering || pin_box_comm.hovering || title_comm.hovering)
			{
				pin_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
				if (!tab->pinned)
				{
					close_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
				}
			}

			if (pin_box_comm.pressed)
			{
				tab->pinned = !tab->pinned;
			}

			if (close_box_comm.pressed)
			{
				framed_ui_attempt_to_close_tab(tab);
			}
		}
	}
	ui_pop_string();
	return(title_container);
}

////////////////////////////////
//~ hampus: Panels

internal FramedUI_Panel *
framed_ui_panel_alloc(Arena *arena)
{
	FramedUI_Panel *result = push_struct(arena, FramedUI_Panel);
	result->children[Side_Min] = result->children[Side_Max] = result->sibling = result->parent = &g_nil_panel;
	result->tab_group.first = result->tab_group.active_tab = result->tab_group.last = &g_nil_tab;
	result->box = &g_nil_box;
	result->string = str8_pushf(framed_ui_state->perm_arena, "UI_Panel%"PRIS32, framed_ui_state->num_panels);
	framed_ui_state->num_panels++;
	log_info("Allocated panel: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal Side
framed_ui_get_panel_side(FramedUI_Panel *panel)
{
	assert(panel->parent);
	Side result = panel->parent->children[Side_Min] == panel ? Side_Min : Side_Max;
	assert(panel->parent->children[result] == panel);
	return(result);
}

internal B32
framed_ui_panel_is_leaf(FramedUI_Panel *panel)
{
	B32 result = (framed_ui_panel_is_nil(panel->children[0]) || framed_ui_panel_is_nil(panel->children[1]));
	return(result);
}

internal UI_Comm
framed_ui_hover_panel_type(Str8 string, F32 width_in_em, FramedUI_Panel *root, Axis2 axis, B32 center, Side side)
{
	ui_next_width(ui_em(width_in_em, 1));
	ui_next_height(ui_em(width_in_em, 1));
	ui_next_color(ui_top_text_style()->color);

	UI_Box *box = ui_box_make(
		UI_BoxFlag_DrawBackground |
		UI_BoxFlag_Clickable,
		string);

	if (!center)
	{
		ui_parent(box)
		{
			ui_seed(string)
			{
				ui_size(!axis, ui_fill())
					ui_size(axis, ui_pct(0.5f, 1))
					ui_border_color(ui_top_rect_style()->color[0])
				{
					ui_next_border_thickness(2);
					ui_box_make(
						UI_BoxFlag_DrawBorder |
						UI_BoxFlag_HotAnimation,
						str8_lit("LeftLeft"));

					ui_next_border_thickness(2);
					ui_box_make(
						UI_BoxFlag_DrawBorder |
						UI_BoxFlag_HotAnimation,
						str8_lit("LeftRight"));
				}
			}
		}
	}

	UI_Comm comm = ui_comm_from_box(box);
	return(comm);
}

internal Void
framed_ui_update_panel(FramedUI_Panel *root)
{
	assert(root->frame_index != framed_ui_state->frame_index);
	root->frame_index = framed_ui_state->frame_index;
	switch (root->split_axis)
	{
		case Axis2_X: ui_next_child_layout_axis(Axis2_X); break;
		case Axis2_Y: ui_next_child_layout_axis(Axis2_Y); break;
			invalid_case;
	}

	FramedUI_Panel *parent = root->parent;

	if (framed_ui_panel_is_nil(parent))
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
	}
	else
	{
		Axis2 flipped_split_axis = !parent->split_axis;
		ui_next_size(parent->split_axis, ui_pct(root->pct_of_parent, 0));
		ui_next_size(flipped_split_axis, ui_fill());
	}

	// NOTE(hampus): It is clickable and has view scroll so that it can consume
	// all the click and scroll events at the end to prevent boxes from behind
	// it to take them.
	ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
	UI_Box *box = ui_box_make(
		UI_BoxFlag_Clip |
		UI_BoxFlag_Clickable |
		UI_BoxFlag_ViewScroll,
		root->string
	);
	root->box = box;

	ui_push_parent(box);
#if 1
	if (!framed_ui_panel_is_leaf(root))
	{
		FramedUI_Panel *child0 = root->children[Side_Min];
		FramedUI_Panel *child1 = root->children[Side_Max];

		framed_ui_update_panel(child0);

		B32 dragging = false;
		F32 drag_delta = 0;
		ui_seed(root->string)
		{
			ui_next_size(root->split_axis, ui_em(0.2f, 1));
			ui_next_size(!root->split_axis, ui_pct(1, 1));
			ui_next_corner_radius(0);
			ui_next_hover_cursor(root->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
			ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
			UI_Box *draggable_box = ui_box_make(
				UI_BoxFlag_Clickable |
				UI_BoxFlag_DrawBackground,
				str8_lit("DraggableBox")
			);
			UI_Comm comm = ui_comm_from_box(draggable_box);
			if (comm.dragging)
			{
				dragging = true;
				drag_delta = comm.drag_delta.v[root->split_axis];
			}
		}

		framed_ui_update_panel(child1);

		if (dragging)
		{
			child0->pct_of_parent -= drag_delta / box->fixed_size.v[root->split_axis];
			child1->pct_of_parent += drag_delta / box->fixed_size.v[root->split_axis];

			child0->pct_of_parent = f32_clamp(0, child0->pct_of_parent, 1.0f);
			child1->pct_of_parent = f32_clamp(0, child1->pct_of_parent, 1.0f);
		}
	}
	else
	{
		ui_push_string(root->string);

		box->layout_style.child_layout_axis = Axis2_Y;

		{
			Gfx_EventList *event_list = ui_events();
			if (ui_mouse_is_inside_box(box))
			{
				for (Gfx_Event *node = event_list->first; node != 0; node = node->next)
				{
					if (node->kind == Gfx_EventKind_KeyPress &&
							(node->key == Gfx_Key_MouseLeft ||
							 node->key == Gfx_Key_MouseRight ||
							 node->key == Gfx_Key_MouseMiddle))
					{
						if (framed_ui_panel_is_nil(framed_ui_state->next_focused_panel))
						{
							framed_ui_state->next_focused_panel = root;
						}
						if (root->window != framed_ui_state->master_window)
						{
							framed_ui_window_reorder_to_front(root->window);
						}
					}
				}
			}
		}

		// NOTE(hampus): Axis2_COUNT is the center
		Axis2   hover_axis = Axis2_COUNT;
		Side    hover_side = 0;
		UI_Comm tab_release_comms[FramedUI_TabReleaseKind_COUNT] = { 0 };
		B32     hovering_any_symbols = false;

		//- hampus: Drag & split symbols

		if (framed_ui_is_dragging())
		{
			FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_pct(1, 1));
			UI_Box *split_symbols_container = ui_box_make(UI_BoxFlag_FloatingPos, str8_lit("SplitSymbolsContainer"));
			if (root == drag_data->hovered_panel)
			{
				ui_parent(split_symbols_container)
				{
					ui_push_string(str8_lit("SplitSymbolsContainer"));
					F32 size = 3;
					ui_spacer(ui_fill());

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());
						tab_release_comms[FramedUI_TabReleaseKind_Top] = framed_ui_hover_panel_type(
							str8_lit("TabReleaseTop"), size, root, Axis2_Y, false, Side_Min);

						ui_spacer(ui_fill());
					}
					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[FramedUI_TabReleaseKind_Left] = framed_ui_hover_panel_type(str8_lit("TabReleaseLeft"), size, root, Axis2_X, false, Side_Min);

						ui_spacer(ui_em(1, 1));

						tab_release_comms[FramedUI_TabReleaseKind_Center] = framed_ui_hover_panel_type(str8_lit("TabReleaseCenter"), size, root, Axis2_X, true, Side_Min);

						ui_spacer(ui_em(1, 1));

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[FramedUI_TabReleaseKind_Right] = framed_ui_hover_panel_type(str8_lit("TabReleaseRight"), size, root, Axis2_X, false, Side_Max);

						ui_spacer(ui_fill());
					}

					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());
						tab_release_comms[FramedUI_TabReleaseKind_Bottom] = framed_ui_hover_panel_type(str8_lit("TabReleaseBottom"), size, root, Axis2_Y, false, Side_Max);
						ui_spacer(ui_fill());
					}
					ui_spacer(ui_fill());
					ui_pop_string();
				}
			}

			if (root != drag_data->tab->panel)
			{
				UI_Comm panel_comm = ui_comm_from_box(split_symbols_container);
				if (panel_comm.hovering)
				{
					drag_data->hovered_panel = root;
				}
			}

			for (FramedUI_TabReleaseKind i = (FramedUI_TabReleaseKind) 0; i < FramedUI_TabReleaseKind_COUNT; ++i)
			{
				UI_Comm *comm = tab_release_comms + i;
				if (comm->hovering)
				{
					hovering_any_symbols = true;
					switch (i)
					{
						case FramedUI_TabReleaseKind_Center:
						{
							hover_axis = Axis2_COUNT;
						} break;

						case FramedUI_TabReleaseKind_Left:
						case FramedUI_TabReleaseKind_Right:
						{
							hover_axis = Axis2_X;
							hover_side = i == FramedUI_TabReleaseKind_Left ? Side_Min : Side_Max;
						} break;

						case FramedUI_TabReleaseKind_Top:
						case FramedUI_TabReleaseKind_Bottom:
						{
							hover_axis = Axis2_Y;
							hover_side = i == FramedUI_TabReleaseKind_Top ? Side_Min : Side_Max;
						} break;

						invalid_case;
					}
				}
				if (comm->released)
				{
					switch (i)
					{
						case FramedUI_TabReleaseKind_Center:
						{
							framed_ui_panel_attach_tab(root, drag_data->tab, true);
							dll_remove(
								framed_ui_state->window_list.first,
								framed_ui_state->window_list.last,
								drag_data->tab->panel->window);
							framed_ui_state->drag_status = FramedUI_DragStatus_Released;
						} break;

						case FramedUI_TabReleaseKind_Left:
						case FramedUI_TabReleaseKind_Right:
						case FramedUI_TabReleaseKind_Top:
						case FramedUI_TabReleaseKind_Bottom:
						{
							framed_ui_panel_split_and_attach_tab(root, drag_data->tab, hover_axis, hover_side);
							dll_remove(
								framed_ui_state->window_list.first,
								framed_ui_state->window_list.last,
								drag_data->tab->panel->window);
							framed_ui_state->drag_status = FramedUI_DragStatus_Released;
						} break;

						invalid_case;
					}
					framed_ui_drag_release();
				}
			}
		}

		//- hampus: Drag & split preview overlay

		if (hovering_any_symbols)
		{
			Vec4F32 top_color = ui_top_rect_style()->color[0];
			top_color.r += 0.2f;
			top_color.g += 0.2f;
			top_color.b += 0.2f;
			top_color.a = 0.5f;

			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_pct(1, 1));
			UI_Box *container = ui_box_make(UI_BoxFlag_FloatingPos, str8_lit("OverlayBoxContainer"));
			ui_parent(container)
			{
				if (hover_axis == Axis2_COUNT)
				{
					ui_next_width(ui_pct(1, 1));
					ui_next_height(ui_pct(1, 1));
				}
				else
				{
					container->layout_style.child_layout_axis = hover_axis;
					if (hover_side == Side_Max)
					{
						ui_spacer(ui_fill());
					}

					ui_next_size(hover_axis, ui_pct(0.5f, 1));
					ui_next_size(axis_flip(hover_axis), ui_pct(1, 1));
				}

				ui_next_vert_gradient(top_color, top_color);
				ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("OverlayBox"));
			}
		}

		//- hampus: Tab bar

		F32 title_bar_height_em = 1.2f;
		F32 tab_spacing_em = 0.2f;
		F32 tab_button_height_em = title_bar_height_em - 0.2f;;

		UI_Box *title_bar = &g_nil_box;
		UI_Box *tabs_container = &g_nil_box;
		ui_next_width(ui_fill());
		ui_row()
		{
			ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
			ui_next_width(ui_fill());
			ui_next_height(ui_em(title_bar_height_em, 1));
			if (root->tab_group.count == 1)
			{
				ui_next_extra_box_flags(
					UI_BoxFlag_Clickable |
					UI_BoxFlag_HotAnimation |
					UI_BoxFlag_ActiveAnimation);
			}

			ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBar));
			title_bar = ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("TitleBar"));
			ui_parent(title_bar)
			{
				//- hampus: Tab dropdown menu

				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_row()
				{
					UI_Key tab_dropown_menu_key = ui_key_from_string(title_bar->key, str8_lit("TabDropdownMenu"));

					// NOTE(hampus): Calculate the largest tab to decice the dropdown list size
					Vec2F32 largest_dim = v2f32(0, 0);
					if (root->tab_group.count >= 2)
					{
						for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
						{
							Vec2F32 dim = render_measure_text(render_font_from_key(ui_renderer(), ui_top_font_key()), tab->string);
							largest_dim.x = f32_max(largest_dim.x, dim.x);
							largest_dim.y = f32_max(largest_dim.y, dim.y);
						}

						largest_dim.x += ui_top_font_line_height()*0.5f;
						ui_ctx_menu(tab_dropown_menu_key)
						{
							ui_corner_radius(0)
							{
								for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
								{
									ui_next_hover_cursor(Gfx_Cursor_Hand);
									ui_next_extra_box_flags(UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawBackground | UI_BoxFlag_ActiveAnimation | UI_BoxFlag_HotAnimation | UI_BoxFlag_Clickable);
									UI_Box *row_box = ui_named_row_beginf("TabDropDownListEntry%p", tab);
									ui_next_height(ui_em(1, 0.0f));
									ui_next_width(ui_pixels(largest_dim.x, 1));
									// TODO(hampus): Theming
									UI_Box *tab_box = ui_box_make(
										UI_BoxFlag_DrawText,
										str8_lit(""));
									ui_box_equip_display_string(tab_box, tab->string);
									ui_next_height(ui_em(1, 1));
									ui_next_width(ui_em(1, 1));
									ui_next_icon(RENDER_ICON_CROSS);
									ui_next_hover_cursor(Gfx_Cursor_Hand);
									UI_Box *close_box = ui_box_make_f(
										UI_BoxFlag_Clickable |
										UI_BoxFlag_DrawText |
										UI_BoxFlag_HotAnimation |
										UI_BoxFlag_ActiveAnimation,
										"TabCloseButton%p", tab
									);
									UI_Comm close_comm = ui_comm_from_box(close_box);
									if (close_comm.hovering)
									{
										close_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder;

									}
									if (close_comm.clicked)
									{
										framed_ui_attempt_to_close_tab(tab);
									}
									ui_named_row_end();
									UI_Comm row_comm = ui_comm_from_box(row_box);
									if (row_comm.pressed)
									{
										framed_ui_set_tab_to_active(tab);
									}

								}
							}
						}

						ui_next_extra_box_flags(UI_BoxFlag_Clip);
						ui_next_height(ui_pct(1, 1));
						ui_named_column(str8_lit("TabDropDownContainer"))
						{
							F32 corner_radius = (F32) ui_top_font_line_height() * 0.25f;
							ui_spacer(ui_em(0.2f, 1));
							ui_next_icon(RENDER_ICON_LIST);
							ui_next_width(ui_em(title_bar_height_em+0.1f, 1));
							ui_next_height(ui_em(title_bar_height_em+0.1f, 1));
							ui_next_font_size(12);
							ui_next_hover_cursor(Gfx_Cursor_Hand);
							ui_next_vert_corner_radius(corner_radius, 0);
							ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
							UI_Box *tab_dropdown_list_box = ui_box_make(
								UI_BoxFlag_DrawBackground |
								UI_BoxFlag_DrawBorder |
								UI_BoxFlag_HotAnimation |
								UI_BoxFlag_ActiveAnimation |
								UI_BoxFlag_Clickable |
								UI_BoxFlag_DrawText,
								str8_lit("TabDropdownList")
							);
							UI_Comm tab_dropdown_list_comm = ui_comm_from_box(tab_dropdown_list_box);
							if (tab_dropdown_list_comm.pressed)
							{
								ui_ctx_menu_open(tab_dropdown_list_comm.box->key,
																 v2f32(0, -ui_em(0.1f, 1).value), tab_dropown_menu_key);
							}
						}
					}

					ui_spacer(ui_em(tab_spacing_em, 1));

					//- hampus: Tab buttons

					B32 tab_overflow = false;
					ui_next_width(ui_fill());
					ui_next_height(ui_pct(1, 1));
					ui_next_extra_box_flags(UI_BoxFlag_Clip | UI_BoxFlag_AnimateScroll);
					tabs_container = ui_named_row_begin(str8_lit("TabsContainer"));
					{
						FramedUI_Tab *active_tab = root->tab_group.active_tab;

						if (!ui_box_is_nil(root->tab_group.first->tab_container))
						{
							UI_Box *first_tab_box = root->tab_group.first->tab_container;

							UI_Box *active_tab_box = root->tab_group.active_tab->tab_container;

							UI_Box *last_tab_box = root->tab_group.last->tab_container;
							for (FramedUI_Tab *tab = root->tab_group.last; ui_box_is_nil(last_tab_box); tab = tab->prev)
							{
								if (framed_ui_tab_is_nil(tab))
								{
									// NOTE(hampus): Atleast the first tab should have a box
									assert(false);
								}
								last_tab_box = tab->tab_container;
							}
							if (!last_tab_box)
							{
								last_tab_box = first_tab_box;
							}

							F32 end = last_tab_box->rel_pos.x + last_tab_box->fixed_size.x;

							F32 required_tab_bar_width = end - first_tab_box->rel_pos.x;
							tab_overflow = required_tab_bar_width > tabs_container->fixed_size.x && root->tab_group.count >= 2;

							F32 adjustment_for_empty_space = tabs_container->fixed_size.x - (end-tabs_container->scroll.x);
							adjustment_for_empty_space = f32_clamp(0, adjustment_for_empty_space, tabs_container->scroll.x);
							tabs_container->scroll.x -= adjustment_for_empty_space;

							Vec2F32 tab_visiblity_range = v2f32(
								active_tab->tab_container->rel_pos.x,
								active_tab->tab_container->rel_pos.x + active_tab->tab_container->fixed_size.x
							);

							tab_visiblity_range.x = f32_max(0, tab_visiblity_range.x);
							tab_visiblity_range.y = f32_max(0, tab_visiblity_range.y);

							Vec2F32 tab_bar_visiblity_range = v2f32(
								tabs_container->scroll.x,
								tabs_container->scroll.x + tabs_container->fixed_size.x
							);

							F32 delta_left = tab_visiblity_range.x - tab_bar_visiblity_range.x ;
							F32 delta_right = tab_visiblity_range.y - tab_bar_visiblity_range.y;
							delta_left = f32_min(delta_left, 0);
							delta_right = f32_max(delta_right, 0);

							if (tabs_container->fixed_size.x > active_tab_box->fixed_size.x)
							{
								tabs_container->scroll.x += delta_right;
							}
							tabs_container->scroll.x += delta_left;
							tabs_container->scroll.x = f32_max(0, tabs_container->scroll.x);
						}

						// NOTE(hampus): Build tabs

						for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
						{
							ui_next_height(ui_pct(1, 1));
							ui_next_width(ui_children_sum(1));
							ui_next_child_layout_axis(Axis2_Y);
							UI_Box *tab_column = ui_box_make_f(UI_BoxFlag_AnimateX, "TabColumn%p", tab);
							ui_parent(tab_column)
							{
								tab->tab_container = tab_column;
								if (tab != root->tab_group.active_tab)
								{
									ui_spacer(ui_em(0.1f, 1));
								}
								ui_spacer(ui_em(0.2f, 1));
								UI_Box *tab_box = framed_ui_tab_button(tab);
							}
							ui_spacer(ui_em(tab_spacing_em, 1));
						}
					}
					ui_named_row_end();

					// NOTE(hampus): Build prev/next tab buttons

					if (tab_overflow)
					{
						ui_next_height(ui_em(title_bar_height_em, 1));
						ui_next_width(ui_em(title_bar_height_em, 1));
						ui_next_icon(RENDER_ICON_LEFT_OPEN);
						ui_next_hover_cursor(Gfx_Cursor_Hand);
						ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
						UI_Box *prev_tab_button = ui_box_make(
							UI_BoxFlag_Clickable |
							UI_BoxFlag_DrawText |
							UI_BoxFlag_HotAnimation |
							UI_BoxFlag_ActiveAnimation |
							UI_BoxFlag_DrawBackground,
							str8_lit("PrevTabButton")
						);

						UI_Comm prev_tab_comm = ui_comm_from_box(prev_tab_button);
						if (prev_tab_comm.pressed)
						{
							if (!framed_ui_tab_is_nil(root->tab_group.active_tab->prev))
							{
								framed_ui_set_tab_to_active(root->tab_group.active_tab->prev);
							}
							else
							{
								framed_ui_set_tab_to_active(root->tab_group.last);
							}
						}

						ui_next_height(ui_em(title_bar_height_em, 1));
						ui_next_width(ui_em(title_bar_height_em, 1));
						ui_next_icon(RENDER_ICON_RIGHT_OPEN);
						ui_next_hover_cursor(Gfx_Cursor_Hand);
						ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
						UI_Box *next_tab_button = ui_box_make(
							UI_BoxFlag_Clickable |
							UI_BoxFlag_DrawText |
							UI_BoxFlag_HotAnimation |
							UI_BoxFlag_ActiveAnimation |
							UI_BoxFlag_DrawBackground,
							str8_lit("NextTabButton")
						);

						UI_Comm next_tab_comm = ui_comm_from_box(next_tab_button);
						if (next_tab_comm.pressed)
						{
							if (!framed_ui_tab_is_nil(root->tab_group.active_tab->next))
							{
								framed_ui_set_tab_to_active(root->tab_group.active_tab->next);
							}
							else
							{
								framed_ui_set_tab_to_active(root->tab_group.first);
							}
						}
					}

					// NOTE(hampus): Build close tab button

					ui_next_height(ui_em(title_bar_height_em, 1));
					ui_next_width(ui_em(title_bar_height_em, 1));
					ui_next_icon(RENDER_ICON_CROSS);
					ui_next_hover_cursor(Gfx_Cursor_Hand);
					ui_next_color(v4f32(0.6f, 0.1f, 0.1f, 1.0f));
					UI_Box *close_box = ui_box_make(
						UI_BoxFlag_Clickable |
						UI_BoxFlag_DrawText |
						UI_BoxFlag_HotAnimation |
						UI_BoxFlag_ActiveAnimation |
						UI_BoxFlag_DrawBackground,
						str8_lit("CloseButton")
					);
					UI_Comm close_comm = ui_comm_from_box(close_box);
					if (close_comm.hovering)
					{
						ui_tooltip()
						{
							UI_Box *tooltip = ui_box_make(
								UI_BoxFlag_DrawBackground |
								UI_BoxFlag_DrawBorder |
								UI_BoxFlag_DrawDropShadow |
								UI_BoxFlag_DrawText,
								str8_lit("")
							);
							ui_box_equip_display_string(tooltip, str8_lit("Close panel"));
						}
					}

					if (close_comm.clicked)
					{
						framed_ui_attempt_to_close_panel(root);
					}
				}
			}

			if (
				root->tab_group.count == 1 &&
				!framed_ui_is_dragging())
			{
				UI_Comm title_bar_comm = ui_comm_from_box(title_bar);
				if (title_bar_comm.pressed)
				{
					framed_ui_drag_begin_reordering(root->tab_group.active_tab);
					framed_ui_wait_for_drag_threshold();
				}
			}
		}

		//- hampus: Tab content

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_color(framed_ui_color_from_theme(FramedUI_Color_InactivePanelOverlay));
		UI_Box *content_dim = ui_box_make(UI_BoxFlag_FloatingPos, str8_lit("ContentDim"));
		content_dim->flags |= (UI_BoxFlags) (UI_BoxFlag_DrawBackground * (root != framed_ui_state->focused_panel));

		if (
			ui_mouse_is_inside_box(content_dim) &&
			framed_ui_is_tab_reordering() &&
			!ui_mouse_is_inside_box(title_bar))
		{
			framed_ui_wait_for_drag_threshold();
		}


		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		UI_Box *content_box_container = ui_box_make(0, str8_lit("ContentBoxContainer"));
		ui_parent(content_box_container)
		{
			if (root == framed_ui_state->focused_panel)
			{
				ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_ActivePanelBorder));
			}
			else
			{
				ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_InactivePanelBorder));
			}
			ui_next_width(ui_fill());
			ui_next_height(ui_fill());
			ui_next_child_layout_axis(Axis2_Y);
			// TODO(hampus): Should this actually be called panel color...
			ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
			UI_Box *content_box = ui_box_make(
				UI_BoxFlag_DrawBackground |
				UI_BoxFlag_DrawBorder |
				UI_BoxFlag_Clip,
				str8_lit("ContentBox")
			);

			ui_parent(content_box)
			{
				// NOTE(hampus): Add some padding for the content
				UI_Size padding = ui_em(0.3f, 1);
				ui_spacer(padding);
				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_row()
				{
					ui_spacer(padding);
					if (!framed_ui_tab_is_nil(root->tab_group.active_tab))
					{
						FramedUI_Tab *tab = root->tab_group.active_tab;
						ui_next_width(ui_fill());
						ui_next_height(ui_fill());
						ui_next_extra_box_flags(UI_BoxFlag_Clip);
						ui_named_column(str8_lit("PaddedContentBox"))
						{
							tab->view_info.function(&tab->view_info);
						}
						ui_spacer(padding);
					}
				}
				ui_spacer(padding);
			}
		}

		ui_pop_string();
	}

	B32 take_input_from_root = true;
	if (framed_ui_is_dragging())
	{
		if (root == framed_ui_state->drag_data.tab->panel)
		{
			// NOTE(hampus): We don't want to consume all the events
			// if we're dragging a tab in order to make the others panels
			// able to get hovered
			take_input_from_root = false;
		}
	}

	if (take_input_from_root)
	{
		// NOTE(hampus): Consume all non-taken events
		UI_Comm root_comm = ui_comm_from_box(box);
	}
#endif
	ui_pop_parent();
}

////////////////////////////////
//~ hampus: Window

internal Void
framed_ui_window_reorder_to_front(FramedUI_Window *window)
{
	if (window != framed_ui_state->master_window)
	{
		if (!framed_ui_state->next_top_most_window)
		{
			framed_ui_state->next_top_most_window = window;
		}
	}
}

internal Void
framed_ui_window_push_to_front(FramedUI_Window *window)
{
	dll_push_front(framed_ui_state->window_list.first, framed_ui_state->window_list.last, window);
}

internal Void
framed_ui_window_remove_from_list(FramedUI_Window *window)
{
	dll_remove(framed_ui_state->window_list.first, framed_ui_state->window_list.last, window);
}

internal FramedUI_Window *
framed_ui_window_alloc(Arena *arena)
{
	FramedUI_Window *result = push_struct(arena, FramedUI_Window);
	result->string = str8_pushf(arena, "Window%d", framed_ui_state->num_windows++);
	return(result);
}

internal FramedUI_Window *
framed_ui_window_make(Arena *arena, Vec2F32 pos, Vec2F32 size)
{
	FramedUI_Window *result = framed_ui_window_alloc(arena);
	result->root_panel = &g_nil_panel;
	result->box = &g_nil_box;
	FramedUI_Panel *panel = framed_ui_panel_alloc(arena);
	panel->window = result;
	result->size = size;
	result->pos = pos;
	framed_ui_window_push_to_front(result);
	result->root_panel = panel;
	log_info("Allocated panel: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal UI_Comm
framed_ui_window_edge_resizer(FramedUI_Window *window, Str8 string, Axis2 axis, Side side)
{
	ui_next_size(axis, ui_em(0.4f, 1));
	ui_next_size(axis_flip(axis), ui_fill());
	ui_next_hover_cursor(axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
	UI_Box *box = ui_box_make(UI_BoxFlag_Clickable, string);

	Vec2U32 screen_size = gfx_get_window_client_area(ui_renderer()->gfx);
	UI_Comm comm = { 0 };
	if (!framed_ui_is_dragging())
	{
		comm = ui_comm_from_box(box);
		if (comm.dragging)
		{
			F32 drag_delta = comm.drag_delta.v[axis];
			if (side == Side_Min)
			{
				window->pos.v[axis]  -= drag_delta;
				window->size.v[axis] += drag_delta / (F32) screen_size.v[axis];
			}
			else
			{
				window->size.v[axis] -= drag_delta / (F32) screen_size.v[axis];
			}
		}
	}
	return(comm);
}

internal UI_Comm
framed_ui_window_corner_resizer(FramedUI_Window *window, Str8 string, Corner corner)
{
	ui_next_width(ui_em(0.4f, 1));
	ui_next_height(ui_em(0.4f, 1));
	ui_next_hover_cursor(corner == Corner_TopLeft || corner == Corner_BottomRight ?
											 Gfx_Cursor_SizeNWSE :
											 Gfx_Cursor_SizeNESW);
	UI_Box *box = ui_box_make(UI_BoxFlag_Clickable,
														string);
	UI_Comm comm = ui_comm_from_box(box);
	Vec2F32 screen_size   = v2f32_from_v2u32(gfx_get_window_area(ui_renderer()->gfx));
	if (comm.dragging)
	{
		switch (corner)
		{
			case Corner_TopLeft:
			{
				window->pos           = v2f32_sub_v2f32(window->pos, comm.drag_delta);
				window->size          = v2f32_add_v2f32(
					window->size,
					v2f32_hadamard_div_v2f32(comm.drag_delta, screen_size)
				);
			} break;

			case Corner_BottomLeft:
			{
				window->pos.x        -= comm.drag_delta.v[Axis2_X];
				window->size.x       += comm.drag_delta.v[Axis2_X] / screen_size.v[Axis2_X];
				window->size.y       -= comm.drag_delta.v[Axis2_Y] / screen_size.v[Axis2_Y];
			} break;

			case Corner_TopRight:
			{
				window->pos.y -= comm.drag_delta.v[Axis2_Y];
				window->size.x -= comm.drag_delta.v[Axis2_X] / screen_size.v[Axis2_X];
				window->size.y += comm.drag_delta.v[Axis2_Y] / screen_size.v[Axis2_Y];
			} break;

			case Corner_BottomRight:
			{
				window->size = v2f32_sub_v2f32(
					window->size,
					v2f32_hadamard_div_v2f32(comm.drag_delta, screen_size)
				);
			} break;

			invalid_case;
		}
	}

	return(comm);
}

internal Void
framed_ui_update_window(FramedUI_Window *window)
{
	ui_seed(window->string)
	{
		if (window == framed_ui_state->master_window)
		{
			framed_ui_update_panel(window->root_panel);
		}
		else
		{
			Vec2F32 pos = window->pos;
			// NOTE(hampus): Screen pos -> Container pos
			pos = v2f32_sub_v2f32(pos, framed_ui_state->window_container->fixed_rect.min);
			// NOTE(hampus): Container pos -> Window pos
			pos.x -= ui_em(0.4f, 1).value;
			pos.y -= ui_em(0.4f, 1).value;
			ui_next_width(ui_pct(window->size.x, 1));
			ui_next_height(ui_pct(window->size.y, 1));
			ui_next_relative_pos(Axis2_X, pos.v[Axis2_X]);
			ui_next_relative_pos(Axis2_Y, pos.v[Axis2_Y]);
			ui_next_child_layout_axis(Axis2_X);
			ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
			UI_Box *window_container = ui_box_make(UI_BoxFlag_FloatingPos | UI_BoxFlag_DrawDropShadow, str8_lit(""));
			window->box = window_container;
			ui_parent(window_container)
			{
				ui_next_height(ui_fill());
				ui_column()
				{
					framed_ui_window_corner_resizer(window, str8_lit("TopLeftWindowResize"), Corner_TopLeft);
					framed_ui_window_edge_resizer(window, str8_lit("TopWindowResize"), Axis2_X, Side_Min);
					framed_ui_window_corner_resizer(window, str8_lit("BottomLeftWindowResize"), Corner_BottomLeft);
				}

				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_column()
				{
					framed_ui_window_edge_resizer(window, str8_lit("LeftWindowResize"), Axis2_Y, Side_Min);
					framed_ui_update_panel(window->root_panel);
					framed_ui_window_edge_resizer(window, str8_lit("RightWindowResize"), Axis2_Y, Side_Max);
				}

				ui_next_height(ui_fill());
				ui_column()
				{
					framed_ui_window_corner_resizer(window, str8_lit("TopRightWindowResize"), Corner_TopRight);
					framed_ui_window_edge_resizer(window, str8_lit("BottomWindowResize"), Axis2_X, Side_Max);
					framed_ui_window_corner_resizer(window, str8_lit("BottomRightWindowResize"), Corner_BottomRight);
				}
			}
			window->size.v[Axis2_X] = f32_clamp(0.05f, window->size.v[Axis2_X], 1.0f);
			window->size.v[Axis2_Y] = f32_clamp(0.05f, window->size.v[Axis2_Y], 1.0f);
		}
	}
}

#include "framed_ui_commands.c"

////////////////////////////////
//~ hampus: UI startup builder

#define framed_ui_builder_split_panel(panel_to_split, split_axis, ...) framed_ui_builder_split_panel_(&(FramedUI_PanelSplit) { .panel = panel_to_split, .axis = split_axis, __VA_ARGS__})
internal FramedUI_SplitPanelResult
framed_ui_builder_split_panel_(FramedUI_PanelSplit *data)
{
	FramedUI_SplitPanelResult result = { 0 };
	framed_ui_command_panel_split(data);
	result.panels[Side_Min] = data->panel;
	result.panels[Side_Max] = data->panel->sibling;
	return(result);
}

////////////////////////////////
//~ hampus: Tab views

#define framed_ui_get_view_data(view_info, type) framed_ui_get_or_push_view_data_(view_info, sizeof(type))

internal Void *
framed_ui_get_or_push_view_data_(FramedUI_TabViewInfo *view_info, U64 size)
{
	if (view_info->data == 0)
	{
		view_info->data = push_array(framed_ui_state->perm_arena, U8, size);
	}
	Void *result = view_info->data;
	return(result);
}

frame_ui_tab_view(framed_ui_tab_view_default)
{
	FramedUI_Tab *tab = view_info->data;
	FramedUI_Panel *panel = tab->panel;
	FramedUI_Window *window = panel->window;
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_row()
	{
		ui_next_width(ui_pct(1, 0));
		ui_next_height(ui_pct(1, 0));
		ui_column()
		{
			if (ui_button(str8_lit("Split panel X")).pressed)
			{
				framed_ui_panel_split(panel, Axis2_X);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Split panel Y")).pressed)
			{
				framed_ui_panel_split(panel, Axis2_Y);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close panel")).pressed)
			{
				framed_ui_attempt_to_close_panel(panel);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Add tab")).pressed)
			{
				FramedUI_Tab *new_tab = framed_ui_tab_make(framed_ui_state->perm_arena, 0, 0, str8_lit(""));
				framed_ui_panel_attach_tab(panel, new_tab, false);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close tab")).pressed)
			{
				framed_ui_attempt_to_close_tab(tab);
			}
			ui_spacer(ui_em(0.5f, 1));
			ui_row()
			{
				ui_check(&ui_ctx->show_debug_lines, str8_lit("ShowDebugLines"));
				ui_spacer(ui_em(0.5f, 1));
				ui_text(str8_lit("Show debug lines"));
			}
		}
	}
}

////////////////////////////////
//~ hampus: Update

internal Void
framed_ui_update(Render_Context *renderer, Gfx_EventList *event_list)
{
	Vec2F32 mouse_pos = ui_mouse_pos();

	B32 left_mouse_released = false;
	for (Gfx_Event *event = event_list->first; event != 0; event = event->next)
	{
		switch (event->kind)
		{
			case Gfx_EventKind_KeyRelease:
			{
				if (event->key == Gfx_Key_MouseLeft)
				{
					left_mouse_released = true;
				}
			} break;

			default: {} break;
		}
	}

	framed_ui_state->focused_panel = framed_ui_state->next_focused_panel;
	framed_ui_state->next_focused_panel = &g_nil_panel;

	// TODO(hampus): This is really ugly. Fix this.
	if (!framed_ui_tab_is_nil(framed_ui_state->swap_tab0))
	{
		FramedUI_TabSwap data =
		{
			.tab0 = framed_ui_state->swap_tab0,
			.tab1 = framed_ui_state->swap_tab1,
		};
		framed_ui_command_tab_swap(&data);
		framed_ui_state->swap_tab0->tab_container->rel_pos.x = 0;
		framed_ui_state->swap_tab1->tab_container->rel_pos.x = 0;
		framed_ui_state->swap_tab0 = &g_nil_tab;
		framed_ui_state->swap_tab1 = &g_nil_tab;
	}

	//- hampus: Update Windows

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	UI_Box *window_root_parent = ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("RootWindow"));
	framed_ui_state->window_container = window_root_parent;
	ui_parent(window_root_parent)
	{
		for (FramedUI_Window *window = framed_ui_state->window_list.first; window != 0; window = window->next)
		{
			if (!(window->flags & FramedUI_WindowFlags_Closed))
			{
				framed_ui_update_window(window);
			}
		}
	}

	//- hampus: Update tab drag

	if (left_mouse_released &&
			framed_ui_is_dragging())
	{
		framed_ui_drag_release();
	}

	FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
	switch (framed_ui_state->drag_status)
	{
		case FramedUI_DragStatus_Inactive: {} break;

		case FramedUI_DragStatus_Reordering: {} break;

		case FramedUI_DragStatus_WaitingForDragThreshold:
		{
			F32 drag_threshold = ui_em(3, 1).value;
			Vec2F32 delta = v2f32_sub_v2f32(mouse_pos, drag_data->drag_origin);
			if (f32_abs(delta.x) > drag_threshold ||
					f32_abs(delta.y) > drag_threshold)
			{
				FramedUI_Tab *tab = drag_data->tab;

				// NOTE(hampus): Calculate the new window size
				Vec2F32 new_window_pct = v2f32(1, 1);
				FramedUI_Panel *panel_child = tab->panel;
				for (FramedUI_Panel *panel_parent = panel_child->parent; !framed_ui_panel_is_nil(panel_parent); panel_parent = panel_parent->parent)
				{
					Axis2 axis = panel_parent->split_axis;
					new_window_pct.v[axis] *= panel_child->pct_of_parent;
					panel_child = panel_parent;
				}

				new_window_pct.x *= tab->panel->window->size.x;
				new_window_pct.y *= tab->panel->window->size.y;

				FramedUI_Panel *tab_panel = tab->panel;
				B32 create_new_window =
					!(tab->panel == tab->panel->window->root_panel &&
						tab_panel->tab_group.count == 1 &&
						tab_panel->window != framed_ui_state->master_window);

				if (create_new_window)
				{
					// NOTE(hampus): Close the tab from the old panel
					{
						FramedUI_TabDelete tab_close =
						{
							.tab = drag_data->tab
						};
						framed_ui_command_tab_close(&tab_close);
					}

					FramedUI_Window *new_window = framed_ui_window_make(framed_ui_state->perm_arena, v2f32(0, 0), new_window_pct);

					FramedUI_TabAttach tab_attach =
					{
						.tab = drag_data->tab,
						.panel = new_window->root_panel,
						.set_active = true,
					};
					framed_ui_command_tab_attach(&tab_attach);
				}
				else
				{
					drag_data->tab->panel->sibling = &g_nil_panel;
					framed_ui_window_reorder_to_front(drag_data->tab->panel->window);
				}
				
				drag_data->tab->panel->window->pos = ui_mouse_pos();

				framed_ui_state->next_focused_panel = drag_data->tab->panel;
				framed_ui_state->drag_status = FramedUI_DragStatus_Dragging;
				log_info("Drag: dragging");
			}

		} break;

		case FramedUI_DragStatus_Dragging:
		{
			FramedUI_Window *window = drag_data->tab->panel->window;
			Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, ui_prev_mouse_pos());
			window->pos = v2f32_add_v2f32(window->pos, mouse_delta);;
		} break;

		case FramedUI_DragStatus_Released:
		{
			memory_zero_struct(&framed_ui_state->drag_data);
			framed_ui_state->drag_status = FramedUI_DragStatus_Inactive;
		} break;

		invalid_case;
	}

	if (left_mouse_released && framed_ui_state->drag_status != FramedUI_DragStatus_Inactive)
	{
		framed_ui_drag_end();
	}

	ui_end();

	for (U64 i = 0; i < framed_ui_state->cmd_buffer.pos; ++i)
	{
		FramedUI_Command *cmd = framed_ui_state->cmd_buffer.buffer + i;
		switch (cmd->kind)
		{
			case FramedUI_CommandKind_TabAttach:  framed_ui_command_tab_attach(cmd->data); break;
			case FramedUI_CommandKind_TabClose:   framed_ui_command_tab_close(cmd->data); break;
			case FramedUI_CommandKind_TabSwap:    framed_ui_command_tab_swap(cmd->data); break;

			case FramedUI_CommandKind_PanelSplit:          framed_ui_command_panel_split(cmd->data);            break;
			case FramedUI_CommandKind_PanelSplitAndAttach: framed_ui_command_panel_split_and_attach(cmd->data); break;
			case FramedUI_CommandKind_PanelSetActiveTab:   framed_ui_command_panel_set_active_tab(cmd->data);   break;
			case FramedUI_CommandKind_PanelClose:          framed_ui_command_panel_close(cmd->data);            break;

			case FramedUI_CommandKind_WindowRemoveFromList: framed_ui_command_window_remove_from_list(cmd->data); break;
			case FramedUI_CommandKind_WindowPushToFront:    framed_ui_command_window_push_to_front(cmd->data);    break;
		}
	}

	if (framed_ui_state->next_top_most_window)
	{
		FramedUI_Window *window = framed_ui_state->next_top_most_window;
		framed_ui_window_remove_from_list(window);
		framed_ui_window_push_to_front(window);
	}

	if (framed_ui_panel_is_nil(framed_ui_state->next_focused_panel))
	{
		framed_ui_state->next_focused_panel = framed_ui_state->focused_panel;
	}

	framed_ui_state->next_top_most_window = 0;

	framed_ui_state->cmd_buffer.pos = 0;
}
