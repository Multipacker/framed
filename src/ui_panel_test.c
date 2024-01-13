////////////////////////////////
//~ hampus: TODO

// @feature [ ] - Scroll tabs horizontally if there are too many to fit
// @feature [ ] - Reorder tabs
// @feature [ ] - Be able to pin windows which disables closing
// @feature [ ] - Tab dropdown menu
// @bug [ ] - Close button is rendered even though the tab is outside tab bar
// @bug [ ] - The user can drop a panel on the menu bar which will hide the tab bar
// @cleanup [ ] - UI commands. Discriminated unions instead of data array?
// @feature [ ] - UI startup builder
// @feature [ ] - Move around windows that have multiple panels

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "image/image_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "image/image_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"

#include "ui_panel_test.h"

#include "log_ui.c"
#include "texture_ui.c"

////////////////////////////////
//~ hampus: Globals

AppState *app_state;

////////////////////////////////
//~ hampus: Command

internal Void *
cmd_push(CmdBuffer *buffer, CmdKind kind)
{
	assert(buffer->pos < buffer->size);
	Cmd *result = buffer->buffer + buffer->pos;
	memory_zero_struct(result);
	result->kind = kind;
	buffer->pos++;
	return(result->data);
}

////////////////////////////////
//~ hampus: Tab dragging

internal Void
ui_drag_prepare(Tab *tab, Vec2F32 mouse_offset)
{
	DragData *drag_data = &app_state->drag_data;
	drag_data->tab = tab;

	drag_data->drag_origin = gfx_get_mouse_pos(g_ui_ctx->renderer->gfx);

	// NOTE(hampus): Since the root window may not have
	// it's origin on (0, 0), we have to account for it
	app_state->drag_status = DragStatus_WaitingForDragThreshold;
}

internal Void
ui_drag_release(Void)
{
	app_state->drag_status = DragStatus_Released;
}

internal Void
ui_drag_end(Void)
{
	app_state->drag_status = DragStatus_Inactive;
	memory_zero_struct(&app_state->drag_data);
}

internal B32
ui_currently_dragging(Void)
{
	B32 result = app_state->drag_status == DragStatus_Dragging;
	return(result);
}

internal B32
ui_drag_is_prepared(Void)
{
	B32 result = app_state->drag_status == DragStatus_WaitingForDragThreshold;
	return(result);
}

internal B32
ui_drag_is_inactive(Void)
{
	B32 result = app_state->drag_status == DragStatus_Inactive;
	return(result);
}

////////////////////////////////
//~ hampus: Tabs

internal Tab *
ui_tab_alloc(Arena *arena)
{
	Tab *result = push_struct(arena, Tab);
	result->string = str8_pushf(app_state->perm_arena, "Tab%"PRIS32, app_state->num_tabs);
	app_state->num_tabs++;
	return(result);
}

internal Void
ui_tab_equip_view_info(Tab *tab, TabViewInfo view_info)
{
	tab->view_info = view_info;
}

UI_TAB_VIEW(ui_tab_view_default);
internal Tab *ui_tab_make(Arena *arena, UI_TabViewProc *function, Void *data)
{
	Tab *result = ui_tab_alloc(arena);
	TabViewInfo view_info = {function, data};
	ui_tab_equip_view_info(result, view_info);
	if (!function)
	{
		// NOTE(hampus): Equip with default view function
		result->view_info.function = ui_tab_view_default;
		result->view_info.data = result;
	}
	return(result);
}

internal B32
ui_tab_is_active(Tab *tab)
{
	B32 result = tab->panel->tab_group.active_tab == tab;
	return(result);
}

internal Void
ui_tab_close(Tab *tab)
{
	if (!tab->pinned)
	{
		TabDelete *data = cmd_push(&app_state->cmd_buffer, CmdKind_TabClose);
		data->tab = tab;
	}
}

internal Void
ui_tab_button(Tab *tab)
{
	assert(tab->frame_index != app_state->frame_index);
	tab->frame_index = app_state->frame_index;
	ui_push_string(tab->string);

	B32 active = ui_tab_is_active(tab);

	ui_spacer(ui_em(0.1f, 1));
	if (!active)
	{
		ui_spacer(ui_em(0.1f, 1));
	}

	Vec4F32 color = active ? v4f32(0.4f, 0.4f, 0.4f, 1.0f) : v4f32(0.2f, 0.2f, 0.2f, 1.0f);

	F32 height_em = active ? 1.1f : 1;
	F32 corner_radius = (F32) ui_top_font_size() * 0.5f;

	ui_next_color(color);
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
										  UI_BoxFlag_Clip,
										  str8_lit("TitleContainer"));
	tab->box = title_container;
	ui_parent(title_container)
	{
		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_PIN);
		ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
		UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
		if (tab->pinned)
		{
			pin_box_flags |= UI_BoxFlag_DrawText;
		}

		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(corner_radius, 0, 0, 0);
		UI_Box *pin_box = ui_box_make(pin_box_flags,
									  str8_lit("PinButton"));

		UI_Box *title = ui_box_make(UI_BoxFlag_DrawText,
									tab->string);

		ui_box_equip_display_string(title, tab->string);

		ui_next_height(ui_pct(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(RENDER_ICON_CROSS);
		ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
		ui_next_hover_cursor(Gfx_Cursor_Hand);
		ui_next_corner_radies(0, corner_radius, 0, 0);
		UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable,
										str8_lit("CloseButton"));
		// TODO(hampus): We shouldn't need to do this here
		// since there shouldn't even be any input events
		// left in the queue if dragging is ocurring.
		if (!ui_currently_dragging())
		{
			UI_Comm pin_box_comm   = ui_comm_from_box(pin_box);
			UI_Comm close_box_comm = ui_comm_from_box(close_box);
			UI_Comm title_comm = ui_comm_from_box(title_container);

			if (title_comm.pressed)
			{
				if (!tab->pinned)
				{
					ui_drag_prepare(tab, title_comm.rel_mouse);
				}

				PanelSetActiveTab *data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelSetActiveTab);
				data->tab = tab;
				data->panel = tab->panel;
			}

			// NOTE(hampus): Icon appearance
			UI_BoxFlags icon_hover_flags = UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation | UI_BoxFlag_DrawText;

			if (pin_box_comm.hovering)
			{
				pin_box->flags |= icon_hover_flags;
			}

			if (close_box_comm.hovering)
			{
				close_box->flags |= icon_hover_flags;
			}

			if (close_box_comm.hovering ||
				pin_box_comm.hovering ||
				title_comm.hovering)
			{
				pin_box->flags   |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
				close_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
			}

			if (pin_box_comm.pressed)
			{
				tab->pinned = !tab->pinned;
			}

			if (close_box_comm.pressed)
			{
				ui_tab_close(tab);
			}
		}
	}
	ui_pop_string();
}

////////////////////////////////
//~ hampus: Panels

internal Panel *
ui_panel_alloc(Arena *arena)
{
	Panel *result = push_struct(arena, Panel);
	result->string = str8_pushf(app_state->perm_arena, "Panel%"PRIS32, app_state->num_panels);
	app_state->num_panels++;
	return(result);
}

internal Side
ui_get_panel_side(Panel *panel)
{
	assert(panel->parent);
	Side result = panel->parent->children[Side_Min] == panel ? Side_Min : Side_Max;
	assert(panel->parent->children[result] == panel);
	return(result);
}

internal Void
ui_attach_tab_to_panel(Panel *panel, Tab *tab, B32 set_active)
{
	TabAttach *data  = cmd_push(&app_state->cmd_buffer, CmdKind_TabAttach);
	data->tab        = tab;
	data->panel      = panel;
	data->set_active = set_active;
}

internal Void
ui_panel_split(Panel *first, Axis2 split_axis)
{
	PanelSplit *data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelSplit);
	data->panel = first;
	data->axis = split_axis;
	data->alloc_new_tab = true;
}

internal B32
ui_panel_is_leaf(Panel *panel)
{
	B32 result = !(panel->children[0] &&  panel->children[1]);
	return(result);
}

internal Void
ui_attach_and_split_tab_to_panel(Panel *panel, Tab *tab, Axis2 axis, Side side)
{
	PanelSplitAndAttach *data  = cmd_push(&app_state->cmd_buffer, CmdKind_PanelSplitAndAttach);
	data->tab           = tab;
	data->panel         = panel;
	data->axis          = axis;
	data->panel_side    = side;
}

internal UI_Comm
ui_hover_panel_type(Str8 string, F32 width_in_em, Panel *root, Axis2 axis, B32 center, Side side)
{
	ui_next_width(ui_em(width_in_em, 1));
	ui_next_height(ui_em(width_in_em, 1));
	ui_next_color(ui_top_text_style()->color);

	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground,
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
ui_panel(Panel *root)
{
	assert(root->frame_index != app_state->frame_index);
	root->frame_index = app_state->frame_index;
	switch (root->split_axis)
	{
		case Axis2_X: ui_next_child_layout_axis(Axis2_X); break;
		case Axis2_Y: ui_next_child_layout_axis(Axis2_Y); break;
		invalid_case;
	}

	Panel *parent = root->parent;

	if (parent)
	{
		Axis2 flipped_split_axis = !parent->split_axis;
		ui_next_size(parent->split_axis, ui_pct(root->pct_of_parent, 0));
		ui_next_size(flipped_split_axis, ui_fill());
	}
	else
	{
		ui_next_width(ui_pct(1, 0));
		ui_next_height(ui_pct(1, 0));
	}

	// NOTE(hampus): It is clickable so that it can consume
	// all the click events at the end to prevent boxes
	// from behind it to take click events
	UI_Box *box = ui_box_make(UI_BoxFlag_Clip |
							  UI_BoxFlag_Clickable |
							  UI_BoxFlag_DrawBackground,
							  root->string);
	root->box = box;

	ui_push_parent(box);

	if (!ui_panel_is_leaf(root))
	{
		Panel *child0 = root->children[Side_Min];
		Panel *child1 = root->children[Side_Max];

		ui_panel(child0);

		B32 dragging = false;
		F32 drag_delta = 0;

		ui_seed(root->string)
		{
			ui_next_size(root->split_axis, ui_em(0.2f, 1));
			ui_next_size(!root->split_axis, ui_pct(1, 1));
			ui_next_corner_radius(0);
			ui_next_hover_cursor(root->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
			UI_Box *draggable_box = ui_box_make(UI_BoxFlag_HotAnimation |
												UI_BoxFlag_ActiveAnimation |
												UI_BoxFlag_DrawBackground |
												UI_BoxFlag_Clickable,
												str8_lit("DraggableBox"));
			root->dragger = draggable_box;
			UI_Comm comm = ui_comm_from_box(draggable_box);
			if (comm.dragging)
			{
				dragging = true;
				drag_delta = comm.drag_delta.v[root->split_axis];
			}
		}

		ui_panel(child1);

		if (dragging)
		{
			child0->pct_of_parent -= drag_delta / box->calc_size.v[root->split_axis];
			child1->pct_of_parent += drag_delta / box->calc_size.v[root->split_axis];

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
					if (node->kind == Gfx_EventKind_KeyPress)
					{
						if (!app_state->next_focused_panel)
						{
							app_state->next_focused_panel = root;
						}
						if (root->window != app_state->master_window &&
							root->window != app_state->window_list.first &&
							!ui_currently_dragging())
						{
							ui_window_reorder_to_front(root->window);
						}
					}
				}
			}
		}

		// NOTE(hampus): Axis2_COUNT is the center
		Axis2   hover_axis = Axis2_COUNT;
		Side    hover_side = 0;
		UI_Comm tab_release_comms[TabReleaseKind_COUNT] = {0};
		B32     hovering_any_symbols = false;

		//- hampus: Drag & split symbols

		if (ui_currently_dragging())
		{
			DragData *drag_data = &app_state->drag_data;
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
						tab_release_comms[TabReleaseKind_Top] = ui_hover_panel_type(str8_lit("TabReleaseTop"), size, root, Axis2_Y, false,
																					Side_Min);

						ui_spacer(ui_fill());
					}
					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[TabReleaseKind_Left] = ui_hover_panel_type(str8_lit("TabReleaseLeft"), size, root, Axis2_X, false, Side_Min);

						ui_spacer(ui_em(1, 1));

						tab_release_comms[TabReleaseKind_Center] = ui_hover_panel_type(str8_lit("TabReleaseCenter"), size, root, Axis2_X, true, Side_Min);

						ui_spacer(ui_em(1, 1));

						ui_next_child_layout_axis(Axis2_X);
						tab_release_comms[TabReleaseKind_Right] = ui_hover_panel_type(str8_lit("TabReleaseRight"), size, root, Axis2_X, false, Side_Max);

						ui_spacer(ui_fill());
					}

					ui_spacer(ui_em(1, 1));

					ui_next_width(ui_fill());
					ui_row()
					{
						ui_spacer(ui_fill());
						tab_release_comms[TabReleaseKind_Bottom] = ui_hover_panel_type(str8_lit("TabReleaseBottom"), size, root, Axis2_Y, false, Side_Max);
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

			for (TabReleaseKind i = (TabReleaseKind) 0;
				 i < TabReleaseKind_COUNT;
				 ++i)
			{
				UI_Comm *comm = tab_release_comms + i;
				if (comm->hovering)
				{
					hovering_any_symbols = true;
					switch (i)
					{
						case TabReleaseKind_Center:
						{
							hover_axis = Axis2_COUNT;
						} break;

						case TabReleaseKind_Left:
						case TabReleaseKind_Right:
						{
							hover_axis = Axis2_X;
							hover_side = i == TabReleaseKind_Left ? Side_Min : Side_Max;
						} break;

						case TabReleaseKind_Top:
						case TabReleaseKind_Bottom:
						{
							hover_axis = Axis2_Y;
							hover_side = i == TabReleaseKind_Top ? Side_Min : Side_Max;
						} break;

						invalid_case;
					}
				}
				if (comm->released)
				{
					switch (i)
					{
						case TabReleaseKind_Center:
						{
							ui_attach_tab_to_panel(root, drag_data->tab, true);
							dll_remove(app_state->window_list.first,
									   app_state->window_list.last,
									   drag_data->tab->panel->window);
							app_state->drag_status = DragStatus_Released;
						} break;

						case TabReleaseKind_Left:
						case TabReleaseKind_Right:
						case TabReleaseKind_Top:
						case TabReleaseKind_Bottom:
						{
							ui_attach_and_split_tab_to_panel(root, drag_data->tab, hover_axis, hover_side);
							dll_remove(app_state->window_list.first,
									   app_state->window_list.last,
									   drag_data->tab->panel->window);
							app_state->drag_status = DragStatus_Released;
						} break;

						invalid_case;
					}
					ui_drag_release();
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

		ui_next_width(ui_fill());
		ui_row()
		{
			ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
			ui_next_width(ui_fill());
			ui_next_height(ui_em(1.4f, 1));
			if (root->tab_group.count == 1)
			{
				ui_next_extra_box_flags(UI_BoxFlag_Clickable |
										UI_BoxFlag_HotAnimation |
										UI_BoxFlag_ActiveAnimation);
			}
			UI_Box *title_bar = ui_box_make(UI_BoxFlag_DrawBackground,
											str8_lit("TitleBar"));
			ui_parent(title_bar)
			{
				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_row()
				{
					for (Tab *tab = root->tab_group.first;
						 tab != 0;
						 tab = tab->next)
					{
						ui_next_width(ui_children_sum(1));
						ui_next_height(ui_em(1.2f, 1));
						ui_next_child_layout_axis(Axis2_Y);
						UI_Box *container = ui_box_make(0, str8_lit(""));
						ui_parent(container)
						{
							ui_spacer(ui_em(0.2f, 1));
							ui_tab_button(tab);
						}
						ui_spacer(ui_em(0.2f, 1));
					}

					ui_spacer(ui_fill());

					ui_next_height(ui_em(1.3f, 1));
					ui_next_width(ui_em(1.3f, 1));
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
						PanelClose *data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelClose);
						data->panel = root;
					}
				}
			}

			if (root->tab_group.count == 1 &&
				!ui_currently_dragging())
			{
				UI_Comm title_bar_comm = ui_comm_from_box(title_bar);
				if (title_bar_comm.pressed)
				{
					ui_drag_prepare(root->tab_group.active_tab, title_bar_comm.rel_mouse);
				}
			}
		}

		//- hampus: Tab content

		if (root != app_state->focused_panel)
		{
			ui_next_width(ui_fill());
			ui_next_height(ui_fill());
			ui_next_color(v4f32(0.0f, 0.0f, 0.0f, 0.5f));
			ui_box_make(UI_BoxFlag_DrawBackground |
						UI_BoxFlag_FloatingPos,
						str8_lit("ContentDim"));
		}

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		UI_Box *content_box_container = ui_box_make(0,
													str8_lit("ContentBoxContainer"));
		ui_parent(content_box_container)
		{
			if (root == app_state->focused_panel)
			{
				ui_next_border_color(v4f32(1, 0.5f, 0, 1));
			}
			ui_next_width(ui_fill());
			ui_next_height(ui_fill());
			ui_next_child_layout_axis(Axis2_Y);
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
					Tab *tab = root->tab_group.active_tab;
					tab->view_info.function(tab->view_info.data);
					ui_spacer(padding);
				}
				ui_spacer(padding);
			}
		}
		ui_pop_string();
	}

	B32 take_input_from_root = true;
	if (ui_currently_dragging())
	{
		if (root == app_state->drag_data.tab->panel)
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
ui_window_reorder_to_front(Window *window)
{
	if (window != app_state->master_window)
	{
		if (!app_state->next_top_most_window)
		{
			app_state->next_top_most_window = window;
		}
	}
}

internal Void
ui_window_push_to_front(Window *window)
{
	dll_push_front(app_state->window_list.first, app_state->window_list.last, window);
}

internal Void
ui_window_remove_from_list(Window *window)
{
	dll_remove(app_state->window_list.first, app_state->window_list.last, window);
}

internal Window *
ui_window_alloc(Arena *arena)
{
	Window *result = push_struct(arena, Window);
	result->string = str8_pushf(arena, "Window%d", app_state->num_windows++);
	return(result);
}

internal Window *
ui_window_make(Arena *arena, Vec2F32 size)
{
	Window *result = ui_window_alloc(arena);
	Panel *panel = ui_panel_alloc(arena);
	panel->window = result;
	result->size = size;
	ui_window_push_to_front(result);
	result->root_panel = panel;
	return(result);
}

internal UI_Comm
ui_window_edge_resizer(Window *window, Str8 string, Axis2 axis, Side side)
{
	ui_next_size(axis, ui_em(0.4f, 1));
	ui_next_size(axis_flip(axis), ui_fill());
	ui_next_hover_cursor(axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
	UI_Box *box = ui_box_make(UI_BoxFlag_Clickable,
							  string);
	Vec2U32 screen_size = gfx_get_window_client_area(g_ui_ctx->renderer->gfx);
	UI_Comm comm = {0};
	if (!ui_currently_dragging())
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
ui_window_corner_resizer(Window *window, Str8 string, Corner corner)
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
ui_update_window(Window *window)
{
	ui_seed(window->string)
	{
		if (window == app_state->master_window)
		{
			ui_panel(window->root_panel);
		}
		else
		{
			Vec2F32 pos = window->pos;
			// NOTE(hampus): Screen pos -> Container pos
			pos = v2f32_sub_v2f32(pos, app_state->window_container->rect.min);
			// NOTE(hampus): Container pos -> Window pos
			pos.x -= ui_em(0.4f, 1).value;
			pos.y -= ui_em(0.4f, 1).value;
			ui_next_width(ui_pct(window->size.x, 1));
			ui_next_height(ui_pct(window->size.y, 1));
			ui_next_relative_pos(Axis2_X, pos.v[Axis2_X]);
			ui_next_relative_pos(Axis2_Y, pos.v[Axis2_Y]);
			ui_next_child_layout_axis(Axis2_X);
			UI_Box *window_container = ui_box_make(UI_BoxFlag_FloatingPos |
												   UI_BoxFlag_DrawDropShadow,
												   str8_lit(""));
			window->box = window_container;
			ui_parent(window_container)
			{
				ui_next_height(ui_fill());
				ui_column()
				{
					ui_window_corner_resizer(window, str8_lit("TopLeftWindowResize"), Corner_TopLeft);
					ui_window_edge_resizer(window, str8_lit("TopWindowResize"), Axis2_X, Side_Min);
					ui_window_corner_resizer(window, str8_lit("BottomLeftWindowResize"), Corner_BottomLeft);
				}

				ui_next_width(ui_fill());
				ui_next_height(ui_fill());
				ui_column()
				{
					ui_window_edge_resizer(window, str8_lit("LeftWindowResize"), Axis2_Y, Side_Min);
					ui_panel(window->root_panel);
					ui_window_edge_resizer(window, str8_lit("RightWindowResize"), Axis2_Y, Side_Max);
				}

				ui_next_height(ui_fill());
				ui_column()
				{
					ui_window_corner_resizer(window, str8_lit("TopRightWindowResize"), Corner_TopRight);
					ui_window_edge_resizer(window, str8_lit("BottomWindowResize"), Axis2_X, Side_Max);
					ui_window_corner_resizer(window, str8_lit("BottomRightWindowResize"), Corner_BottomRight);
				}
			}
			window->size.v[Axis2_X] = f32_clamp(0.05f, window->size.v[Axis2_X], 1.0f);
			window->size.v[Axis2_Y] = f32_clamp(0.05f, window->size.v[Axis2_Y], 1.0f);
		}
	}
}

#include "ui_panel_cmd.c"

////////////////////////////////
//~ hampus: UI startup builder

#define ui_builder_split_panel(panel_to_split, split_axis, ...) ui_builder_split_panel_(&(PanelSplit) { .panel = panel_to_split, .axis = split_axis, __VA_ARGS__})
internal SplitPanelResult
ui_builder_split_panel_(PanelSplit *data)
{
	SplitPanelResult result = {0};
	ui_command_panel_split(data);
	result.panels[Side_Min] = data->panel;
	result.panels[Side_Max] = data->panel->sibling;
	return(result);
}

////////////////////////////////
//~ hampus: Tab views

UI_TAB_VIEW(ui_tab_view_default)
{
	Tab *tab = data;
	Panel *panel = tab->panel;
	Window *window = panel->window;
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
				ui_panel_split(panel, Axis2_X);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Split panelF Y")).pressed)
			{
				ui_panel_split(panel, Axis2_Y);
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close panel")).pressed)
			{
				PanelClose *panel_close_data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelClose);
				panel_close_data->panel = panel;
			}
			ui_spacer(ui_em(1.0f, 1));
			if (ui_button(str8_lit("Add tab")).pressed)
			{
				Tab *new_tab = ui_tab_make(app_state->perm_arena, 0, 0);
				TabAttach *tab_attach_data = cmd_push(&app_state->cmd_buffer, CmdKind_TabAttach);
				tab_attach_data->tab = new_tab;
				tab_attach_data->panel = panel;
			}
			ui_spacer(ui_em(0.5f, 1));
			if (ui_button(str8_lit("Close tab")).pressed)
			{
				ui_tab_close(tab);
			}
		}
	}
}

UI_TAB_VIEW(ui_tab_view_logger)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	ui_logger();
}

UI_TAB_VIEW(ui_tab_view_texture_viewer)
{
	ui_next_width(ui_fill());
	ui_next_height(ui_fill());
	Render_TextureSlice texture = *(Render_TextureSlice *) data;
	ui_texture_view(texture);
}

////////////////////////////////
//~ hampus: Main

internal S32
os_main(Str8List arguments)
{
	log_init(str8_lit("log.txt"));

	Arena *perm_arena = arena_create();
	app_state = push_struct(perm_arena, AppState);
	app_state->perm_arena = perm_arena;

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	Render_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	Str8 path = str8_lit("data/image.png");
	Render_TextureSlice image_texture = { 0 };

	Str8 image_contents = { 0 };
	arena_scratch(0, 0)
	{
		if (os_file_read(scratch, path, &image_contents))
		{
			image_load(app_state->perm_arena, renderer, image_contents, &image_texture);
		}
		else
		{
			log_error("Could not load file '%"PRISTR8"'", str8_expand(path));
		}
	}

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	app_state->cmd_buffer.buffer = push_array(app_state->perm_arena, Cmd, CMD_BUFFER_SIZE);
	app_state->cmd_buffer.size = CMD_BUFFER_SIZE;

	//- hampus: Build startup UI

	{
		Window *master_window = ui_window_make(app_state->perm_arena, v2f32(1.0f, 1.0f));

		Panel *first_panel = master_window->root_panel;

		SplitPanelResult split_panel_result = ui_builder_split_panel(first_panel, Axis2_X);
		{
			{
				TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, ui_tab_view_logger, 0),
					.panel = split_panel_result.panels[Side_Min],
				};
				ui_command_tab_attach(&attach);
			}

			{
				TabAttach attach =
				{
					.tab = ui_tab_make(app_state->perm_arena, 0, 0),
					.panel = split_panel_result.panels[Side_Max],
				};
				ui_command_tab_attach(&attach);
			}
		}

		app_state->master_window = master_window;
		app_state->next_focused_panel = first_panel;
	}

	app_state->frame_index = 1;

	Vec2F32 prev_mouse_pos = {0};;

	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Vec2F32 mouse_pos = gfx_get_mouse_pos(&gfx);
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];

		B32 left_mouse_released = false;
		Gfx_EventList events = gfx_get_events(current_arena, &gfx);
		for (Gfx_Event *event = events.first;
			 event != 0;
			 event = event->next)
		{
			switch (event->kind)
			{
				case Gfx_EventKind_Quit:
				{
					running = false;
				} break;

				case Gfx_EventKind_KeyPress:
				{
					if (event->key == Gfx_Key_F11)
					{
						gfx_toggle_fullscreen(&gfx);
					}
					log_info("Key press!");
				} break;

				case Gfx_EventKind_KeyRelease:
				{
					if (event->key == Gfx_Key_MouseLeft)
					{
						left_mouse_released = true;
					}
				} break;

				default:
				{
				} break;
			}
		}

		render_begin(renderer);

		ui_begin(ui, &events, renderer, dt);
		UI_Key my_ctx_menu = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu"));

		ui_ctx_menu(my_ctx_menu)
			ui_width(ui_em(4, 1))
		{
			if (ui_button(str8_lit("Test")).pressed)
			{
				printf("Hello world!");
			}

			if (ui_button(str8_lit("Test2")).pressed)
			{
				printf("Hello world!");
			}

			if (ui_button(str8_lit("Test3")).pressed)
			{
				printf("Hello world!");
			}
		}

		UI_Key my_ctx_menu2 = ui_key_from_string(ui_key_null(), str8_lit("MyContextMenu2"));

		ui_ctx_menu(my_ctx_menu2)
			ui_width(ui_em(4, 1))
		{
			if (ui_button(str8_lit("Test5")).pressed)
			{
				printf("Hello world!");
			}

			ui_row()
			{
				if (ui_button(str8_lit("Test52")).pressed)
				{
					printf("Hello world!");
				}
				if (ui_button(str8_lit("Test5251")).pressed)
				{
					printf("Hello world2!");
				}
			}
			if (ui_button(str8_lit("Test53")).pressed)
			{
				printf("Hello world!");
			}
		}

		ui_next_extra_box_flags(UI_BoxFlag_DrawBackground);
		ui_next_width(ui_fill());
		ui_corner_radius(0)
			ui_softness(0)
			ui_row()
		{
			UI_Comm comm = ui_button(str8_lit("File"));

			if (comm.hovering)
			{
				if (ui_ctx_menu_is_open())
				{
					ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
				}
			}
			if (comm.pressed)
			{
				ui_ctx_menu_open(comm.box->key, v2f32(0, 0), my_ctx_menu);
			}

			UI_Comm comm2 = ui_button(str8_lit("Edit"));

			if (comm2.hovering)
			{
				if (ui_ctx_menu_is_open())
				{
					ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
				}
			}
			if (comm2.pressed)
			{
				ui_ctx_menu_open(comm2.box->key, v2f32(0, 0), my_ctx_menu2);
			}
			ui_button(str8_lit("View"));
			ui_button(str8_lit("Options"));
			ui_button(str8_lit("Help"));
		}

		app_state->focused_panel = app_state->next_focused_panel;
		app_state->next_focused_panel = 0;

		ui_log_keep_alive(current_arena);

		//- hampus: Update Windows

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		UI_Box *window_root_parent = ui_box_make(0, str8_lit("RootWindow"));
		app_state->window_container = window_root_parent;
		ui_parent(window_root_parent)
		{
			for (Window *window = app_state->window_list.first;
				 window != 0;
				 window = window->next)
			{
				ui_update_window(window);
			}
		}

		//- hampus: Update tab drag

		if (left_mouse_released &&
			ui_currently_dragging())
		{
			ui_drag_release();
		}

		DragData *drag_data = &app_state->drag_data;
		switch (app_state->drag_status)
		{
			case DragStatus_Inactive: {} break;

			case DragStatus_WaitingForDragThreshold:
			{
				F32 drag_threshold = ui_em(2, 1).value;
				Vec2F32 delta = v2f32_sub_v2f32(g_ui_ctx->mouse_pos, drag_data->drag_origin);
				if (f32_abs(delta.x) > drag_threshold ||
					f32_abs(delta.y) > drag_threshold)
				{
					Tab *tab = drag_data->tab;

					// NOTE(hampus): Calculate the new window size
					Vec2F32 new_window_pct = v2f32(1, 1);
					Panel *panel_child = tab->panel;
					for (Panel *panel_parent = panel_child->parent;
						 panel_parent != 0;
						 panel_parent = panel_parent->parent)
					{
						Axis2 axis = panel_parent->split_axis;
						new_window_pct.v[axis] *= panel_child->pct_of_parent;
						panel_child = panel_parent;
					}

					new_window_pct.x *= tab->panel->window->size.x;
					new_window_pct.y *= tab->panel->window->size.y;

					Panel *tab_panel = tab->panel;
					B32 create_new_window = !(tab->panel == tab->panel->window->root_panel &&
											  tab_panel->tab_group.count == 1 &&
											  tab_panel->window != app_state->master_window);
					if (create_new_window)
					{
						// NOTE(hampus): Close the tab from the old panel
						{
							TabDelete tab_close =
							{
								.tab = drag_data->tab
							};
							ui_command_tab_close(&tab_close);
						}

						Window *new_window = ui_window_make(app_state->perm_arena, new_window_pct);

						{
							TabAttach tab_attach =
							{
								.tab = drag_data->tab,
								.panel = new_window->root_panel,
								.set_active = true,
							};
							ui_command_tab_attach(&tab_attach);
						}
					}
					else
					{
						drag_data->tab->panel->sibling = 0;
						ui_window_reorder_to_front(drag_data->tab->panel->window);
					}

					Vec2F32 offset = v2f32_sub_v2f32(drag_data->drag_origin, tab->box->rect.min);
					app_state->next_focused_panel = drag_data->tab->panel;
					app_state->drag_status = DragStatus_Dragging;
					drag_data->tab->panel->window->pos = v2f32_sub_v2f32(mouse_pos, offset);
				}

			} break;

			case DragStatus_Dragging:
			{
				Window *window = drag_data->tab->panel->window;
				Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, prev_mouse_pos);
				window->pos = v2f32_add_v2f32(window->pos, mouse_delta);;
			} break;

			case DragStatus_Released:
			{
				// NOTE(hampus): If it is the last tab of the window,
				// we don't need to allocate a new panel. Just use
				// the tab's panel
				memory_zero_struct(&app_state->drag_data);
				app_state->drag_status = DragStatus_Inactive;
			} break;

			invalid_case;
		}

		if (left_mouse_released &&
			ui_drag_is_prepared())
		{
			ui_drag_end();
		}

		if (app_state->next_top_most_window)
		{
			Window *window = app_state->next_top_most_window;
			ui_window_remove_from_list(window);
			ui_window_push_to_front(window);
		}

		app_state->next_top_most_window = 0;

		ui_end();

		for (U64 i = 0; i < app_state->cmd_buffer.pos; ++i)
		{
			Cmd *cmd = app_state->cmd_buffer.buffer + i;
			switch (cmd->kind)
			{
				case CmdKind_TabAttach: ui_command_tab_attach(cmd->data); break;
				case CmdKind_TabClose:  ui_command_tab_close(cmd->data); break;

				case CmdKind_PanelSplit:          ui_command_panel_split(cmd->data);            break;
				case CmdKind_PanelSplitAndAttach: ui_command_panel_split_and_attach(cmd->data); break;
				case CmdKind_PanelSetActiveTab:   ui_command_panel_set_active_tab(cmd->data);   break;
				case CmdKind_PanelClose:          ui_command_panel_close(cmd->data);            break;

				case CmdKind_WindowRemoveFromList: ui_command_window_remove_from_list(cmd->data); break;
				case CmdKind_WindowPushToFront:    ui_command_window_push_to_front(cmd->data);    break;
			}
		}

		if (!app_state->next_focused_panel)
		{
			app_state->next_focused_panel = app_state->focused_panel;
		}

		app_state->cmd_buffer.pos = 0;

		render_end(renderer);

		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);

		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;

		app_state->frame_index++;

		prev_mouse_pos = mouse_pos;
	}

	return(0);
}
