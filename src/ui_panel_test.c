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
	
	B32 pinned;
};

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
};

AppState *app_state;

internal Panel *
ui_panel_alloc(Arena *arena, Panel *parent, F32 percent_of_parent)
{
	Panel *result = push_struct(arena, Panel);
	result->percent_of_parent = percent_of_parent;
	result->parent = parent;
	result->string = str8_pushf(app_state->perm_arena, "Panel%"PRIS32, app_state->num_panels);
	dll_push_back(parent->first, parent->last, result);
	app_state->num_panels++;
	return(result);
}

internal Void
ui_panel_split(Panel *root, Axis2 split_axis)
{
	root->split_axis = split_axis;
	 ui_panel_alloc(app_state->perm_arena, root, 0.5f);
	ui_panel_alloc(app_state->perm_arena, root, 0.5f);
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
		R_RectInstance *instance = 0;
		
		F32 d = 0;
		
		rect_style->color[Corner_TopLeft] = v4f32_add_v4f32(rect_style->color[Corner_TopLeft],
															v4f32(d, d, d, 0));
		rect_style->color[Corner_TopRight] = v4f32_add_v4f32(rect_style->color[Corner_TopLeft],
															 v4f32(d, d, d, 0));
		instance = render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .softness = rect_style->softness, .slice = rect_style->slice);
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
		ui_next_size(flipped_split_axis, ui_pct(1, 0));
	}
	else
	{
		ui_next_width(ui_pct(1, 1));
		ui_next_height(ui_pct(1, 1));
	}
	
	UI_Box *box = ui_box_make(0, 
							  root->string);
	root->box = box;
	
	ui_push_parent(box);
	ui_push_string(root->string);
	
	if (!ui_panel_is_leaf(root))
	{
		Panel *first = root->first;
		Panel *last  = root->last;
		
		ui_panel(first);
		
		ui_next_size(root->split_axis, ui_em(0.3f, 1));
		ui_next_size(!root->split_axis, ui_pct(1, 1));
		ui_next_corner_radius(0);
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
			ui_panel(last);
	}
	else
	{
		box->layout_style.child_layout_axis = Axis2_Y;
		box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder;
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
					UI_Box *pin_box = ui_box_make(root->pinned ? UI_BoxFlag_DrawText : 0,
												  str8_lit("PinButton"));
					
					UI_Box *title = ui_box_make(UI_BoxFlag_DrawText,
												root->string);
					
					ui_box_equip_display_string(title, root->string);
					
					UI_Comm title_comm = ui_comm_from_box(title_container);
					
					UI_BoxFlags close_button_flags = 0;
					if (title_comm.hovering)
					{
						pin_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_Clickable;
						close_button_flags |= UI_BoxFlag_DrawText | UI_BoxFlag_Clickable;
					}
					
					UI_Comm pin_box_comm = ui_comm_from_box(pin_box);
					if (pin_box_comm.hovering)
					{
						pin_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation;
					}
					if (pin_box_comm.pressed)
					{
						root->pinned = !root->pinned;
					}
						
					ui_next_height(ui_em(1, 1));
					ui_next_width(ui_em(1, 1));
					ui_next_icon(R_ICON_CROSS);
					ui_next_color(v4f32(0.2f, 0.2f, 0.2f, 1.0f));
					UI_Box *close_box = ui_box_make(close_button_flags,
								str8_lit("CloseButton"));
					UI_Comm close_button_comm = ui_comm_from_box(close_box);
					
					if (close_button_comm.hovering)
					{
						close_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation;
					}
					
					if (close_button_comm.pressed && !root->pinned && root->parent)
					{
						root->parent->first = 0;
						root->parent->last = 0;
					}
				}
				}
			}
			
			// NOTE(hampus): Content
			ui_next_width(ui_pct(1, 0));
			ui_next_height(ui_pct(1, 0));
			UI_Box *content_box = ui_box_make(0, str8_lit(""));
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
						if (ui_button(str8_lit("Split X")).pressed)
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
		
	}
	ui_pop_string();
	ui_pop_parent();
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
	
	Panel *root_panel = push_struct(app_state->perm_arena, Panel);
	root_panel->string = str8_pushf(app_state->perm_arena, "RootPanel");
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
		
		ui_panel(root_panel);
		
		ui_end();
		
		render_end(renderer);
		
		arena_pop_to(previous_arena, 0);
		swap(frame_arenas[0], frame_arenas[1], Arena *);
		
		U64 end_counter = os_now_nanoseconds();
		dt = (F64) (end_counter - start_counter) / (F64) billion(1);
		start_counter = end_counter;
	}
	
	return(0);
}
