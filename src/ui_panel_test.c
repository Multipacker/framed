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
struct Panel
{
	Panel *first;
	Panel *last;
	Panel *next;
	Panel *prev;
	Panel *parent;
	
	Axis2 split_axis;
	F32 percent_of_parent;
	
	Str8 string;
	
	UI_Box *box;
	
	U64 frame_index;
	
	B32 pinned;
};

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
	Panel *panel_to_delete;
	
	Panel *panel_to_split;
	Axis2 panel_split_axis;
	
	Panel *root_panel;
};

AppState *app_state;

internal Panel *
ui_panel_alloc(Arena *arena)
{
	Panel *result = push_struct(arena, Panel);
	result->string = str8_pushf(app_state->perm_arena, "Panel%"PRIS32, app_state->num_panels);
	app_state->num_panels++;
	return(result);
}

internal Void
ui_panel_split(Panel *first, Axis2 split_axis)
{
	app_state->panel_to_split = first;
	app_state->panel_split_axis = split_axis;
}

internal B32
ui_panel_is_leaf(Panel *panel)
{
	B32 result = !(panel->first || panel->last);
	return(result);
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

internal Void
ui_panel(Panel *root)
{
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
		ui_next_size(parent->split_axis, ui_pct(root->percent_of_parent, 0));
		ui_next_size(flipped_split_axis, ui_fill());
	}
	else
	{
		ui_next_width(ui_pct(1, 1));
		ui_next_height(ui_pct(1, 1));
	}
	
	UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
							  UI_BoxFlag_Clip, 
							  root->string);
	root->box = box;
	
	ui_push_parent(box);
	
	if (!ui_panel_is_leaf(root))
	{
		Panel *first = root->first;
		Panel *last  = root->last;
		
		ui_panel(first);
		
		ui_next_size(root->split_axis, ui_em(0.3f, 1));
		ui_next_size(!root->split_axis, ui_pct(1, 1));
		ui_next_corner_radius(0);
		ui_seed(root->string)
		{
			UI_Box *draggable_box = ui_box_make(UI_BoxFlag_HotAnimation |
												UI_BoxFlag_ActiveAnimation |
												UI_BoxFlag_DrawBackground |
												UI_BoxFlag_Clickable, 
												str8_lit("DraggableBox"));
		UI_Comm comm = ui_comm_from_box(draggable_box);
		if (comm.dragging)
		{
			F32 drag_delta = comm.drag_delta.v[root->split_axis];
			B32 left_up = drag_delta < 0;
				first->percent_of_parent -= drag_delta / box->calc_size.v[root->split_axis];
			first->percent_of_parent = f32_clamp(0, first->percent_of_parent, 1.0f); 
			
			last->percent_of_parent = 1.0f - first->percent_of_parent;
			
			first->box->layout_style.size[root->split_axis].value = first->percent_of_parent;
			last->box->layout_style.size[root->split_axis].value  = last->percent_of_parent;
		}
		}
			ui_panel(last);
	}
	else
	{
		ui_push_string(root->string);
		box->layout_style.child_layout_axis = Axis2_Y;
		box->flags |= UI_BoxFlag_DrawBorder;
		ui_box_equip_custom_draw_proc(box, ui_panel_leaf_custom_draw);
		// NOTE(hampus): Build title bar
			ui_next_width(ui_pct(1, 1));
			ui_row()
			{
				ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
			ui_next_width(ui_pct(1, 1));
			ui_next_height(ui_em(1.3f, 1));
				UI_Box *title_bar = ui_box_make(UI_BoxFlag_DrawBackground,
												str8_lit(""));
				ui_parent(title_bar)
			{
			ui_spacer(ui_em(0.3f, 1));
				ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
				ui_next_vert_corner_radius(ui_top_font_size() * 0.5f, 0);
				ui_next_child_layout_axis(Axis2_X);
				ui_next_width(ui_children_sum(1));
				ui_next_height(ui_children_sum(1));
				UI_Box *title_container = ui_box_make(UI_BoxFlag_DrawBackground |
												  UI_BoxFlag_HotAnimation |
													  UI_BoxFlag_ActiveAnimation,
													  str8_lit("TitleContainer"));
				ui_parent(title_container )
				{
					ui_next_height(ui_em(1, 1));
					ui_next_width(ui_em(1, 1));
					ui_next_icon(R_ICON_PIN);
					ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
					UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
					if (root->pinned)
					{
						pin_box_flags |= UI_BoxFlag_DrawText;
					}
					UI_Box *pin_box = ui_box_make(pin_box_flags,
												  str8_lit("PinButton"));
					
					UI_Box *title = ui_box_make(UI_BoxFlag_DrawText,
												root->string);
					
					ui_box_equip_display_string(title, root->string);
					
					ui_next_height(ui_em(1, 1));
					ui_next_width(ui_em(1, 1));
					ui_next_icon(R_ICON_CROSS);
					ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
					UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable,
													str8_lit("CloseButton"));
					
					UI_Comm pin_box_comm   = ui_comm_from_box(pin_box);
					
					UI_Comm close_box_comm = ui_comm_from_box(close_box);
					
					UI_Comm title_comm = ui_comm_from_box(title_container);
					
					if (title_comm.hovering)
					{
						pin_box->flags   |= UI_BoxFlag_DrawText;
						close_box->flags |= UI_BoxFlag_DrawText;
					}
					
					if (pin_box_comm.hovering)
					{
						pin_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation;
					}
					
					if (close_box_comm.hovering)
					{
						close_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation;
					}
					
					if (pin_box_comm.pressed)
					{
						root->pinned = !root->pinned;
					}
					
					if (close_box_comm.pressed)
					{
						if (!app_state->panel_to_delete && !root->pinned && root->parent)
						{
							app_state->panel_to_delete = root;
						}
					}
				}
				}
			}
			
			// NOTE(hampus): Content
			ui_next_width(ui_pct(1, 0));
			ui_next_height(ui_pct(1, 0));
			UI_Box *content_box = ui_box_make(0, str8_lit("Hehe"));
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
					UI_Comm comm = ui_button(str8_lit("Split X"));
						if (comm.pressed)
						{
							ui_panel_split(root, Axis2_X);
						}
						if (ui_button(str8_lit("Split Y")).pressed)
						{
							ui_panel_split(root, Axis2_Y);
						}
					}
				}
		}
		ui_pop_string();
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
		for (Panel *child = root->first;
			 child != 0;
			 child = child->next)
		{
			ui_panel_draw_debug(child);
		}
	}
	
	ui_row_end();
	indent_level -= 1;
}

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
	
	app_state->root_panel = push_struct(app_state->perm_arena, Panel);
	app_state->root_panel->string = str8_pushf(app_state->perm_arena, "RootPanel");
	gfx_show_window(&gfx);
	B32 running = true;
	while (running)
	{
		Arena *current_arena  = frame_arenas[0];
		Arena *previous_arena = frame_arenas[1];
		
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
				
				default:
				{
				} break;
			}
		}
		
		render_begin(renderer);
		
		ui_begin(ui, &events, renderer, dt);
		
		ui_panel(app_state->root_panel);

#if 0
		ui_next_relative_pos(Axis2_X, 200);
		ui_next_relative_pos(Axis2_Y, 200);
		UI_Box *debug_view = ui_box_make(UI_BoxFlag_FloatingPos , str8_lit(""));
		ui_parent(debug_view)
		{
		ui_panel_draw_debug(app_state->root_panel);
		}
#endif

		ui_end();
		
		if (app_state->panel_to_delete)
		{
			Panel *root = app_state->panel_to_delete;
			B32 is_first = root->parent->first == root;
			Panel *replacement = 0;
			if (is_first)
			{
				replacement = root->next;
			}
			else
			{
				replacement = root->prev;
			}
			
				if (root->parent->parent)
				{
				if (root->parent == root->parent->parent->first)
				{
					root->parent->parent->first = replacement;
					root->parent->parent->last->prev = replacement;
					replacement->next = root->parent->parent->last;
					replacement->prev = 0;
					replacement->percent_of_parent = 1.0f - replacement->next->percent_of_parent;
				}
				else
				{
					root->parent->parent->last = replacement;
					root->parent->parent->first->next = replacement;
					replacement->prev = root->parent->parent->first;
					replacement->next = 0;
					replacement->percent_of_parent = 1.0f - replacement->prev->percent_of_parent;
				}
				replacement->parent = root->parent->parent;
			}
			else if (root->parent)
			{
				// NOTE(hampus): We try to remove on of the root's children
				if (is_first)
				{
					app_state->root_panel = root->next;
				}
				else
				{
					app_state->root_panel = root->prev;
				}
				app_state->root_panel->first = 0;
				app_state->root_panel->last = 0;
				app_state->root_panel->next = 0;
				app_state->root_panel->prev = 0;
				app_state->root_panel->parent = 0;
			}
			else
			{
				// NOTE(hampus): We tried to remove the root. big no
			}
			
			app_state->panel_to_delete = 0;
		}
		else if (app_state->panel_to_split)
		{
			Panel *first = app_state->panel_to_split;
			Axis2 split_axis = app_state->panel_split_axis;
			
			Panel *new_parent = ui_panel_alloc(app_state->perm_arena);
			Panel *second     = ui_panel_alloc(app_state->perm_arena);
			
			new_parent->percent_of_parent = first->percent_of_parent;
			
			first->percent_of_parent = 0.5f;
			second->percent_of_parent = 0.5f;
			
			new_parent->split_axis = split_axis;
			
			Panel *next = first->next;
			Panel *prev = first->prev;
			
			new_parent->next = next;
			
			dll_push_back(new_parent->first, new_parent->last, first);
			dll_push_back(new_parent->first, new_parent->last, second);
			
			new_parent->next = next;
			new_parent->prev = prev;
			new_parent->parent = first->parent;
			
			if (first->parent)
			{
				if (first == first->parent->first)
				{
					first->parent->first = new_parent;
					first->parent->last = next;
					next->prev = new_parent;
				}
				else
				{
					first->parent->first = prev;
					first->parent->last = new_parent;
					prev->next = new_parent;
				}
			}
			else
			{
				app_state->root_panel = new_parent;
			}
			
			first->parent = new_parent;
			second->parent = new_parent;
			
			app_state->panel_to_split = 0;
		}
		
		render_end(renderer);
		
		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
		
		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;
	}
	
	return(0);
}