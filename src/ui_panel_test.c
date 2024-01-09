// TODO(hampus):
// []  - Look into the 1-frame rendering glitch when dropping a tab
// [ ] - What to do if the user drops a tab outside of any panel
// [ ] - Customize the view of tabs
//          - Function pointers?
//          - Enums?
// [ ] - Scroll tabs horizontally if there are too many to fit
// [ ] - Reorder tabs
// [ ] - Highlighting of the active panel
// [ ] - Windows
//     [ ] - Resizing
//     [ ] - Window title = tab title
//     [ ] - Window content = tab content
//     [x] - Dragging around
//     [ ] - Pinnable which disables closing
//     [ ] - A common frame for panels on a window

#include "base/base_inc.h"
#include "os/os_inc.h"
#include "log/log_inc.h"
#include "gfx/gfx_inc.h"
#include "render/render_inc.h"
#include "ui/ui_inc.h"

#include "base/base_inc.c"
#include "os/os_inc.c"
#include "log/log_inc.c"
#include "gfx/gfx_inc.c"
#include "render/render_inc.c"
#include "ui/ui_inc.c"

typedef struct Panel Panel;
typedef struct Cmd Cmd;
typedef struct Window Window ;

typedef struct Tab Tab;
struct Tab
{
	Tab *next;
	Tab *prev;

	Str8 string;
	B32 pinned;

	Panel *panel;

	// NOTE(hampus): For debugging
	U64 frame_index;
};

typedef struct TabGroup TabGroup;
struct TabGroup
{
	U64 count;
	Tab *active_tab;
	Tab *first;
	Tab *last;
};

typedef struct Panel Panel;
struct Panel
{
	Panel *children[Side_COUNT];
	Panel *sibling;
	Panel *parent;
	Window *window;

	TabGroup tab_group;

	Axis2 split_axis;
	F32 pct_of_parent;

	Str8 string;

	UI_Box *box;

	// NOTE(hampus): For debugging
	UI_Box *dragger;
	U64 frame_index;
};

typedef struct Window Window;
struct Window
{
	Window *next;
	Window *prev;

	Vec2F32 pos;
	Vec2F32 size;
	Panel *root_panel;

	B32 dragging;
};

typedef struct WindowList WindowList;
struct WindowList
{
	Window *first;
	Window *last;
	U64 count;
};

typedef enum CmdKind CmdKind;
enum CmdKind
{
	CmdKind_TabAttach,
	CmdKind_TabClose,

	CmdKind_PanelSplit,
	CmdKind_PanelSplitAndAttach,
	CmdKind_PanelSetActiveTab,
	CmdKind_PanelClose,
};

typedef enum TabReleaseKind TabReleaseKind;
enum TabReleaseKind
{
	TabReleaseKind_Center,

	TabReleaseKind_Left,
	TabReleaseKind_Right,

	TabReleaseKind_Top,
	TabReleaseKind_Bottom,

	TabReleaseKind_COUNT
};

typedef struct TabDelete TabDelete;
struct TabDelete
{
	Tab *tab;
};

typedef struct TabAttach TabAttach;
struct TabAttach
{
	Tab   *tab;
	Panel *panel;
	B32    set_active;
};

typedef struct PanelSplit PanelSplit;
struct PanelSplit
{
	Panel *panel;
	Axis2  axis;
	B32 alloc_new_tab;
};

typedef struct PanelSplitAndAttach PanelSplitAndAttach;
struct PanelSplitAndAttach
{
	Panel *panel;
	Tab *tab;
	Axis2 axis;
	Side panel_side;  // Which side to put this panel on
};

typedef struct PanelSetActiveTab PanelSetActiveTab;
struct PanelSetActiveTab
{
	Panel *panel;
	Tab *tab;
};

typedef struct PanelClose PanelClose;
struct PanelClose
{
	Panel *panel;
};

#define UI_CMD(name) Void name(Void *params)
typedef UI_CMD(UI_CmdProc);

typedef struct Cmd Cmd;
struct Cmd
{
	CmdKind kind;
	UI_CmdProc *proc;
	U8 data[512];
};

#define CMD_BUFFER_SIZE 16
typedef struct CmdBuffer CmdBuffer;
struct CmdBuffer
{
	U64 pos;
	U64 size;
	Cmd *buffer;
};

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
	U64 num_tabs;

	UI_Box *root_window;

	CmdBuffer cmd_buffer;

	WindowList window_list;

	// NOTE(hampus): Dragging stuff
	Vec2F32 start_drag_pos;
	Vec2F32 current_drag_pos;
	Tab *drag_candidate;
	Tab *drag_tab;
	Panel *hovering_panel;
	Vec2F32 new_window_pct;
	B8 tab_released;
	B8 remove_tab_from_panel;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

AppState *app_state;

internal Void ui_attach_tab_to_panel(Panel *panel, Tab *tab, B32 set_active);

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
ui_prepare_for_drag(Tab *tab, Vec2F32 mouse_offset)
{
	// NOTE(hampus): Since the root window may not have
	// it's origin on (0, 0), we have to account for it
	app_state->start_drag_pos = v2f32_sub_v2f32(gfx_get_mouse_pos(g_ui_ctx->renderer->gfx),
												app_state->root_window->rect.min);

	// NOTE(hampus): Place it where the cursor placed
	// it on the box, not only from the box's origin
	app_state->start_drag_pos = v2f32_sub_v2f32(app_state->start_drag_pos, mouse_offset);

	app_state->drag_candidate = tab;
	app_state->current_drag_pos = app_state->start_drag_pos;

	// NOTE(hampus): Save the amount of space the
	// new window should have. Since the panel
	// is removed from the tab, we have to
	// calculate it here.
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

	app_state->new_window_pct = new_window_pct;
}

internal Void
ui_begin_drag(Void)
{
	app_state->drag_tab = app_state->drag_candidate;
	app_state->drag_candidate = 0;
	if (app_state->drag_tab->panel == app_state->drag_tab->panel->window->root_panel &&
		app_state->drag_tab->panel->tab_group.count == 1)
	{
	}
	else
	{
		TabDelete *tab_delete_data = cmd_push(&app_state->cmd_buffer, CmdKind_TabClose);
		tab_delete_data->tab = app_state->drag_tab;
		app_state->remove_tab_from_panel = true;
	}
}

internal Void
ui_end_drag(Void)
{
	app_state->hovering_panel = 0;
	memory_zero_struct(&app_state->start_drag_pos);
	memory_zero_struct(&app_state->current_drag_pos);
	app_state->drag_tab = 0;
	app_state->hovering_panel = 0;
	app_state->remove_tab_from_panel = false;
}

////////////////////////////////
//~ hampus: Tabs

internal Tab *
ui_tab_alloc(Arena *arena)
{
	Tab *result = push_struct(arena, Tab);
	result->string = str8_pushf(app_state->perm_arena, "Tab%"PRIS32, app_state->num_panels);
	app_state->num_panels++;
	return(result);
}

internal B32
ui_tab_is_active(Tab *tab)
{
	B32 result = false;
	if (tab->panel)
	{
		result = tab->panel->tab_group.active_tab == tab;
	}
	return(result);
}

internal Void
ui_tab_button(Tab *tab)
{
	//assert(tab->frame_index != app_state->frame_index);
	tab->frame_index = app_state->frame_index;
	ui_push_string(tab->string);

	Vec4F32 color = ui_tab_is_active(tab) ? v4f32(0.4f, 0.4f, 0.4f, 1.0f) : v4f32(0.2f, 0.2f, 0.2f, 1.0f);

	ui_next_color(color);
	ui_next_vert_corner_radius((F32) ui_top_font_size() * 0.5f, 0);
	ui_next_child_layout_axis(Axis2_X);
	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));

	UI_Box *title_container = ui_box_make(UI_BoxFlag_DrawBackground |
										  UI_BoxFlag_HotAnimation |
										  UI_BoxFlag_ActiveAnimation |
										  UI_BoxFlag_Clickable,
										  str8_lit("TitleContainer"));
	ui_parent(title_container)
	{
		ui_next_height(ui_em(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(R_ICON_PIN);
		ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
		UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
		if (tab->pinned)
		{
			pin_box_flags |= UI_BoxFlag_DrawText;
		}

		UI_Box *pin_box = ui_box_make(pin_box_flags,
									  str8_lit("PinButton"));

		UI_Box *title = ui_box_make(UI_BoxFlag_DrawText,
									tab->string);

		ui_box_equip_display_string(title, tab->string);

		ui_next_height(ui_em(1, 1));
		ui_next_width(ui_em(1, 1));
		ui_next_icon(R_ICON_CROSS);
		ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
		UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable,
										str8_lit("CloseButton"));

		if (!app_state->drag_tab)
		{
			UI_Comm pin_box_comm   = ui_comm_from_box(pin_box);
			UI_Comm close_box_comm = ui_comm_from_box(close_box);
			UI_Comm title_comm = ui_comm_from_box(title_container);

			if (title_comm.pressed)
			{
				if (!app_state->drag_tab && !tab->pinned)
				{
					ui_prepare_for_drag(tab, title_comm.rel_mouse);
				}

				PanelSetActiveTab *data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelSetActiveTab);
				data->tab = tab;
				data->panel = tab->panel;
			}

			if (title_comm.released)
			{
				if (app_state->drag_candidate == tab)
				{
					app_state->drag_candidate = 0;
				}
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

			if (close_box_comm.hovering || pin_box_comm.hovering || title_comm.hovering)
			{
				pin_box->flags   |= UI_BoxFlag_DrawText;
				close_box->flags |= UI_BoxFlag_DrawText;
			}

			if (pin_box_comm.pressed)
			{
				tab->pinned = !tab->pinned;
			}

			if (close_box_comm.pressed)
			{
				if (!tab->pinned)
				{
					TabDelete *data = cmd_push(&app_state->cmd_buffer, CmdKind_TabClose);
					data->tab = tab;
				}
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

UI_CUSTOM_DRAW_PROC(ui_panel_leaf_custom_draw)
{
	render_push_clip(g_ui_ctx->renderer, root->clip_rect->rect->min, root->clip_rect->rect->max, root->clip_rect->clip_to_parent);

	UI_RectStyle *rect_style = &root->rect_style;
	UI_TextStyle *text_style = &root->text_style;

	R_Font *font = render_font_from_key(g_ui_ctx->renderer, text_style->font);

	rect_style->color[0] = vec4f32_srgb_to_linear(rect_style->color[0]);
	rect_style->color[1] = vec4f32_srgb_to_linear(rect_style->color[1]);
	rect_style->color[2] = vec4f32_srgb_to_linear(rect_style->color[2]);
	rect_style->color[3] = vec4f32_srgb_to_linear(rect_style->color[3]);

	text_style->color = vec4f32_srgb_to_linear(text_style->color);

	if (ui_box_has_flag(root, UI_BoxFlag_DrawDropShadow))
	{
		Vec2F32 min = v2f32_sub_v2f32(root->rect.min, v2f32(10, 10));
		Vec2F32 max = v2f32_add_v2f32(root->rect.max, v2f32(15, 15));
		R_RectInstance *instance = render_rect(g_ui_ctx->renderer,
											   min,
											   max,
											   .softness = 15, .color = v4f32(0, 0, 0, 1));
		memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBackground))
	{
		// TODO(hampus): Correct darkening/lightening
		R_RectInstance *instance = render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .softness = rect_style->softness, .slice = rect_style->slice);
		memory_copy_array(instance->colors, rect_style->color);
		memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
	}

	render_pop_clip(g_ui_ctx->renderer);
	if (g_ui_ctx->show_debug_lines)
	{
		render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .border_thickness = 1, .color = v4f32(1, 0, 1, 1));
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_draw(child);
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBorder))
	{
		F32 d = 0;

		rect_style->border_color = rect_style->border_color;
		R_RectInstance *instance = render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .border_thickness = rect_style->border_thickness, .color = rect_style->border_color, .softness = rect_style->softness);
		memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
	}
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
		ui_next_width(ui_pct(root->window->size.x, 1));
		ui_next_height(ui_pct(root->window->size.y, 1));
		ui_next_relative_pos(Axis2_X, root->window->pos.v[Axis2_X]);
		ui_next_relative_pos(Axis2_Y, root->window->pos.v[Axis2_Y]);
		ui_next_extra_box_flags(UI_BoxFlag_FloatingPos);
	}

	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
							  UI_BoxFlag_Clip,
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
			ui_next_size(root->split_axis, ui_em(0.3f, 1));
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


		// NOTE(hampus): Axis2_COUNT is the center
		Axis2   hover_axis = Axis2_COUNT;
		Side    hover_side = 0;
		UI_Comm tab_release_comms[TabReleaseKind_COUNT] = {0};
		B32 hovering_any_symbols = false;
		// NOTE(hampus): Drag & split symbols
		if (app_state->drag_tab)
		{
			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_pct(1, 1));
			UI_Box *split_symbols_container = ui_box_make(UI_BoxFlag_FloatingPos,
														  str8_lit("SplitSymbolsContainer"));
			if (root == app_state->hovering_panel)
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

			if (root != app_state->drag_tab->panel)
			{
				UI_Comm panel_comm = ui_comm_from_box(split_symbols_container);
				if (panel_comm.hovering)
				{
					app_state->hovering_panel = root;
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
							ui_attach_tab_to_panel(root, app_state->drag_tab, true);
							dll_remove(app_state->window_list.first,
									   app_state->window_list.last,
									   app_state->drag_tab->panel->window);
							app_state->tab_released = true;
						} break;

						case TabReleaseKind_Left:
						case TabReleaseKind_Right:
						case TabReleaseKind_Top:
						case TabReleaseKind_Bottom:
						{
							ui_attach_and_split_tab_to_panel(root, app_state->drag_tab, hover_axis, hover_side);
							dll_remove(app_state->window_list.first,
									   app_state->window_list.last,
									   app_state->drag_tab->panel->window);
							app_state->tab_released = true;
						} break;

						invalid_case;
					}

					ui_end_drag();
				}
			}

		}

		box->layout_style.child_layout_axis = Axis2_Y;
		box->flags |= UI_BoxFlag_DrawBorder;
		ui_box_equip_custom_draw_proc(box, ui_panel_leaf_custom_draw);

		// NOTE(hampus): Title bar
		ui_next_width(ui_fill());
		ui_row()
		{
			ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
			ui_next_width(ui_fill());
			ui_next_height(ui_em(1.3f, 1));
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
					ui_column()
					{
						ui_spacer(ui_em(0.3f, 1));
						ui_row()
						{
							for (Tab *tab = root->tab_group.first;
								 tab != 0;
								 tab = tab->next)
							{
								ui_tab_button(tab);
								ui_spacer(ui_em(0.2f, 1));
							}
						}
					}

					ui_spacer(ui_fill());

					ui_next_height(ui_em(1.3f, 1));
					ui_next_width(ui_em(1.3f, 1));
					ui_next_icon(R_ICON_CROSS);
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
				!app_state->drag_tab)
			{
				UI_Comm title_bar_comm = ui_comm_from_box(title_bar);
				if (title_bar_comm.pressed)
				{
					ui_prepare_for_drag(root->tab_group.active_tab, title_bar_comm.rel_mouse);

				}
			}
		}

		B32 has_tabs = root->tab_group.first != 0;

		UI_Box *content_box = 0;

		// NOTE(hampus): Active tab's content
		for (Tab *tab = root->tab_group.first;
			 tab != 0;
			 tab = tab->next)
		{
			if (tab == root->tab_group.active_tab)
			{
				// NOTE(hampus): Tab content
				ui_next_width(ui_pct(1, 1));
				ui_next_height(ui_pct(1, 0));
				content_box = ui_box_make(0,
										  str8_lit("ContentBox"));
				ui_parent(content_box)
				{
					ui_spacer(ui_em(0.5f, 1));
					ui_next_width(ui_pct(1, 0));
					ui_next_height(ui_pct(1, 0));
					ui_row()
					{
						ui_spacer(ui_em(0.5f, 1));
						ui_next_width(ui_pct(1, 0));
						ui_next_height(ui_pct(1, 0));
						ui_column()
						{
							if (ui_button(str8_lit("Split panel X")).pressed)
							{
								ui_panel_split(root, Axis2_X);
							}
							ui_spacer(ui_em(0.5f, 1));
							if (ui_button(str8_lit("Split panel Y")).pressed)
							{
								ui_panel_split(root, Axis2_Y);
							}
							ui_spacer(ui_em(0.5f, 1));
							if (ui_button(str8_lit("Close panel")).pressed)
							{
								PanelClose *data = cmd_push(&app_state->cmd_buffer, CmdKind_PanelClose);
								data->panel = root;
							}
							ui_spacer(ui_em(1.0f, 1));
							if (ui_button(str8_lit("Add tab")).pressed)
							{
								Tab *new_tab = ui_tab_alloc(app_state->perm_arena);
								TabAttach *data = cmd_push(&app_state->cmd_buffer, CmdKind_TabAttach);
								data->tab = new_tab;
								data->panel = root;
							}
							ui_spacer(ui_em(0.5f, 1));
							if (ui_button(str8_lit("Close tab")).pressed)
							{
								TabDelete *data = cmd_push(&app_state->cmd_buffer, CmdKind_TabClose);
								data->tab = tab;
							}
							ui_spacer(ui_em(0.5f, 1));
						}
					}
				}
			}
		}

		// NOTE(hampus): Drag & split overlay
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
					ui_next_vert_gradient(top_color, top_color);
					UI_Box *overlay_box = ui_box_make(UI_BoxFlag_DrawBackground,
													  str8_lit("OverlayBox"));
				}
				else
				{
					switch (hover_axis)
					{
						case Axis2_X:
						{
							container->layout_style.child_layout_axis = Axis2_X;
							if (hover_side == Side_Max)
							{
								ui_spacer(ui_fill());
							}
							ui_next_width(ui_pct(0.5f, 1));
							ui_next_height(ui_pct(1, 1));
							ui_next_vert_gradient(top_color, top_color);
							UI_Box *overlay_box = ui_box_make(UI_BoxFlag_DrawBackground,
															  str8_lit("OverlayBox"));
						} break;

						case Axis2_Y:
						{
							container->layout_style.child_layout_axis = Axis2_Y;
							if (hover_side == Side_Max)
							{
								ui_spacer(ui_fill());
							}
							ui_next_height(ui_pct(0.5f, 1));
							ui_next_width(ui_pct(1, 1));
							ui_next_vert_gradient(top_color, top_color);
							UI_Box *overlay_box = ui_box_make(UI_BoxFlag_DrawBackground,
															  str8_lit("OverlayBox"));
						} break;

						default: break;
					}
				}
			}
		}

		ui_pop_string();
	}

	if (app_state->drag_tab)
	{
		if (root != app_state->drag_tab->panel)
		{
			// NOTE(hampus): We don't want to consume all the events
			// if we're dragging a tab in order to make the others panels
			// able to get hovered
			// NOTE(hampus): Consume all non-taken events
			UI_Comm root_comm = ui_comm_from_box(box);
		}
	}

	ui_pop_parent();
}

internal Void
ui_panel_draw_debug(Panel *root)
{
	local F32 indent_level = 0;
	Str8 split_axis = root->split_axis == Axis2_X ? str8_lit("X") : str8_lit("Y");
	ui_textf("%"PRISTR8": %"PRISTR8, str8_expand(root->string), str8_expand(split_axis));
	indent_level += 1;
	ui_row_begin();
	ui_spacer(ui_em(indent_level, 1));

	ui_column()
	{
		for (Side side = (Side) 0;
			 side < Side_COUNT;
			 side++)
		{
			if (root->children[side])
			{
				ui_panel_draw_debug(root->children[side]);
			}
		}
	}

	ui_row_end();
	indent_level -= 1;
}

#include "ui_panel_cmd.c"

internal S32
os_main(Str8List arguments)
{
	Arena *perm_arena = arena_create();
	app_state = push_struct(perm_arena, AppState);
	app_state->perm_arena = perm_arena;

	log_init(str8_lit("log.txt"));

	Gfx_Context gfx = gfx_init(0, 0, 720, 480, str8_lit("Title"));

	R_Context *renderer = render_init(&gfx);
	Arena *frame_arenas[2];
	frame_arenas[0] = arena_create();
	frame_arenas[1] = arena_create();

	UI_Context *ui = ui_init();

	U64 start_counter = os_now_nanoseconds();
	F64 dt = 0;

	app_state->cmd_buffer.buffer = push_array(app_state->perm_arena, Cmd, CMD_BUFFER_SIZE);
	app_state->cmd_buffer.size = CMD_BUFFER_SIZE;

	Window *first_window = push_struct(app_state->perm_arena, Window);

	first_window->root_panel = push_struct(app_state->perm_arena, Panel);
	first_window->root_panel->string = str8_pushf(app_state->perm_arena, "RootPanel");

	first_window->root_panel->window = first_window;

	first_window->size = v2f32(1, 1);

	dll_push_back(app_state->window_list.first, app_state->window_list.last, first_window);

	{
		Tab *tab = ui_tab_alloc(app_state->perm_arena);
		TabAttach attach =
		{
			.tab = tab,
			.panel = first_window->root_panel,
			.set_active = true,
		};
		tab_attach(&attach);
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

		ui_next_width(ui_fill());
		ui_next_height(ui_fill());
		UI_Box *window_root_parent = ui_box_make(0, str8_lit("RootWindow"));
		app_state->root_window = window_root_parent;
		if (app_state->drag_tab ||
			app_state->drag_candidate)
		{
			Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, prev_mouse_pos);
			app_state->current_drag_pos = v2f32_add_v2f32(app_state->current_drag_pos,
														  mouse_delta);
		}

		local B32 begin_drag = false;

		if (begin_drag)
		{
			// NOTE(hampus): If it is the last tab of the window,
			// we don't need to allocate a new panel. Just use
			// the tab's panel
			if (app_state->remove_tab_from_panel)
			{
				Window *new_window = push_struct(app_state->perm_arena, Window);
				new_window->root_panel = ui_panel_alloc(app_state->perm_arena);
				new_window->root_panel->window = new_window;

				dll_push_front(app_state->window_list.first, app_state->window_list.last, new_window);

				{
					TabAttach attach =
					{
						.tab = app_state->drag_tab,
						.panel = new_window->root_panel,
						.set_active = true,
					};
					tab_attach(&attach);
				}
				new_window->size = app_state->new_window_pct;
			}
			else
			{
				app_state->drag_tab->panel->sibling = 0;
			}
			begin_drag = false;
		}

		if (app_state->drag_tab)
		{
			app_state->drag_tab->panel->window->pos = app_state->current_drag_pos;
		}

		B32 currently_dragging = app_state->drag_candidate != 0;
		if (currently_dragging)
		{
			Vec2F32 delta = v2f32_sub_v2f32(app_state->current_drag_pos, app_state->start_drag_pos);
			if (f32_abs(delta.x) > 20 || f32_abs(delta.y) > 20)
			{
				begin_drag = true;
				ui_begin_drag();
			}
		}

		ui_parent(window_root_parent)
		{
			for (Window *window = app_state->window_list.first;
				 window != 0;
				 window = window->next)
			{
				ui_panel(window->root_panel);
			}
		}

		if (left_mouse_released && app_state->drag_tab && !app_state->tab_released)
		{
			app_state->tab_released = true;
		}

		if (app_state->tab_released)
		{
			ui_end_drag();
		}

		app_state->tab_released = false;

		ui_end();

		for (U64 i = 0; i < app_state->cmd_buffer.pos; ++i)
		{
			Cmd *cmd = app_state->cmd_buffer.buffer + i;
			switch (cmd->kind)
			{
				case CmdKind_TabAttach: tab_attach(cmd->data); break;
				case CmdKind_TabClose:  tab_delete(cmd->data); break;

				case CmdKind_PanelSplit:          panel_split(cmd->data);            break;
				case CmdKind_PanelSplitAndAttach: panel_split_and_attach(cmd->data); break;
				case CmdKind_PanelSetActiveTab:   panel_set_active_tab(cmd->data);   break;
				case CmdKind_PanelClose:          panel_close(cmd->data);            break;
			}
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