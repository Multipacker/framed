////////////////////////////////
//~ hampus: Short term
//
// [ ] @feature Tabs
//  [ ] @feature Scroll tabs horizontally if there are too many to fit
//                 - Partially fixed. You can navigate tabs by pressing the arrows to the right
//  [ ] @code @feature UI startup builder
//  [ ] @bug If you begin to reorder a tab, and then drag it out, the offset will be wrong.
//  [ ] @bug Reordering tabs at the end of the tab bar list when the tab bar is full is
//           kinda yuck.

////////////////////////////////
//~ hampus: Medium term
//
// [ ] @feature Only change panel focus with mouse presses
// [ ] @bug Weird flickering on the first appearance of the tab navigation buttons
// [ ] @polish Resizing panels with tab animation doesn't look that good right now.

////////////////////////////////
//~ hampus: Long term
//
// [ ] @bug Close button is rendered even though the tab is outside tab bar
//            - Solved by removing the clip box flag, but this shouldn't solve it
// [ ] @feature Move around windows that have multiple panels
// [ ] @feature Be able to pin windows which disables closing
// [ ] @bug The user can drop a panel on the menu bar which will hide the tab bar
// [ ] @code @cleanup UI commands. Discriminated unions instead of data array?

////////////////////////////////
//~ hampus: Globals

global ProfilerUI_State *profiler_ui_state;

////////////////////////////////
//~ hampus: Theming

internal Vec4F32
profiler_ui_color_from_theme(ProfilerUI_Color color)
{
	Vec4F32 result = profiler_ui_state->theme.colors[color];
	return(result);
}

internal Void
profiler_ui_set_color(ProfilerUI_Color color, Vec4F32 value)
{
	profiler_ui_state->theme.colors[color] = value;
}

internal Str8
profiler_ui_string_from_color(ProfilerUI_Color color)
{
	Str8 result = str8_lit("Null");
	switch (color)
	{
		case ProfilerUI_Color_Panel:                { result = str8_lit("Panel background"); } break;
		case ProfilerUI_Color_InactivePanelBorder:  { result = str8_lit("Inactive panel border"); } break;
		case ProfilerUI_Color_ActivePanelBorder:    { result = str8_lit("Active panel border"); } break;
		case ProfilerUI_Color_InactivePanelOverlay: { result = str8_lit("Inactive panel overlay"); } break;

		case ProfilerUI_Color_TabBar:                { result = str8_lit("Tab bar background"); } break;
		case ProfilerUI_Color_ActiveTab:  { result = str8_lit("Active tab background"); } break;
		case ProfilerUI_Color_InactiveTab:    { result = str8_lit("Inactive tab background"); } break;
		case ProfilerUI_Color_TabTitle:    { result = str8_lit("Tab foreground"); } break;
		case ProfilerUI_Color_TabBorder: { result = str8_lit("Tab border"); } break;
		case ProfilerUI_Color_TabBarButtons: { result = str8_lit("Tab bar buttons"); } break;

		invalid_case;
	}
	return(result);
}

////////////////////////////////
//~ hampus: Command

internal Void *
profiler_ui_command_push(ProfilerUI_CommandBuffer *buffer, ProfilerUI_CommandKind kind)
{
	assert(buffer->pos < buffer->size);
	ProfilerUI_Command *result = buffer->buffer + buffer->pos;
	memory_zero_struct(result);
	result->kind = kind;
	buffer->pos++;
	return(result->data);
}

////////////////////////////////
//~ hampus: Command helpers

internal Void
profiler_ui_attempt_to_close_tab(ProfilerUI_Tab *tab)
{
	if (!tab->pinned)
	{
		ProfilerUI_TabDelete *data = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_TabClose);
		data->tab = tab;
	}
}

internal Void
profiler_ui_panel_attach_tab(ProfilerUI_Panel *panel, ProfilerUI_Tab *tab, B32 set_active)
{
	ProfilerUI_TabAttach *data  = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_TabAttach);
	data->tab        = tab;
	data->panel      = panel;
	data->set_active = set_active;
}

internal Void
profiler_ui_panel_split(ProfilerUI_Panel *first, Axis2 split_axis)
{
	ProfilerUI_PanelSplit *data = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_PanelSplit);
	data->panel = first;
	data->axis = split_axis;
}

internal Void
profiler_ui_panel_split_and_attach_tab(ProfilerUI_Panel *panel, ProfilerUI_Tab *tab, Axis2 axis, Side side)
{
	ProfilerUI_PanelSplitAndAttach *data  = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_PanelSplitAndAttach);
	data->tab           = tab;
	data->panel         = panel;
	data->axis          = axis;
	data->panel_side    = side;
}

internal Void
profiler_ui_reorder_tab_in_front(ProfilerUI_Tab *tab, ProfilerUI_Tab *next)
{
	ProfilerUI_TabReorder *data = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_TabReorder);
	data->tab = tab;
	data->next = next;
}

internal Void
profiler_ui_set_tab_to_active(ProfilerUI_Tab *tab)
{
	ProfilerUI_PanelSetActiveTab *data = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_PanelSetActiveTab);
	data->tab = tab;
	data->panel = tab->panel;
}

internal Void
profiler_ui_attempt_to_close_panel(ProfilerUI_Panel *panel)
{
	B32 any_tab_pinned = false;
	for (ProfilerUI_Tab *tab = panel->tab_group.first;
		 tab != 0;
		 tab = tab->next)
	{
		if (tab->pinned)
		{
			any_tab_pinned = true;
			break;
		}
	}
	if (!any_tab_pinned)
	{
		ProfilerUI_PanelClose *data = profiler_ui_command_push(&profiler_ui_state->cmd_buffer, ProfilerUI_CommandKind_PanelClose);
		data->panel = panel;
	}
}

////////////////////////////////
//~ hampus: Tab dragging

internal Void
profiler_ui_drag_begin_reordering(ProfilerUI_Tab *tab, Vec2F32 mouse_offset)
{
	if (!tab->pinned)
	{
		ProfilerUI_DragData *drag_data = &profiler_ui_state->drag_data;
		drag_data->tab = tab;
		drag_data->drag_origin = ui_mouse_pos();
		profiler_ui_state->drag_status = ProfilerUI_DragStatus_Reordering;
		log_info("Drag: reordering");
	}
}

internal Void
profiler_ui_wait_for_drag_threshold(Void)
{
	profiler_ui_state->drag_status = ProfilerUI_DragStatus_WaitingForDragThreshold;
	log_info("Drag: waiting for drag threshold");
}

internal Void
profiler_ui_drag_release(Void)
{
	profiler_ui_state->drag_status = ProfilerUI_DragStatus_Released;
	log_info("Drag: released");
}

internal Void
profiler_ui_drag_end(Void)
{
	profiler_ui_state->drag_status = ProfilerUI_DragStatus_Inactive;
	memory_zero_struct(&profiler_ui_state->drag_data);
	log_info("Drag: end");
}

internal B32
profiler_ui_is_dragging(Void)
{
	B32 result = profiler_ui_state->drag_status == ProfilerUI_DragStatus_Dragging;
	return(result);
}

internal B32
profiler_ui_is_tab_reordering(Void)
{
	B32 result = profiler_ui_state->drag_status == ProfilerUI_DragStatus_Reordering;
	return(result);
}

internal B32
profiler_ui_is_waiting_for_drag_threshold(Void)
{
	B32 result = profiler_ui_state->drag_status == ProfilerUI_DragStatus_WaitingForDragThreshold;
	return(result);
}

internal B32
profiler_ui_drag_is_inactive(Void)
{
	B32 result = profiler_ui_state->drag_status == ProfilerUI_DragStatus_Inactive;
	return(result);
}

////////////////////////////////
//~ hampus: Tabs

internal ProfilerUI_Tab *
profiler_ui_tab_alloc(Arena *arena)
{
	ProfilerUI_Tab *result = push_struct(arena, ProfilerUI_Tab);
	return(result);
}

internal Void
profiler_ui_tab_equip_view_info(ProfilerUI_Tab *tab, ProfilerUI_TabViewInfo view_info)
{
	tab->view_info = view_info;
}

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_default);
internal ProfilerUI_Tab *
profiler_ui_tab_make(Arena *arena, ProfilerUI_TabViewProc *function, Void *data, Str8 name)
{
	ProfilerUI_Tab *result = profiler_ui_tab_alloc(arena);
	if (name.size == 0)
	{
		// NOTE(hampus): We probably won't do this in the future because
		// you won't probably be able to have unnamed tabs.
		result->string = str8_pushf(profiler_ui_state->perm_arena, "Tab%"PRIS32, profiler_ui_state->num_tabs);
	}
	else
	{
		result->string = str8_copy(profiler_ui_state->perm_arena, name);
	}
	// TODO(hampus): Check for name collision with other tabs
	ProfilerUI_TabViewInfo view_info = {function, data};
	profiler_ui_tab_equip_view_info(result, view_info);
	if (!function)
	{
		// NOTE(hampus): Equip with default view function
		result->view_info.function = profiler_ui_tab_view_default;
		result->view_info.data = result;
	}
	profiler_ui_state->num_tabs++;
	log_info("Allocated tab: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal B32
profiler_ui_tab_is_active(ProfilerUI_Tab *tab)
{
	B32 result = tab->panel->tab_group.active_tab == tab;
	return(result);
}

internal B32
profiler_ui_tab_is_dragged(ProfilerUI_Tab *tab)
{
	B32 result = false;
	if (profiler_ui_state->drag_status == ProfilerUI_DragStatus_Dragging)
	{
		if (profiler_ui_state->drag_data.tab == tab)
		{
			result = true;
		}
	}
	return(result);
}

internal UI_Box *
profiler_ui_tab_button(ProfilerUI_Tab *tab)
{
	assert(tab->frame_index != profiler_ui_state->frame_index);
	tab->frame_index = profiler_ui_state->frame_index;
	ui_push_string(tab->string);

	B32 active = profiler_ui_tab_is_active(tab);

	F32 height_em = active ? 1.2f : 1.1f;
	F32 corner_radius = (F32) ui_top_font_size() * 0.5f;

	Vec4F32 color = active ?
		profiler_ui_color_from_theme(ProfilerUI_Color_ActiveTab) :
	profiler_ui_color_from_theme(ProfilerUI_Color_InactiveTab);
	ui_next_color(color);
	ui_next_border_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBorder));
	ui_next_vert_corner_radius(corner_radius, 0);
	ui_next_child_layout_axis(Axis2_X);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_em(height_em, 1));
	ui_next_hover_cursor(Gfx_Cursor_Hand);
	UI_Box *title_container = ui_box_make(UI_BoxFlag_DrawBackground |
										  UI_BoxFlag_HotAnimation |
										  UI_BoxFlag_ActiveAnimation |
										  UI_BoxFlag_Clickable  |
										  UI_BoxFlag_DrawBorder |
										  UI_BoxFlag_AnimateY,
										  str8_lit("TitleContainer"));

	tab->box = title_container;
	ui_parent(title_container)
	{
		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_PIN);
		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(corner_radius, 0, 0, 0);
		ui_next_border_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBorder));
		ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_InactiveTab));
		UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
		if (tab->pinned)
		{
			pin_box_flags |= UI_BoxFlag_DrawText;
		}
		UI_Box *pin_box = ui_box_make(pin_box_flags,
									  str8_lit("PinButton"));

		ui_next_text_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabTitle));
		UI_Box *title = ui_box_make(UI_BoxFlag_DrawText,
									tab->string);

		ui_box_equip_display_string(title, tab->string);

		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_CROSS);
		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(0, corner_radius, 0, 0);
		ui_next_border_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBorder));
		ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_InactiveTab));
		UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable,
										str8_lit("CloseButton"));
		// TODO(hampus): We shouldn't need to do this here
		// since there shouldn't even be any input events
		// left in the queue if dragging is ocurring.
		if (profiler_ui_is_tab_reordering())
		{
			assert(profiler_ui_state->drag_data.tab);
			if (tab != profiler_ui_state->drag_data.tab)
			{
				RectF32 rect = ui_box_get_fixed_rect(title_container->parent);
				B32 hovered  = ui_mouse_is_inside_rect(rect);
				if (hovered)
				{
					if (profiler_ui_state->drag_data.tab == tab->next)
					{
						profiler_ui_reorder_tab_in_front(profiler_ui_state->drag_data.tab, tab);
					}
					else
					{
						profiler_ui_reorder_tab_in_front(tab, profiler_ui_state->drag_data.tab);
					}
					profiler_ui_drag_end();
				}
			}
		}

		if (!profiler_ui_is_dragging())
		{
			UI_Comm pin_box_comm   = ui_comm_from_box(pin_box);
			UI_Comm close_box_comm = {0};
			if (!tab->pinned)
			{
				close_box_comm = ui_comm_from_box(close_box);
			}

			UI_Comm title_comm = ui_comm_from_box(title_container);
			if (title_comm.pressed)
			{
				if (!tab->pinned)
				{
					profiler_ui_drag_begin_reordering(tab, title_comm.rel_mouse);
				}

				profiler_ui_set_tab_to_active(tab);
			}

			// NOTE(hampus): Icon appearance
			UI_BoxFlags icon_hover_flags = UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation | UI_BoxFlag_DrawText;

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

			if (close_box_comm.hovering ||
				pin_box_comm.hovering ||
				title_comm.hovering)
			{
				pin_box->flags   |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
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
				profiler_ui_attempt_to_close_tab(tab);
			}
		}
	}
	ui_pop_string();
	return(title_container);
}

////////////////////////////////
//~ hampus: Panels

internal ProfilerUI_Panel *
profiler_ui_panel_alloc(Arena *arena)
{
	ProfilerUI_Panel *result = push_struct(arena, ProfilerUI_Panel);
	result->string = str8_pushf(profiler_ui_state->perm_arena, "UI_Panel%"PRIS32, profiler_ui_state->num_panels);
	profiler_ui_state->num_panels++;
	log_info("Allocated panel: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal Side
profiler_ui_get_panel_side(ProfilerUI_Panel *panel)
{
	assert(panel->parent);
	Side result = panel->parent->children[Side_Min] == panel ? Side_Min : Side_Max;
	assert(panel->parent->children[result] == panel);
	return(result);
}

internal B32
profiler_ui_panel_is_leaf(ProfilerUI_Panel *panel)
{
	B32 result = !(panel->children[0] &&  panel->children[1]);
	return(result);
}

internal UI_Comm
profiler_ui_hover_panel_type(Str8 string, F32 width_in_em, ProfilerUI_Panel *root, Axis2 axis, B32 center, Side side)
{
	ui_next_width(ui_em(width_in_em, 1));
	ui_next_height(ui_em(width_in_em, 1));
	ui_next_color(ui_top_text_style()->color);

	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
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
					ui_box_make(UI_BoxFlag_DrawBorder |
								UI_BoxFlag_HotAnimation,
								str8_lit("LeftLeft"));

					ui_next_border_thickness(2);
					ui_box_make(UI_BoxFlag_DrawBorder |
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
profiler_ui_update_panel(ProfilerUI_Panel *root)
{
	assert(root->frame_index != profiler_ui_state->frame_index);
	root->frame_index = profiler_ui_state->frame_index;
	switch (root->split_axis)
	{
		case Axis2_X: ui_next_child_layout_axis(Axis2_X); break;
		case Axis2_Y: ui_next_child_layout_axis(Axis2_Y); break;
		invalid_case;
	}

	ProfilerUI_Panel *parent = root->parent;

	if (parent)
	{
		Axis2 flipped_split_axis = !parent->split_axis;
		ui_next_size(parent->split_axis, ui_pct(root->pct_of_parent, 0));
		ui_next_size(flipped_split_axis, ui_fill());
	}
	else
	{
		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
	}

	// NOTE(hampus): It is clickable so that it can consume
	// all the click events at the end to prevent boxes
	// from behind it to take click events
	ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_Panel));
	UI_Box *box = ui_box_make(UI_BoxFlag_Clip |
							  UI_BoxFlag_Clickable |
							  UI_BoxFlag_DrawBackground,
							  root->string);
	root->box = box;

	ui_push_parent(box);

	if (!profiler_ui_panel_is_leaf(root))
	{
		ProfilerUI_Panel *child0 = root->children[Side_Min];
		ProfilerUI_Panel *child1 = root->children[Side_Max];

		profiler_ui_update_panel(child0);

		B32 dragging = false;
		F32 drag_delta = 0;

		ui_seed(root->string)
		{
			ui_next_size(root->split_axis, ui_em(0.2f, 1));
			ui_next_size(!root->split_axis, ui_pct(1, 1));
			ui_next_corner_radius(0);
			ui_next_hover_cursor(root->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
			ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_Panel));
			UI_Box *draggable_box = ui_box_make(UI_BoxFlag_Clickable |
												UI_BoxFlag_DrawBackground,
												str8_lit("DraggableBox"));
			root->dragger = draggable_box;
			UI_Comm comm = ui_comm_from_box(draggable_box);
			if (comm.dragging)
			{
				dragging = true;
				drag_delta = comm.drag_delta.v[root->split_axis];
				profiler_ui_state->resizing_panel = root;
			}
		}

		profiler_ui_update_panel(child1);

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
			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_pct(1, 1));
			UI_Box *input_detector = ui_box_make(UI_BoxFlag_FloatingPos,
												 str8_lit("InputDetector"));
			B32 make_window_topmost = false;
			Gfx_EventList *event_list = g_ui_ctx->event_list;
			if (ui_mouse_is_inside_box(input_detector))
			{
				for (Gfx_Event *node = event_list->first;
					 node != 0;
					 node = node->next)
				{
					if (node->kind == Gfx_EventKind_KeyPress &&
						(node->key == Gfx_Key_MouseLeft ||
						 node->key == Gfx_Key_MouseRight ||
						 node->key == Gfx_Key_MouseMiddle))
					{
						if (!profiler_ui_state->next_focused_panel)
						{
							profiler_ui_state->next_focused_panel = root;
						}
						if (root->window != profiler_ui_state->master_window &&
							root->window != profiler_ui_state->window_list.first &&
							!profiler_ui_is_dragging())
						{
							profiler_ui_window_reorder_to_front(root->window);
						}
					}
				}
			}
		}

		// NOTE(hampus): Axis2_COUNT is the center
		Axis2   hover_axis = Axis2_COUNT;
		Side    hover_side = 0;
		UI_Comm tab_release_comms[ProfilerUI_TabReleaseKind_COUNT] = {0};
		B32     hovering_any_symbols = false;

		//- hampus: Drag & split symbols

		if (profiler_ui_is_dragging())
		{
			ProfilerUI_DragData *drag_data = &profiler_ui_state->drag_data;
			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_pct(1, 1));
			UI_Box *split_symbols_container = ui_box_make(UI_BoxFlag_FloatingPos,
														  str8_lit("SplitSymbolsContainer"));
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
						tab_release_comms[ProfilerUI_TabReleaseKind_Top] = profiler_ui_hover_panel_type(str8_lit("TabReleaseTop"), size, root, Axis2_Y, false,
																										Side_Min);

						ui_spacer(ui_fill());
					}
					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[ProfilerUI_TabReleaseKind_Left] = profiler_ui_hover_panel_type(str8_lit("TabReleaseLeft"), size, root, Axis2_X, false, Side_Min);

						ui_spacer(ui_em(1, 1));

						tab_release_comms[ProfilerUI_TabReleaseKind_Center] = profiler_ui_hover_panel_type(str8_lit("TabReleaseCenter"), size, root, Axis2_X, true, Side_Min);

						ui_spacer(ui_em(1, 1));

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[ProfilerUI_TabReleaseKind_Right] = profiler_ui_hover_panel_type(str8_lit("TabReleaseRight"), size, root, Axis2_X, false, Side_Max);

						ui_spacer(ui_fill());
					}

					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());
						tab_release_comms[ProfilerUI_TabReleaseKind_Bottom] = profiler_ui_hover_panel_type(str8_lit("TabReleaseBottom"), size, root, Axis2_Y, false, Side_Max);
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

			for (ProfilerUI_TabReleaseKind i = (ProfilerUI_TabReleaseKind) 0;
				 i < ProfilerUI_TabReleaseKind_COUNT;
				 ++i)
			{
				UI_Comm *comm = tab_release_comms + i;
				if (comm->hovering)
				{
					hovering_any_symbols = true;
					switch (i)
					{
						case ProfilerUI_TabReleaseKind_Center:
						{
							hover_axis = Axis2_COUNT;
						} break;

						case ProfilerUI_TabReleaseKind_Left:
						case ProfilerUI_TabReleaseKind_Right:
						{
							hover_axis = Axis2_X;
							hover_side = i == ProfilerUI_TabReleaseKind_Left ? Side_Min : Side_Max;
						} break;

						case ProfilerUI_TabReleaseKind_Top:
						case ProfilerUI_TabReleaseKind_Bottom:
						{
							hover_axis = Axis2_Y;
							hover_side = i == ProfilerUI_TabReleaseKind_Top ? Side_Min : Side_Max;
						} break;

						invalid_case;
					}
				}
				if (comm->released)
				{
					switch (i)
					{
						case ProfilerUI_TabReleaseKind_Center:
						{
							profiler_ui_panel_attach_tab(root, drag_data->tab, true);
							dll_remove(profiler_ui_state->window_list.first,
									   profiler_ui_state->window_list.last,
									   drag_data->tab->panel->window);
							profiler_ui_state->drag_status = ProfilerUI_DragStatus_Released;
						} break;

						case ProfilerUI_TabReleaseKind_Left:
						case ProfilerUI_TabReleaseKind_Right:
						case ProfilerUI_TabReleaseKind_Top:
						case ProfilerUI_TabReleaseKind_Bottom:
						{
							profiler_ui_panel_split_and_attach_tab(root, drag_data->tab, hover_axis, hover_side);
							dll_remove(profiler_ui_state->window_list.first,
									   profiler_ui_state->window_list.last,
									   drag_data->tab->panel->window);
							profiler_ui_state->drag_status = ProfilerUI_DragStatus_Released;
						} break;

						invalid_case;
					}
					profiler_ui_drag_release();
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
			UI_Box *container = ui_box_make(UI_BoxFlag_FloatingPos,
											str8_lit("OverlayBoxContainer"));
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
				ui_box_make(UI_BoxFlag_DrawBackground,
							str8_lit("OverlayBox"));
			}
		}

		//- hampus: Tab bar

		F32 title_bar_height_em = 1.2f;
		F32 tab_spacing_em = 0.2f;
		F32 tab_button_height_em = title_bar_height_em - 0.2f;;

		UI_Box *title_bar = 0;
		UI_Box *tabs_container = 0;
		ui_next_width(ui_fill());
		ui_row()
		{
			ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
			ui_next_width(ui_fill());
			ui_next_height(ui_em(title_bar_height_em, 1));
			if (root->tab_group.count == 1)
			{
				ui_next_extra_box_flags(UI_BoxFlag_Clickable |
										UI_BoxFlag_HotAnimation |
										UI_BoxFlag_ActiveAnimation);
			}

			ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBar));
			title_bar = ui_box_make(UI_BoxFlag_DrawBackground,
									str8_lit("TitleBar"));
			ui_parent(title_bar)
			{

				//- hampus: Tab dropdown menu

				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_row()
				{
					UI_Key tab_dropown_menu_key = ui_key_from_string(title_bar->key, str8_lit("TabDropdownMenu"));

					ui_ctx_menu(tab_dropown_menu_key)
						ui_width(ui_em(4, 1))
					{

						ui_corner_radius(0)
						{
							for (ProfilerUI_Tab *tab = root->tab_group.first;
								 tab != 0;
								 tab = tab->next)
							{
								ui_next_hover_cursor(Gfx_Cursor_Hand);
								ui_next_height(ui_em(1, 0.0f));
								// TODO(hampus): Theming
								UI_Box *tab_box = ui_box_make(UI_BoxFlag_DrawText |
															  UI_BoxFlag_DrawBorder |
															  UI_BoxFlag_HotAnimation |
															  UI_BoxFlag_ActiveAnimation |
															  UI_BoxFlag_Clickable |
															  UI_BoxFlag_DrawBackground,
															  tab->string);
								UI_Comm tab_comm = ui_comm_from_box(tab_box);
								if (tab_comm.pressed)
								{
									profiler_ui_set_tab_to_active(tab);
								}
							}
						}
					}

					ui_next_extra_box_flags(UI_BoxFlag_Clip);
					ui_next_height(ui_pct(1, 1));
					ui_named_column(str8_lit("TabDropDownContainer"))
					{
						F32 corner_radius = (F32) ui_top_font_size() * 0.25f;
						ui_spacer(ui_em(0.2f, 1));
						ui_next_icon(RENDER_ICON_LIST);
						ui_next_width(ui_em(title_bar_height_em+0.1f, 1));
						ui_next_height(ui_em(title_bar_height_em+0.1f, 1));
						ui_next_font_size(12);
						ui_next_hover_cursor(Gfx_Cursor_Hand);
						ui_next_vert_corner_radius(corner_radius, 0);
						ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBarButtons));
						UI_Box *tab_dropdown_list_box = ui_box_make(UI_BoxFlag_DrawBackground |
																	UI_BoxFlag_DrawBorder |
																	UI_BoxFlag_HotAnimation |
																	UI_BoxFlag_ActiveAnimation |
																	UI_BoxFlag_Clickable |
																	UI_BoxFlag_DrawText,
																	str8_lit("TabDropdownList"));
						UI_Comm tab_dropdown_list_comm = ui_comm_from_box(tab_dropdown_list_box);
						if (tab_dropdown_list_comm.pressed)
						{
							ui_ctx_menu_open(tab_dropdown_list_comm.box->key,
											 v2f32(0, -ui_em(0.1f, 1).value), tab_dropown_menu_key);
						}
					}
					ui_spacer(ui_em(tab_spacing_em, 1));

					//- hampus: Tab buttons

					B32 tab_overflow = false;
					ui_next_width(ui_fill());
					ui_next_height(ui_pct(1, 1));
					ui_next_extra_box_flags(UI_BoxFlag_Clip);
					tabs_container = ui_named_row_begin(str8_lit("TabsContainer"));
					{
						ProfilerUI_Tab *active_tab = root->tab_group.active_tab;
						if (root->tab_group.count > 1 &&
							root->tab_group.first->box &&
							root->tab_group.last->box)
						{
							// NOTE(hampus): Calculate required tab bar offset for
							// viewing the active tab group

							RectF32 active_tab_rect = ui_box_get_fixed_rect(active_tab->box->parent);
							RectF32 first_tab_rect  = ui_box_get_fixed_rect(root->tab_group.first->box->parent);
							RectF32 tab_bar_rect    = tabs_container->fixed_rect;

							ProfilerUI_Tab *last_tab_with_size = root->tab_group.last;
							while (ui_box_created_this_frame(last_tab_with_size->box->parent))
							{
								if (last_tab_with_size == root->tab_group.first)
								{
									break;
								}
								last_tab_with_size = last_tab_with_size->prev;
							}

							F32 start = first_tab_rect.x0;
							F32 end   = first_tab_rect.x1;

							F32 active_tab_width = active_tab_rect.x1 - active_tab_rect.x0;
							F32 available_tab_bar_width = tab_bar_rect.x1 - tab_bar_rect.x0;
							F32 required_tab_bar_width = end - start;
							if (last_tab_with_size != root->tab_group.first)
							{
								RectF32 last_tab_rect = ui_box_get_fixed_rect(last_tab_with_size->box->parent);
								end = last_tab_rect.x1;
								required_tab_bar_width = end - start;
								if (required_tab_bar_width > available_tab_bar_width)
								{
									tab_overflow = true;
								}
							}

							B32 active_tab_is_visible =
								active_tab_rect.x0 >= tab_bar_rect.x0 &&
								active_tab_rect.x1 < tab_bar_rect.x1;
							if (!active_tab_is_visible)
							{
								B32 overflow_left  = active_tab_rect.x0 < tab_bar_rect.x0;
								B32 overflow_right = active_tab_rect.x1 >= tab_bar_rect.x1;
								if (available_tab_bar_width > active_tab_width)
								{
									if (overflow_right)
									{
										root->tab_group.view_offset_x -= active_tab_rect.x1 - tab_bar_rect.x1;
									}
									else if (overflow_left)
									{
										root->tab_group.view_offset_x -= active_tab_rect.x0 - tab_bar_rect.x0;
									}
								}
								else
								{
									root->tab_group.view_offset_x -= active_tab_rect.x0 - tab_bar_rect.x0;
								}
							}

							root->tab_group.view_offset_x = f32_min(0, root->tab_group.view_offset_x);

							if (required_tab_bar_width > available_tab_bar_width)
							{
								if (root->tab_group.view_offset_x < (available_tab_bar_width - required_tab_bar_width))
								{
									root->tab_group.view_offset_x = available_tab_bar_width - required_tab_bar_width;
								}
							}
						}

						ui_spacer(ui_pixels(root->tab_group.view_offset_x, 1));

						// NOTE(hampus): Build tabs

						for (ProfilerUI_Tab *tab = root->tab_group.first;
							 tab != 0;
							 tab = tab->next)
						{
							ui_next_height(ui_pct(1, 1));
							ui_next_width(ui_children_sum(1));
							ui_next_child_layout_axis(Axis2_Y);
							UI_Box *tab_column = ui_box_make_f(UI_BoxFlag_AnimateX,
															   "TabColumn%p", tab);
							ui_parent(tab_column)
							{
								if (tab != root->tab_group.active_tab)
								{
									ui_spacer(ui_em(0.1f, 1));
								}
								ui_spacer(ui_em(0.1f, 1));
								UI_Box *tab_box = profiler_ui_tab_button(tab);
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
						ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBarButtons));
						UI_Box *prev_tab_button = ui_box_make(UI_BoxFlag_Clickable |
															  UI_BoxFlag_DrawText |
															  UI_BoxFlag_HotAnimation |
															  UI_BoxFlag_ActiveAnimation |
															  UI_BoxFlag_DrawBackground,
															  str8_lit("PrevTabButton"));

						UI_Comm prev_tab_comm = ui_comm_from_box(prev_tab_button);
						if (prev_tab_comm.pressed)
						{
							if (root->tab_group.active_tab->prev)
							{
								profiler_ui_set_tab_to_active(root->tab_group.active_tab->prev);
							}
							else
							{
								profiler_ui_set_tab_to_active(root->tab_group.last);
							}
						}

						ui_next_height(ui_em(title_bar_height_em, 1));
						ui_next_width(ui_em(title_bar_height_em, 1));
						ui_next_icon(RENDER_ICON_RIGHT_OPEN);
						ui_next_hover_cursor(Gfx_Cursor_Hand);
						ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_TabBarButtons));
						UI_Box *next_tab_button = ui_box_make(UI_BoxFlag_Clickable |
															  UI_BoxFlag_DrawText |
															  UI_BoxFlag_HotAnimation |
															  UI_BoxFlag_ActiveAnimation |
															  UI_BoxFlag_DrawBackground,
															  str8_lit("NextTabButton"));

						UI_Comm next_tab_comm = ui_comm_from_box(next_tab_button);
						if (next_tab_comm.pressed)
						{
							if (root->tab_group.active_tab->next)
							{
								profiler_ui_set_tab_to_active(root->tab_group.active_tab->next);
							}
							else
							{
								profiler_ui_set_tab_to_active(root->tab_group.first);
							}
						}
					}

					// NOTE(hampus): Build close tab button

					ui_next_height(ui_em(title_bar_height_em, 1));
					ui_next_width(ui_em(title_bar_height_em, 1));
					ui_next_icon(RENDER_ICON_CROSS);
					ui_next_hover_cursor(Gfx_Cursor_Hand);
					ui_next_color(v4f32(0.6f, 0.1f, 0.1f, 1.0f));
					UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable |
													UI_BoxFlag_DrawText |
													UI_BoxFlag_HotAnimation |
													UI_BoxFlag_ActiveAnimation |
													UI_BoxFlag_DrawBackground,
													str8_lit("CloseButton"));
					UI_Comm close_comm = ui_comm_from_box(close_box);
					if (close_comm.hovering)
					{
						ui_tooltip()
						{
							UI_Box *tooltip = ui_box_make(UI_BoxFlag_DrawBackground |
														  UI_BoxFlag_DrawBorder |
														  UI_BoxFlag_DrawDropShadow |
														  UI_BoxFlag_DrawText,
														  str8_lit(""));
							ui_box_equip_display_string(tooltip, str8_lit("Close panel"));
						}
					}

					if (close_comm.clicked)
					{
						profiler_ui_attempt_to_close_panel(root);
					}
				}
			}

			if (root->tab_group.count == 1 &&
				!profiler_ui_is_dragging())
			{
				UI_Comm title_bar_comm = ui_comm_from_box(title_bar);
				if (title_bar_comm.pressed)
				{
					profiler_ui_drag_begin_reordering(root->tab_group.active_tab, title_bar_comm.rel_mouse);
					profiler_ui_wait_for_drag_threshold();
				}
			}
		}

		//- hampus: Tab content


		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_InactivePanelOverlay));
		UI_Box *content_dim = ui_box_make(UI_BoxFlag_FloatingPos,
										  str8_lit("ContentDim"));
		content_dim->flags |= UI_BoxFlag_DrawBackground * (root != profiler_ui_state->focused_panel);

		if (ui_mouse_is_inside_box(content_dim) &&
			profiler_ui_is_tab_reordering() &&
			!ui_mouse_is_inside_box(title_bar))
		{
			profiler_ui_wait_for_drag_threshold();
		}

		if (root->tab_group.active_tab)
		{
			ui_next_width(ui_fill());
			ui_next_height(ui_fill());
			UI_Box *content_box_container = ui_box_make(0,
														str8_lit("ContentBoxContainer"));
			ui_parent(content_box_container)
			{
				if (root == profiler_ui_state->focused_panel)
				{
					ui_next_border_color(profiler_ui_color_from_theme(ProfilerUI_Color_ActivePanelBorder));
				}
				else
				{
					ui_next_border_color(profiler_ui_color_from_theme(ProfilerUI_Color_InactivePanelBorder));
				}
				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_next_child_layout_axis(Axis2_Y);
				// TODO(hampus): Should this actually be called panel color...
				ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_Panel));
				UI_Box *content_box = ui_box_make(UI_BoxFlag_DrawBackground |
												  UI_BoxFlag_DrawBorder,
												  str8_lit("ContentBox"));
				// NOTE(hampus): Add some padding for the content
				ui_parent(content_box)
				{
					UI_Size padding = ui_em(0.3f, 1);
					ui_spacer(padding);
					ui_next_width(ui_fill());
					ui_next_height(ui_fill());
					ui_row()
					{
						ui_spacer(padding);
						ProfilerUI_Tab *tab = root->tab_group.active_tab;
						tab->view_info.function(tab->view_info.data);
						ui_spacer(padding);
					}
					ui_spacer(padding);
				}
			}
		}

		ui_pop_string();
	}

	B32 take_input_from_root = true;
	if (profiler_ui_is_dragging())
	{
		if (root == profiler_ui_state->drag_data.tab->panel)
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

	ui_pop_parent();
}

////////////////////////////////
//~ hampus: Window

internal Void
profiler_ui_window_reorder_to_front(ProfilerUI_Window *window)
{
	if (window != profiler_ui_state->master_window)
	{
		if (!profiler_ui_state->next_top_most_window)
		{
			profiler_ui_state->next_top_most_window = window;
		}
	}
}

internal Void
profiler_ui_window_push_to_front(ProfilerUI_Window *window)
{
	dll_push_front(profiler_ui_state->window_list.first, profiler_ui_state->window_list.last, window);
}

internal Void
profiler_ui_window_remove_from_list(ProfilerUI_Window *window)
{
	dll_remove(profiler_ui_state->window_list.first, profiler_ui_state->window_list.last, window);
}

internal ProfilerUI_Window *
profiler_ui_window_alloc(Arena *arena)
{
	ProfilerUI_Window *result = push_struct(arena, ProfilerUI_Window);
	result->string = str8_pushf(arena, "Window%d", profiler_ui_state->num_windows++);
	return(result);
}

internal ProfilerUI_Window *
profiler_ui_window_make(Arena *arena, Vec2F32 size)
{
	ProfilerUI_Window *result = profiler_ui_window_alloc(arena);
	ProfilerUI_Panel *panel = profiler_ui_panel_alloc(arena);
	panel->window = result;
	result->size = size;
	profiler_ui_window_push_to_front(result);
	result->root_panel = panel;
	log_info("Allocated panel: %"PRISTR8, str8_expand(result->string));
	return(result);
}

internal UI_Comm
profiler_ui_window_edge_resizer(ProfilerUI_Window *window, Str8 string, Axis2 axis, Side side)
{
	ui_next_size(axis, ui_em(0.4f, 1));
	ui_next_size(axis_flip(axis), ui_fill());
	ui_next_hover_cursor(axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
	UI_Box *box = ui_box_make(UI_BoxFlag_Clickable,
							  string);
	Vec2U32 screen_size = gfx_get_window_client_area(g_ui_ctx->renderer->gfx);
	UI_Comm comm = {0};
	if (!profiler_ui_is_dragging())
	{
		comm = ui_comm_from_box(box);
		if (comm.dragging)
		{
			F32 drag_delta = comm.drag_delta.v[axis];
			if (side == Side_Min)
			{
				window->pos.v[axis]  -= drag_delta;
				window->size.v[axis] += drag_delta / (F32)screen_size.v[axis];
			}
			else
			{
				window->size.v[axis] -= drag_delta / (F32)screen_size.v[axis];
			}
		}
	}
	return(comm);
}

internal UI_Comm
profiler_ui_window_corner_resizer(ProfilerUI_Window *window, Str8 string, Corner corner)
{
	ui_next_width(ui_em(0.4f, 1));
	ui_next_height(ui_em(0.4f, 1));
	ui_next_hover_cursor(corner == Corner_TopLeft || corner == Corner_BottomRight ?
						 Gfx_Cursor_SizeNWSE :
						 Gfx_Cursor_SizeNESW);
	UI_Box *box = ui_box_make(UI_BoxFlag_Clickable,
							  string);
	UI_Comm comm = ui_comm_from_box(box);
	Vec2F32 screen_size   = v2f32_from_v2u32(gfx_get_window_area(g_ui_ctx->renderer->gfx));
	if (comm.dragging)
	{
		switch (corner)
		{
			case Corner_TopLeft:
			{
				window->pos           = v2f32_sub_v2f32(window->pos, comm.drag_delta);
				window->size          = v2f32_add_v2f32(window->size,
														v2f32_hadamard_div_v2f32(comm.drag_delta, screen_size));
			} break;

			case Corner_BottomLeft:
			{
				window->pos.x        -= comm.drag_delta.v[Axis2_X];
				window->size.x       += comm.drag_delta.v[Axis2_X] / screen_size.v[Axis2_X];
				window->size.y       -= comm.drag_delta.v[Axis2_Y] / screen_size.v[Axis2_Y];
			} break;

			case Corner_TopRight:
			{
				window->pos.y        -= comm.drag_delta.v[Axis2_Y];
				window->size.x       -= comm.drag_delta.v[Axis2_X] / screen_size.v[Axis2_X];
				window->size.y       += comm.drag_delta.v[Axis2_Y] / screen_size.v[Axis2_Y];
			} break;

			case Corner_BottomRight:
			{
				window->size          = v2f32_sub_v2f32(window->size,
														v2f32_hadamard_div_v2f32(comm.drag_delta, screen_size));
			} break;

			invalid_case;
		}
	}

	return(comm);
}

internal Void
profiler_ui_update_window(ProfilerUI_Window *window)
{
	ui_seed(window->string)
	{
		if (window == profiler_ui_state->master_window)
		{
			profiler_ui_update_panel(window->root_panel);
		}
		else
		{
			Vec2F32 pos = window->pos;
			// NOTE(hampus): Screen pos -> Container pos
			pos = v2f32_sub_v2f32(pos, profiler_ui_state->window_container->fixed_rect.min);
			// NOTE(hampus): Container pos -> Window pos
			pos.x -= ui_em(0.4f, 1).value;
			pos.y -= ui_em(0.4f, 1).value;
			ui_next_width(ui_pct(window->size.x, 1));
			ui_next_height(ui_pct(window->size.y, 1));
			ui_next_relative_pos(Axis2_X, pos.v[Axis2_X]);
			ui_next_relative_pos(Axis2_Y, pos.v[Axis2_Y]);
			ui_next_child_layout_axis(Axis2_X);
			ui_next_color(profiler_ui_color_from_theme(ProfilerUI_Color_Panel));
			UI_Box *window_container = ui_box_make(UI_BoxFlag_FloatingPos |
												   UI_BoxFlag_DrawBackground |
												   UI_BoxFlag_DrawDropShadow,
												   str8_lit(""));
			window->box = window_container;
			ui_parent(window_container)
			{
				ui_next_height(ui_fill());
				ui_column()
				{
					profiler_ui_window_corner_resizer(window, str8_lit("TopLeftWindowResize"), Corner_TopLeft);
					profiler_ui_window_edge_resizer(window, str8_lit("TopWindowResize"), Axis2_X, Side_Min);
					profiler_ui_window_corner_resizer(window, str8_lit("BottomLeftWindowResize"), Corner_BottomLeft);
				}

				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_column()
				{
					profiler_ui_window_edge_resizer(window, str8_lit("LeftWindowResize"), Axis2_Y, Side_Min);
					profiler_ui_update_panel(window->root_panel);
					profiler_ui_window_edge_resizer(window, str8_lit("RightWindowResize"), Axis2_Y, Side_Max);
				}

				ui_next_height(ui_fill());
				ui_column()
				{
					profiler_ui_window_corner_resizer(window, str8_lit("TopRightWindowResize"), Corner_TopRight);
					profiler_ui_window_edge_resizer(window, str8_lit("BottomWindowResize"), Axis2_X, Side_Max);
					profiler_ui_window_corner_resizer(window, str8_lit("BottomRightWindowResize"), Corner_BottomRight);
				}
			}
			window->size.v[Axis2_X] = f32_clamp(0.05f, window->size.v[Axis2_X], 1.0f);
			window->size.v[Axis2_Y] = f32_clamp(0.05f, window->size.v[Axis2_Y], 1.0f);
		}
	}
}

#include "profiler_ui_commands.c"

////////////////////////////////
//~ hampus: UI startup builder

#define profiler_ui_builder_split_panel(panel_to_split, split_axis, ...) profiler_ui_builder_split_panel_(&(ProfilerUI_PanelSplit) { .panel = panel_to_split, .axis = split_axis, __VA_ARGS__})
internal ProfilerUI_SplitPanelResult
profiler_ui_builder_split_panel_(ProfilerUI_PanelSplit *data)
{
	ProfilerUI_SplitPanelResult result = {0};
	profiler_ui_command_panel_split(data);
	result.panels[Side_Min] = data->panel;
	result.panels[Side_Max] = data->panel->sibling;
	return(result);
}

////////////////////////////////
//~ hampus: Tab views

PROFILER_UI_TAB_VIEW(profiler_ui_tab_view_default)
{
	ProfilerUI_Tab *tab = data;
	ProfilerUI_Panel *panel = tab->panel;
	ProfilerUI_Window *window = panel->window;
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
				profiler_ui_panel_split(panel, Axis2_X);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Split panelF Y")).pressed)
			{
				profiler_ui_panel_split(panel, Axis2_Y);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close panel")).pressed)
			{
				profiler_ui_attempt_to_close_panel(panel);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Add tab")).pressed)
			{
				ProfilerUI_Tab *new_tab = profiler_ui_tab_make(profiler_ui_state->perm_arena, 0, 0, str8_lit(""));
				profiler_ui_panel_attach_tab(panel, new_tab, false);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close tab")).pressed)
			{
				profiler_ui_attempt_to_close_tab(tab);
			}
			ui_spacer(ui_em(0.5f, 1));
			ui_row()
			{
				ui_check(&g_ui_ctx->show_debug_lines, str8_lit("ShowDebugLines"));
				ui_spacer(ui_em(0.5f, 1));
				ui_text(str8_lit("Show debug lines"));
			}
		}
	}
}

PROFILER_UI_TAB_VIEW(profiler_ui_theme_tab)
{
	UI_Key theme_color_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("ThemeColorCtxMenu"));

	ui_ctx_menu(theme_color_ctx_menu)
		ui_width(ui_em(4, 1))
	{
		ui_next_width(ui_em(10, 1));
		ui_next_height(ui_em(10, 1));
		UI_Box *container = ui_box_make(UI_BoxFlag_DrawBackground |
										UI_BoxFlag_DrawBorder,
										str8_lit(""));
		ui_parent(container)
		{

		}
	}

	ui_column()
		ui_width(ui_em(1, 1))
		ui_height(ui_em(1, 1))
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
				}
			}
			ui_spacer(ui_em(0.5f, 1));
		}
	}

}

////////////////////////////////
//~ hampus: Update

internal Void
profiler_ui_update(Render_Context *renderer, Gfx_EventList *event_list)
{
	Vec2F32 mouse_pos = ui_mouse_pos();

	B32 left_mouse_released = false;
	for (Gfx_Event *event = event_list->first;
		 event != 0;
		 event = event->next)
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

	profiler_ui_state->focused_panel = profiler_ui_state->next_focused_panel;
	profiler_ui_state->next_focused_panel = 0;

	//- hampus: Update Windows

	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	UI_Box *window_root_parent = ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("RootWindow"));
	profiler_ui_state->window_container = window_root_parent;
	ui_parent(window_root_parent)
	{
		for (ProfilerUI_Window *window = profiler_ui_state->window_list.first;
			 window != 0;
			 window = window->next)
		{
			profiler_ui_update_window(window);
		}
	}

	//- hampus: Update tab drag

	if (left_mouse_released &&
		profiler_ui_is_dragging())
	{
		profiler_ui_drag_release();
	}

	ProfilerUI_DragData *drag_data = &profiler_ui_state->drag_data;
	switch (profiler_ui_state->drag_status)
	{
		case ProfilerUI_DragStatus_Inactive: {} break;

		case ProfilerUI_DragStatus_Reordering: {} break;

		case ProfilerUI_DragStatus_WaitingForDragThreshold:
		{
			F32 drag_threshold = ui_em(3, 1).value;
			Vec2F32 delta = v2f32_sub_v2f32(mouse_pos, drag_data->drag_origin);
			if (f32_abs(delta.x) > drag_threshold ||
				f32_abs(delta.y) > drag_threshold)
			{
				ProfilerUI_Tab *tab = drag_data->tab;

				// NOTE(hampus): Calculate the new window size
				Vec2F32 new_window_pct = v2f32(1, 1);
				ProfilerUI_Panel *panel_child = tab->panel;
				for (ProfilerUI_Panel *panel_parent = panel_child->parent;
					 panel_parent != 0;
					 panel_parent = panel_parent->parent)
				{
					Axis2 axis = panel_parent->split_axis;
					new_window_pct.v[axis] *= panel_child->pct_of_parent;
					panel_child = panel_parent;
				}

				new_window_pct.x *= tab->panel->window->size.x;
				new_window_pct.y *= tab->panel->window->size.y;

				ProfilerUI_Panel *tab_panel = tab->panel;
				B32 create_new_window = !(tab->panel == tab->panel->window->root_panel &&
										  tab_panel->tab_group.count == 1 &&
										  tab_panel->window != profiler_ui_state->master_window);
				if (create_new_window)
				{
					// NOTE(hampus): Close the tab from the old panel
					{
						ProfilerUI_TabDelete tab_close =
						{
							.tab = drag_data->tab
						};
						profiler_ui_command_tab_close(&tab_close);
					}

					ProfilerUI_Window *new_window = profiler_ui_window_make(profiler_ui_state->perm_arena, new_window_pct);

					{
						ProfilerUI_TabAttach tab_attach =
						{
							.tab = drag_data->tab,
							.panel = new_window->root_panel,
							.set_active = true,
						};
						profiler_ui_command_tab_attach(&tab_attach);
					}
				}
				else
				{
					drag_data->tab->panel->sibling = 0;
					profiler_ui_window_reorder_to_front(drag_data->tab->panel->window);
				}

				Vec2F32 offset = v2f32_sub_v2f32(drag_data->drag_origin, tab->box->fixed_rect.min);
				profiler_ui_state->next_focused_panel = drag_data->tab->panel;
				profiler_ui_state->drag_status = ProfilerUI_DragStatus_Dragging;
				drag_data->tab->panel->window->pos = v2f32_sub_v2f32(mouse_pos, offset);
				log_info("Drag: dragging");
			}

		} break;

		case ProfilerUI_DragStatus_Dragging:
		{
			ProfilerUI_Window *window = drag_data->tab->panel->window;
			Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, ui_prev_mouse_pos());
			window->pos = v2f32_add_v2f32(window->pos, mouse_delta);;
		} break;

		case ProfilerUI_DragStatus_Released:
		{
			// NOTE(hampus): If it is the last tab of the window,
			// we don't need to allocate a new panel. Just use
			// the tab's panel
			memory_zero_struct(&profiler_ui_state->drag_data);
			profiler_ui_state->drag_status = ProfilerUI_DragStatus_Inactive;
		} break;

		invalid_case;
	}

	if (left_mouse_released &&
		(profiler_ui_is_waiting_for_drag_threshold() ||
		 profiler_ui_is_tab_reordering()))
	{
		profiler_ui_drag_end();
	}

	if (profiler_ui_state->next_top_most_window)
	{
		ProfilerUI_Window *window = profiler_ui_state->next_top_most_window;
		profiler_ui_window_remove_from_list(window);
		profiler_ui_window_push_to_front(window);
	}

	profiler_ui_state->next_top_most_window = 0;

	ui_end();

	for (U64 i = 0; i < profiler_ui_state->cmd_buffer.pos; ++i)
	{
		ProfilerUI_Command *cmd = profiler_ui_state->cmd_buffer.buffer + i;
		switch (cmd->kind)
		{
			case ProfilerUI_CommandKind_TabAttach:  profiler_ui_command_tab_attach(cmd->data); break;
			case ProfilerUI_CommandKind_TabClose:   profiler_ui_command_tab_close(cmd->data); break;
			case ProfilerUI_CommandKind_TabReorder: profiler_ui_command_tab_reorder(cmd->data); break;

			case ProfilerUI_CommandKind_PanelSplit:          profiler_ui_command_panel_split(cmd->data);            break;
			case ProfilerUI_CommandKind_PanelSplitAndAttach: profiler_ui_command_panel_split_and_attach(cmd->data); break;
			case ProfilerUI_CommandKind_PanelSetActiveTab:   profiler_ui_command_panel_set_active_tab(cmd->data);   break;
			case ProfilerUI_CommandKind_PanelClose:          profiler_ui_command_panel_close(cmd->data);            break;

			case ProfilerUI_CommandKind_WindowRemoveFromList: profiler_ui_command_window_remove_from_list(cmd->data); break;
			case ProfilerUI_CommandKind_WindowPushToFront:    profiler_ui_command_window_push_to_front(cmd->data);    break;
		}
	}

	if (!profiler_ui_state->next_focused_panel)
	{
		profiler_ui_state->next_focused_panel = profiler_ui_state->focused_panel;
	}

	profiler_ui_state->resizing_panel = 0;

	profiler_ui_state->cmd_buffer.pos = 0;
}
