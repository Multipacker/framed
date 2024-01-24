////////////////////////////////
//~ hampus: Short term
//
// [ ] @widget Line edit
// [ ] @feature Death animations
// [ ] @feature Focused box
// [ ] @feature Keyboard navigation
// [ ] @code Change animation speed per-box

////////////////////////////////
//~ hampus: Medium term
//

////////////////////////////////
//~ hampus: Long term
//

#define UI_ICON_FONT_PATH "data/fonts/fontello.ttf"

#define UI_GATHER_STATS 1

#if UI_GATHER_STATS
#  define ui_stats_inc_val(name) ui_get_current_stats()->name++
#else
#  define ui_stats_inc_val(name)
#endif

////////////////////////////////
//~ hampus: Globals

global UI_Context *ui_ctx;

////////////////////////////////
//~ hampus: Accessor functions

internal F64
ui_dt(Void)
{
	F64 result = ui_ctx->dt;
	return(result);
}

internal Render_Context *
ui_renderer(Void)
{
	Render_Context *result = ui_ctx->renderer;
	return(result);
}

internal Gfx_EventList *
ui_events(Void)
{
	Gfx_EventList *result = ui_ctx->event_list;
	return(result);
}

internal UI_Stats *
ui_get_current_stats(Void)
{
	UI_Stats *result = ui_ctx->stats + (ui_ctx->frame_index%2);
	return(result);
}

internal UI_Stats *
ui_get_prev_stats(Void)
{
	UI_Stats *result = ui_ctx->stats + ((ui_ctx->frame_index+1)%2);
	return(result);
}

internal Vec2F32
ui_mouse_pos(Void)
{
	return(ui_ctx->mouse_pos);
}

internal Vec2F32
ui_prev_mouse_pos(Void)
{
	return(ui_ctx->prev_mouse_pos);
}

internal Arena *
ui_permanent_arena(Void)
{
	Arena *result = ui_ctx->permanent_arena;
	return(result);
}

internal Arena *
ui_frame_arena(Void)
{
	Arena *result = ui_ctx->frame_arena;
	return(result);
}

internal B32
ui_animations_enabled(Void)
{
	B32 result = ui_ctx->config.animations;
	return(result);
}

internal F32
ui_animation_speed(Void)
{
	F32 result = ui_ctx->config.animation_speed;
	return(result);
}

////////////////////////////////
//~ hampus: Keying

internal UI_Key
ui_key_null(Void)
{
	UI_Key result = {0};
	return(result);
}

internal B32
ui_key_is_null(UI_Key key)
{
	B32 result = ui_key_match(key, ui_key_null());
	return(result);
}

internal B32
ui_key_match(UI_Key a, UI_Key b)
{
	B32 result = a.value == b.value;
	return(result);
}

// TODO(simon): Switch to hash_str8 and hash_combine.
internal UI_Key
ui_key_from_string(UI_Key seed, Str8 string)
{
	UI_Key result = {0};

	if (string.size != 0)
	{
		result = seed;
		for (U64 i = 0; i < string.size; ++i)
		{
			result.value = ((result.value << 5) + result.value) + string.data[i];
		}
	}

	return(result);
}

internal UI_Key
ui_key_from_string_f(UI_Key seed, CStr fmt, ...)
{
	UI_Key result = {0};
	arena_scratch(0, 0)
	{
		va_list args;
		va_start(args, fmt);
		Str8 string = str8_pushfv(scratch, fmt, args);
		result = ui_key_from_string(seed, string);
		va_end(args);
	}
	return(result);
}

internal Str8
ui_get_hash_part_from_string(Str8 string)
{
	Str8 result = string;
	U64 index = 0;
	if (str8_find_substr8(string, str8_lit("###"), &index))
	{
		result = str8_skip(string, index + 3);
	}
	return(result);
}

internal Str8
ui_get_display_part_from_string(Str8 string)
{
	Str8 result = string;
	U64 index = 0;
	if (str8_find_substr8(string, str8_lit("##"), &index))
	{
		result = str8_chop(string, string.size - index);
	}
	return(result);
}

////////////////////////////////
//~ hampus: Sizing

internal UI_Size
ui_pixels(F32 value, F32 strictness)
{
	UI_Size result = {0};
	result.kind = UI_SizeKind_Pixels;
	result.value = value;
	result.strictness = strictness;
	return(result);
}

internal UI_Size
ui_text_content(F32 strictness)
{
	UI_Size result = {0};
	result.kind = UI_SizeKind_TextContent;
	result.strictness = strictness;
	return(result);
}

internal UI_Size
ui_pct(F32 value, F32 strictness)
{
	UI_Size result = {0};
	result.kind = UI_SizeKind_Pct;
	result.value = value;
	result.strictness = strictness;
	return(result);
}

internal UI_Size
ui_children_sum(F32 strictness)
{
	UI_Size result = {0};
	result.kind = UI_SizeKind_ChildrenSum;
	result.strictness = strictness;
	return(result);
}

internal UI_Size
ui_em(F32 value, F32 strictness)
{
	UI_Size result = {0};
	result.kind = UI_SizeKind_Pixels;
	result.value = ui_top_font_line_height() * value;
	result.strictness = strictness;
	return(result);
}

internal UI_Size
ui_fill(Void)
{
	UI_Size result = ui_pct(1, 0);
	return(result);
}

////////////////////////////////
//~ hampus: Text editing

internal UI_TextAction
ui_text_action_from_event(Gfx_Event *event)
{
	UI_TextAction result = {0};

	if (event->kind == Gfx_EventKind_KeyPress)
	{
		switch (event->key)
		{
			case Gfx_Key_Left:
			{
				result.delta = -1;
			} break;

			case Gfx_Key_Right:
			{
				result.delta = 1;
			} break;

			default: break;
		}
	}
	else if (event->kind == Gfx_EventKind_Char)
	{
		assert(event->character < 128 && "Unicode typing not yet supported!");
		result.character = (U8)event->character;
		result.delta = 1;
	}

	return(result);
}

internal UI_TextActionList
ui_text_action_list_from_events(Arena *arena, Gfx_EventList *event_list)
{
	UI_TextActionList result = {0};
	for (Gfx_Event *event = event_list->first;
		 event != 0;
		 event = event->next)
	{
		UI_TextAction text_action = ui_text_action_from_event(event);
		if (!type_is_zero(text_action))
		{
			UI_TextActionNode *node = push_struct(arena, UI_TextActionNode);
			node->action = text_action;
			dll_push_back(result.first, result.last, node);
		}
	}
	return(result);
}

internal UI_TextOp
ui_text_op_from_state_and_action(Arena *arena, UI_TextEditState *state, UI_TextAction *action)
{
	UI_TextOp result = {0};
	result.new_cursor = state->cursor;
	result.new_mark = state->mark;

	if (action->character)
	{
		result.replace_string.data = push_array(arena, U8, 1);
		*result.replace_string.data = action->character;
		result.replace_string.size = 1;

		result.range.x = result.new_mark;
		result.range.y = result.new_cursor;
	}

	result.new_cursor += action->delta;
	result.new_mark = result.new_cursor;

	return(result);
}

////////////////////////////////
//~ hampus: Box

internal UI_Comm
ui_comm_from_box(UI_Box *box)
{
	assert(!ui_key_is_null(box->key) &&
		   "Tried to gather input from a keyless box!");

	UI_Comm result = {0};
	result.box = box;
	Vec2F32 mouse_pos = gfx_get_mouse_pos(ui_ctx->renderer->gfx);

	result.rel_mouse = v2f32_sub_v2f32(mouse_pos, box->fixed_rect.min);

	B32 gather_input = true;

	RectF32 hover_region = box->fixed_rect;
	for (UI_Box *parent = box->parent;
		 parent != 0;
		 parent = parent->parent)
	{
		if (ui_box_has_flag(parent, UI_BoxFlag_Clip))
		{
			// NOTE(hampus): This should never fire
			// because you can't push the rect of
			// a keyless box
			assert(!ui_key_is_null(parent->key) &&
				   "Clipping to an unstable rectangle");
			hover_region = rectf32_intersect_rectf32(hover_region, parent->fixed_rect);
		}
	}

	if (!ui_key_is_null(ui_ctx->ctx_menu_key))
	{
		// NOTE(hampus): Check to see if this box is a
		// part of the context menu
		B32 part_of_ctx_menu = false;
		UI_Box *ctx_menu_root = ui_ctx->ctx_menu_root;
		for (UI_Box *parent = box->parent;
			 parent != 0;
			 parent = parent->parent)
		{
			if (parent == ctx_menu_root)
			{
				part_of_ctx_menu = true;
				break;
			}
		}

		if (!part_of_ctx_menu)
		{
			// NOTE(hampus): If the mouse is inside
			// the contex menu and it is not a part of
			// it, don't gather input.
			if (rectf32_contains_v2f32(ctx_menu_root->fixed_rect, mouse_pos))
			{
				gather_input = false;
			}
		}
	}

	B32 mouse_over = false;

	if (rectf32_contains_v2f32(hover_region, mouse_pos))
	{
		mouse_over = true;
	}

	if (gather_input)
	{
		if (ui_box_is_active(box))
		{
			result.dragging = true;
			result.drag_delta = v2f32_sub_v2f32(ui_ctx->prev_mouse_pos, ui_ctx->mouse_pos);
			ui_ctx->hot_key = box->key;
		}

		if (ui_key_is_null(ui_ctx->hot_key) && mouse_over)
		{
			ui_ctx->hot_key = box->key;
		}

		if (mouse_over)
		{
			if (ui_box_is_hot(box))
			{
				result.hovering = true;
			}

			Gfx_EventList *event_list = ui_ctx->event_list;
			for (Gfx_Event *node = event_list->first;
				 node != 0;
				 node = node->next)
			{
				switch (node->kind)
				{
					case Gfx_EventKind_KeyRelease:
					{
						switch (node->key)
						{
							case Gfx_Key_MouseLeft:
							{
								result.released = true;
								dll_remove(event_list->first, event_list->last, node);
								if (ui_key_match(box->key, ui_ctx->prev_active_key))
								{
									result.clicked = true;
								}
							} break;

							case Gfx_Key_MouseLeftDouble:
							{
								dll_remove(event_list->first, event_list->last, node);
							} break;

							case Gfx_Key_MouseRight:
							{
								result.right_released = true;
								dll_remove(event_list->first, event_list->last, node);
							} break;

							default:
							{
							} break;
						}
					} break;

					case Gfx_EventKind_KeyPress:
					{
						switch (node->key)
						{
							case Gfx_Key_MouseLeft:
							{
								if (ui_box_has_flag(box, UI_BoxFlag_Clickable))
								{
									result.pressed = true;
									ui_ctx->active_key = box->key;
									dll_remove(event_list->first, event_list->last, node);
									ui_ctx->focus_key = box->key;
								}
							} break;

							case Gfx_Key_MouseRight:
							{
								if (ui_box_has_flag(box, UI_BoxFlag_Clickable))
								{
									dll_remove(event_list->first, event_list->last, node);
									ui_ctx->focus_key = box->key;
								}
							} break;

							case Gfx_Key_MouseLeftDouble:
							{
								if (ui_box_has_flag(box, UI_BoxFlag_Clickable))
								{
									result.double_clicked = true;
									ui_ctx->active_key = box->key;
									dll_remove(event_list->first, event_list->last, node);
									ui_ctx->focus_key = box->key;
								}
							} break;

							default:
							{
							} break;
						}
					} break;

					case Gfx_EventKind_Scroll:
					{
						if (ui_box_has_flag(box, UI_BoxFlag_ViewScroll))
						{
							result.scroll.x = -node->scroll.x;
							result.scroll.y = -node->scroll.y;
							dll_remove(event_list->first, event_list->last, node);
						}
					} break;

					default:
					{
					} break;
				}
			}
		}
	}

	return(result);
}

internal B32
ui_box_is_active(UI_Box *box)
{
	B32 result = false;
	if (!ui_key_is_null(box->key))
	{
		result = ui_key_match(ui_ctx->active_key, box->key);
	}
	return(result);
}

internal B32
ui_box_is_hot(UI_Box *box)
{
	B32 result = false;
	if (!ui_key_is_null(box->key))
	{
		result = ui_key_match(ui_ctx->hot_key, box->key);
	}
	return(result);
}

internal B32
ui_box_is_focused(UI_Box *box)
{
	B32 result = false;
	if (!ui_key_is_null(box->key))
	{
		result = ui_key_match(ui_ctx->focus_key, box->key);
	}
	return(result);
}

internal UI_Box *
ui_box_alloc(Void)
{
	assert(ui_ctx->box_storage.num_free_boxes > 0);
	UI_Box *result = (UI_Box *)ui_ctx->box_storage.first_free_box;
	ASAN_UNPOISON_MEMORY_REGION(result, sizeof(UI_Box));
	ui_ctx->box_storage.first_free_box = ui_ctx->box_storage.first_free_box->next;

	memory_zero_struct(result);

	ui_ctx->box_storage.num_free_boxes--;

	return(result);
}

internal Void
ui_box_free(UI_Box *box)
{
	UI_FreeBox *free_box = (UI_FreeBox *)box;
	free_box->next = ui_ctx->box_storage.first_free_box;
	ui_ctx->box_storage.first_free_box = free_box;
	ui_ctx->box_storage.num_free_boxes++;

	ASAN_POISON_MEMORY_REGION(free_box, sizeof(UI_Box));
}

internal B32
ui_box_has_flag(UI_Box *box, UI_BoxFlags flag)
{
	B32 result = (box->flags & flag) != 0;
	return(result);
}

internal UI_Box *
ui_box_from_key(UI_Key key)
{
	UI_Box *result = 0;

	if (!ui_key_is_null(key))
	{
		U64 count = 0;
		U64 slot_index = key.value % ui_ctx->box_hash_map_count;
		for (result = ui_ctx->box_hash_map[slot_index];
			 result != 0;
			 result = result->hash_next)
		{
			if (ui_key_match(key, result->key))
			{
				break;
			}
#if UI_GATHER_STATS
			++count;
			if (ui_get_current_stats()->max_box_chain_count < count)
			{
				ui_get_current_stats()->max_box_chain_count = count;
			}
#endif

			ui_stats_inc_val(box_chain_count);
		}
	}

	return(result);
}

internal UI_Box *
ui_box_make(UI_BoxFlags flags, Str8 string)
{
	UI_Key key = ui_key_from_string(ui_top_seed(), ui_get_hash_part_from_string(string));

	UI_Box *result = ui_box_from_key(key);

	if (!result)
	{
		if (!ui_key_is_null(key))
		{
			result = ui_box_alloc();
			result->key = key;
			result->first_frame_touched_index = ui_ctx->frame_index;

			U64 slot_index = result->key.value % ui_ctx->box_hash_map_count;
			UI_Box *box = ui_ctx->box_hash_map[slot_index];
			if (box)
			{
				box->hash_prev = result;
				result->hash_next = box;
			}

			ui_ctx->box_hash_map[slot_index] = result;
		}
		else
		{
			result = push_struct_zero(ui_frame_arena(), UI_Box);
			result->first_frame_touched_index = ui_ctx->frame_index;
			result->key = key;

			ui_stats_inc_val(num_transient_boxes);

		}
	}
	else
	{
		ui_stats_inc_val(num_hashed_boxes);
		assert(result->last_frame_touched_index != ui_ctx->frame_index &&
			   "Hash collision!");
	}

	result->first = 0;
	result->last = 0;
	result->next = 0;
	result->prev = 0;
	UI_Box *parent = ui_top_parent();

	result->string = string;

	if (parent)
	{
		// NOTE(hampus): This would not be the case for the root.
		dll_push_back(parent->first, parent->last, result);
	}

	result->rect_style   = *ui_top_rect_style();
	result->text_style   = *ui_top_text_style();
	result->layout_style = *ui_top_layout_style();

	result->clip_rect = ui_ctx->clip_rect_stack.first;

	result->parent = parent;
	result->flags = flags | result->layout_style.box_flags;

	assert(!(ui_box_has_flag(result, UI_BoxFlag_AnimateX) &&
			 ui_key_is_null(result->key) &&
			 "Why would you animate a keyless box"));

	assert(!(ui_box_has_flag(result, UI_BoxFlag_AnimateY) &&
			 ui_key_is_null(result->key) &&
			 "Why would you animate a keyless box"));

	assert(!(ui_box_has_flag(result, UI_BoxFlag_AnimateWidth) &&
			 ui_key_is_null(result->key) &&
			 "Why would you animate a keyless box"));

	assert(!(ui_box_has_flag(result, UI_BoxFlag_AnimateHeight) &&
			 ui_key_is_null(result->key) &&
			 "Why would you animate a keyless box"));

	result->last_frame_touched_index = ui_ctx->frame_index;

	if (ui_box_has_flag(result, UI_BoxFlag_FloatingX))
	{
		result->rel_pos.v[Axis2_X] = result->layout_style.relative_pos.v[Axis2_X];
	}

	if (ui_box_has_flag(result, UI_BoxFlag_FloatingY))
	{
		result->rel_pos.v[Axis2_Y] = result->layout_style.relative_pos.v[Axis2_Y];
	}

	if (ui_ctx->rect_style_stack.auto_pop)
	{
		ui_pop_rect_style();
		ui_ctx->rect_style_stack.auto_pop = false;
	}

	if (ui_ctx->text_style_stack.auto_pop)
	{
		ui_pop_text_style();
		ui_ctx->text_style_stack.auto_pop = false;
	}

	if (ui_ctx->layout_style_stack.auto_pop)
	{
		ui_pop_layout_style();
		ui_ctx->layout_style_stack.auto_pop = false;
	}

	return(result);
}

internal UI_Box *
ui_box_make_f(UI_BoxFlags flags, CStr fmt, ...)
{
	UI_Box *result = {0};
	// TODO(hampus): This won't work with debug_string
	// since it needs to be persistent across the frame
	arena_scratch(0, 0)
	{
		va_list args;
		va_start(args, fmt);
		Str8 string = str8_pushfv(scratch, fmt, args);
		result = ui_box_make(flags, string);
		va_end(args);
	}

	return(result);
}

internal Void
ui_box_equip_display_string(UI_Box *box, Str8 string)
{
	Str8 display_string = ui_get_display_part_from_string(string);
	box->string = str8_copy(ui_frame_arena(), display_string);
}

internal Void
ui_box_equip_custom_draw_proc(UI_Box *box, UI_CustomDrawProc *proc)
{
	box->custom_draw = proc;
}

internal RectF32
ui_box_get_fixed_rect(UI_Box *box)
{
	RectF32 result = {0};
	result.x0 = box->fixed_rect.x0 + box->rel_pos.x - box->rel_pos_animated.x;
	result.x1 = box->fixed_rect.x1 + box->rel_pos.x - box->rel_pos_animated.x;
	result.y0 = box->fixed_rect.y0 + box->rel_pos.y - box->rel_pos_animated.y;
	result.y1 = box->fixed_rect.y1 + box->rel_pos.y - box->rel_pos_animated.y;
	return(result);
}

internal B32
ui_box_was_created_this_frame(UI_Box *box)
{
	B32 result = box->first_frame_touched_index == box->last_frame_touched_index;
	return(result);
}

internal B32
ui_mouse_is_inside_box(UI_Box *box)
{
	B32 result = rectf32_contains_v2f32(box->fixed_rect, ui_ctx->mouse_pos);
	return(result);
}

////////////////////////////////
//~ hampus: Ctx menu

internal B32
ui_ctx_menu_begin(UI_Key key)
{
	B32 is_open = ui_key_match(key, ui_ctx->ctx_menu_key);

	ui_push_parent(ui_ctx->ctx_menu_root);
	ui_push_clip_rect(&ui_ctx->root->fixed_rect, 0);

	if (is_open)
	{
		ui_next_extra_box_flags(UI_BoxFlag_DrawBackground |
								UI_BoxFlag_DrawBorder |
								UI_BoxFlag_AnimateHeight |
								UI_BoxFlag_DrawDropShadow |
								UI_BoxFlag_Clip);
	}

	ui_push_seed(key);
	ui_named_column_begin(str8_lit("CtxMenuColumn"));

	return(is_open);
}

internal Void
ui_ctx_menu_end(Void)
{
	ui_column_end();
	ui_pop_seed();
	ui_pop_clip_rect();
	ui_pop_parent();
}

internal Void
ui_ctx_menu_open(UI_Key anchor, Vec2F32 offset, UI_Key menu)
{
	ui_ctx->next_ctx_menu_key = menu;
	ui_ctx->next_ctx_menu_anchor_key = anchor;
	ui_ctx->next_anchor_offset = offset;
}

internal Void
ui_ctx_menu_close(Void)
{
	ui_ctx->next_ctx_menu_key        = ui_key_null();
	ui_ctx->next_ctx_menu_anchor_key = ui_key_null();
}

internal B32
ui_ctx_menu_is_open(Void)
{
	B32 result = !ui_key_is_null(ui_ctx->ctx_menu_key);
	return(result);
}

internal UI_Key
ui_ctx_menu_key(Void)
{
	UI_Key result = ui_ctx->ctx_menu_key;
	return(result);
}

////////////////////////////////
//~ hampus: Tooltip

internal Void
ui_tooltip_begin(Void)
{
	ui_push_clip_rect(&ui_ctx->root->fixed_rect, 0);
	ui_push_parent(ui_ctx->tooltip_root);
}

internal Void
ui_tooltip_end(Void)
{
	ui_pop_parent();
	ui_pop_clip_rect();
}

////////////////////////////////
//~ hampus: Init, begin end

internal UI_Context *
ui_init(Void)
{
	UI_Context *result = 0;

	Arena *permanent_arena = arena_create();
	Arena *frame_arena = arena_create();

	result = push_struct(permanent_arena, UI_Context);
	ui_ctx = result;

	result->permanent_arena = permanent_arena;
	result->frame_arena = frame_arena;

	result->box_storage.storage_count = 4096;
	result->box_storage.boxes = push_array(permanent_arena, UI_Box, result->box_storage.storage_count);

	for (U64 i = 0; i < 4096; ++i)
	{
		ui_box_free(result->box_storage.boxes + i);
	}

	result->box_hash_map_count = 1024;
	result->box_hash_map = push_array(permanent_arena, UI_Box *, result->box_hash_map_count);

	ui_ctx = 0;

	return(result);
}

internal Void
ui_begin(UI_Context *ctx, Gfx_EventList *event_list, Render_Context *renderer, F64 dt)
{
	ui_ctx = ctx;

	ui_ctx->renderer = renderer;
	ui_ctx->event_list = event_list;
	ui_ctx->dt = dt;

	ui_ctx->prev_active_key = ui_ctx->active_key;

	ui_ctx->mouse_pos = gfx_get_mouse_pos(ui_ctx->renderer->gfx);

	ui_ctx->ctx_menu_key        = ui_ctx->next_ctx_menu_key;
	ui_ctx->ctx_menu_anchor_key = ui_ctx->next_ctx_menu_anchor_key;
	ui_ctx->anchor_offset       = ui_ctx->next_anchor_offset;

	B32 left_mouse_released = false;
	B32 left_mouse_pressed = false;
	B32 escape_key_pressed = false;
	for (Gfx_Event *node = event_list->first;
		 node != 0;
		 node = node->next)
	{
		switch (node->kind)
		{
			case Gfx_EventKind_KeyRelease:
			{
				switch (node->key)
				{
					case Gfx_Key_MouseLeft:
					case Gfx_Key_MouseLeftDouble:
					{
						left_mouse_released = true;
					} break;

					default:
					{
					} break;
				}
			} break;

			case Gfx_EventKind_KeyPress:
			{
				if (node->key == Gfx_Key_Escape)
				{
					escape_key_pressed = true;
				}

				if (node->key == Gfx_Key_MouseLeft)
				{
					left_mouse_pressed = true;
				}
			} break;

			default:
			{
			} break;
		}
	}

	for (U64  i = 0; i < ui_ctx->box_hash_map_count; ++i)
	{
		UI_Box *box = ui_ctx->box_hash_map[i];
		while (box)
		{
			UI_Box *next = box->hash_next;
			if (ui_box_is_active(box))
			{
				if (left_mouse_released)
				{
					ui_ctx->active_key = ui_key_null();
				}
			}

			B32 evict = box->last_frame_touched_index < (ui_ctx->frame_index - 1) ||
				ui_key_is_null(box->key);
			if (evict)
			{
				if (box == ui_ctx->box_hash_map[i])
				{
					ui_ctx->box_hash_map[i] = box->hash_next;
				}

				if (box->hash_next)
				{
					box->hash_next->hash_prev = box->hash_prev;
				}

				if (box->hash_prev)
				{
					box->hash_prev->hash_next = box->hash_next;
				}

				box->hash_next = 0;
				box->hash_prev = 0;
				ui_box_free(box);
			}

			box = next;
		}
	}

	// NOTE(hampus): Setup default config
	ui_ctx->config.animations = true;
	ui_ctx->config.animation_speed = 20;

	ui_ctx->hot_key = ui_key_null();

	Vec4F32 color = v4f32(0.1f, 0.1f, 0.1f, 1.0f);

	// NOTE(hampus): Setup default styling

	UI_TextStyle *text_style = ui_push_text_style();
	text_style->color = v4f32(0.9f, 0.9f, 0.9f, 1.0f);
	text_style->font = str8_lit("data/fonts/Inter-Regular.ttf");
	text_style->font_size = 15;

	text_style->padding.v[Axis2_X] = (F32)ui_top_font_line_height();

	UI_LayoutStyle *layout_style = ui_push_layout_style();
	layout_style->child_layout_axis = Axis2_Y;

	UI_RectStyle *rect_style = ui_push_rect_style();
	rect_style->color[Corner_TopLeft]     = color;
	rect_style->color[Corner_TopRight]    = color;
	rect_style->color[Corner_BottomLeft]  = color;
	rect_style->color[Corner_BottomRight] = color;
	rect_style->border_color              = v4f32(0.6f, 0.6f, 0.6f, 1.0f);
	rect_style->border_thickness          = 1;
	F32 radius                            = (F32)ui_top_font_line_height() * 0.1f;
	rect_style->radies                    = v4f32(radius, radius, radius, radius);
	rect_style->softness                  = 1;

	Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);
	Vec2F32 max_clip;
	max_clip.x = (F32) client_area.x;
	max_clip.y = (F32) client_area.y;

	RectF32 *clip_rect = push_struct(ui_frame_arena(), RectF32);
	clip_rect->max = max_clip;

	ui_push_clip_rect(clip_rect, false);
	ui_push_seed(ui_key_from_string(ui_key_null(), str8_lit("RootSeed")));

	ui_next_width(ui_pixels(max_clip.x, 1));
	ui_next_height(ui_pixels(max_clip.y, 1));
	ui_ctx->root = ui_box_make(UI_BoxFlag_OverflowX |
							   UI_BoxFlag_OverflowY,
							   str8_lit("Root"));

	ui_push_parent(ui_ctx->root);

	ui_next_relative_pos(Axis2_X, ui_ctx->mouse_pos.x+10);
	ui_next_relative_pos(Axis2_Y, ui_ctx->mouse_pos.y);
	ui_ctx->tooltip_root = ui_box_make(UI_BoxFlag_FloatingPos,
									   str8_lit("TooltipRoot"));

	ui_next_width(ui_children_sum(1));
	ui_next_height(ui_children_sum(1));
	ui_ctx->ctx_menu_root = ui_box_make(UI_BoxFlag_FloatingPos,
										str8_lit("CtxMenuRoot"));

	ui_next_width(ui_pct(1, 1));
	ui_next_height(ui_pct(1, 1));
	ui_ctx->normal_root = ui_box_make(0,
									  str8_lit("NormalRoot"));

	if (!ui_key_is_null(ui_ctx->ctx_menu_key))
	{
		if (left_mouse_pressed)
		{
			UI_Box *ctx_menu_root = ui_ctx->ctx_menu_root;
			B32 clicked_inside_context_menu = rectf32_contains_v2f32(ctx_menu_root->fixed_rect, ui_ctx->mouse_pos);

			if (!clicked_inside_context_menu)
			{
				ui_ctx_menu_close();
			}
		}

		if (escape_key_pressed)
		{
			ui_ctx_menu_close();
		}
	}
	else
	{
		if (escape_key_pressed)
		{
			ui_ctx->focus_key = ui_key_null();
		}
	}

	ui_push_parent(ui_ctx->normal_root);
}

internal Void
ui_end(Void)
{
	Debug_Time debug_time = debug_function_begin();
	// NOTE(hampus): Normal root
	ui_pop_parent();

	// NOTE(hampus): Master root
	ui_pop_parent();

	// NOTE(hampus): Root clip rect
	ui_pop_clip_rect();

	if (!ui_key_is_null(ui_ctx->ctx_menu_key))
	{
		Vec2F32 anchor_pos = {0};
		if (!ui_key_is_null(ui_ctx->ctx_menu_anchor_key))
		{
			UI_Box *anchor = ui_box_from_key(ui_ctx->ctx_menu_anchor_key);
			if (anchor)
			{
				anchor_pos = v2f32(anchor->fixed_rect.min.x, anchor->fixed_rect.max.y);
			}
			else
			{
				// TODO(hampus): This doesn't solve the problem
				// if the context menu doesn't have an anchor
				ui_ctx_menu_close();
			}
		}

		anchor_pos = v2f32_add_v2f32(anchor_pos, ui_ctx->anchor_offset);

		ui_ctx->ctx_menu_root->rel_pos = anchor_pos;
	}

	if (!ui_key_is_null(ui_ctx->hot_key))
	{
		UI_Box *hot_box = ui_box_from_key(ui_ctx->hot_key);
		gfx_set_cursor(ui_ctx->renderer->gfx, hot_box->rect_style.hover_cursor);
	}
	else
	{
		gfx_set_cursor(ui_ctx->renderer->gfx, Gfx_Cursor_Arrow);
	}

	ui_layout(ui_ctx->root);
	ui_draw(ui_ctx->root);

	ui_ctx->prev_mouse_pos = ui_ctx->mouse_pos;
	ui_ctx->parent_stack = 0;
	ui_ctx->seed_stack = 0;
	arena_pop_to(ui_frame_arena(), 0);
	ui_ctx->rect_style_stack.first = 0;
	ui_ctx->text_style_stack.first = 0;
	ui_ctx->layout_style_stack.first = 0;
	ui_ctx->clip_rect_stack.first = 0;
	ui_ctx->frame_index++;
	memory_zero_struct(ui_get_current_stats());
	ui_ctx = 0;
	debug_function_end(debug_time);
}

////////////////////////////////
//~ hampus: Layout pass

internal Void
ui_layout(UI_Box *root)
{
	debug_function()
	{
		for (Axis2 axis = Axis2_X; axis < Axis2_COUNT; ++axis)
		{
			ui_solve_independent_sizes(root, axis);
			ui_solve_upward_dependent_sizes(root, axis);
			ui_solve_downward_dependent_sizes(root, axis);
			ui_solve_size_violations(root, axis);
			ui_calculate_final_rect(root, axis);
		}
	}
}

internal Void
ui_solve_independent_sizes(UI_Box *root, Axis2 axis)
{
	// NOTE(hampus): UI_SizeKind_TextContent, UI_SizeKind_Pixels
	Render_Font *font = render_font_from_key(ui_ctx->renderer, ui_font_key_from_text_style(&root->text_style));

	if (root->layout_style.size[axis].kind == UI_SizeKind_Null)
	{
		root->layout_style.size[axis].kind = UI_SizeKind_TextContent;
		root->layout_style.size[axis].strictness = 1;
	}
	UI_Size size = root->layout_style.size[axis];

	switch (size.kind)
	{
		case UI_SizeKind_Pixels:
		{
			root->fixed_size.v[axis] = size.value;
			root->fixed_size.v[axis] = f32_floor(root->fixed_size.v[axis]);
		} break;

		case UI_SizeKind_TextContent:
		{
			Vec2F32 text_dim = {0};
			if (root->text_style.icon)
			{
				text_dim = render_measure_character(font, root->text_style.icon);
			}
			else
			{
				text_dim = render_measure_text(font, root->string);
			}
			root->fixed_size.v[axis] = text_dim.v[axis] + root->text_style.padding.v[axis];
		} break;

		default: break;
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_solve_independent_sizes(child, axis);
	}
}

internal Void
ui_solve_upward_dependent_sizes(UI_Box *root, Axis2 axis)
{
	// NOTE(hampus): UI_SizeKind_Pct
	UI_Size size = root->layout_style.size[axis];
	if (size.kind == UI_SizeKind_Pct)
	{
		assert(root->parent &&
			   "Percent of parent without a parent");

		assert(root->parent->layout_style.size[axis].kind != UI_SizeKind_ChildrenSum &&
			   "Cyclic sizing behaviour");

		F32 parent_size = root->parent->fixed_size.v[axis];
		root->fixed_size.v[axis] = parent_size * size.value;
		root->fixed_size.v[axis] = f32_floor(root->fixed_size.v[axis]);
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_solve_upward_dependent_sizes(child, axis);
	}
}

internal Void
ui_solve_downward_dependent_sizes(UI_Box *root, Axis2 axis)
{
	// NOTE(hampus): UI_SizeKind_ChildrenSum
	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_solve_downward_dependent_sizes(child, axis);
	}

	UI_Size size = root->layout_style.size[axis];
	if (size.kind == UI_SizeKind_ChildrenSum)
	{
		F32 children_total_size = 0;
		Axis2 child_layout_axis = root->layout_style.child_layout_axis;
		for (UI_Box *child = root->first;
			 child != 0;
			 child = child->next)
		{
			if (!ui_box_has_flag(child, (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis)))
			{
				F32 child_size = child->fixed_size.v[axis];
				if (axis == child_layout_axis)
				{
					children_total_size += child_size;
				}
				else
				{
					children_total_size = f32_max(child_size, children_total_size);
				}
			}
		}

		root->fixed_size.v[axis] = children_total_size;
		root->fixed_size.v[axis] = f32_floor(root->fixed_size.v[axis]);
	}
}

internal Void
ui_solve_size_violations(UI_Box *root, Axis2 axis)
{
	F32 available_space = root->fixed_size.v[axis];

	F32 taken_space = 0;
	F32 total_fixup_budget = 0;
	if (!(ui_box_has_flag(root, (UI_BoxFlags)(UI_BoxFlag_OverflowX << axis))))
	{
		for(UI_Box *child = root->first;
			child != 0;
			child = child->next)
		{
			if (!(ui_box_has_flag(child, (UI_BoxFlags)(UI_BoxFlag_FloatingX << axis))))
			{
				if (axis == root->layout_style.child_layout_axis)
				{
					taken_space += child->fixed_size.v[axis];
				}
				else
				{
					taken_space = f32_max(taken_space, child->fixed_size.v[axis]);
				}
				F32 fixup_budget_this_child = child->fixed_size.v[axis] * (1 - child->layout_style.size[axis].strictness);
				total_fixup_budget += fixup_budget_this_child;
			}
		}
	}

	if (!(ui_box_has_flag(root, (UI_BoxFlags)(UI_BoxFlag_OverflowX << axis))))
	{
		F32 violation = taken_space - available_space;
		if (violation > 0 && total_fixup_budget > 0)
		{
			for (UI_Box *child = root->first;
				 child != 0;
				 child = child->next)
			{
				if (!(ui_box_has_flag(child, (UI_BoxFlags)(UI_BoxFlag_FloatingX << axis))))
				{
					F32 fixup_budget_this_child = child->fixed_size.v[axis] * (1 - child->layout_style.size[axis].strictness);
					F32 fixup_size_this_child = 0;
					if (axis == root->layout_style.child_layout_axis)
					{
						fixup_size_this_child = fixup_budget_this_child * (violation / total_fixup_budget);
					}
					else
					{
						fixup_size_this_child = child->fixed_size.v[axis] - available_space;
					}
					fixup_size_this_child = f32_clamp(0, fixup_size_this_child, fixup_budget_this_child);
					child->fixed_size.v[axis] -= fixup_size_this_child;
					child->fixed_size.v[axis] = f32_floor(child->fixed_size.v[axis]);
					for (UI_Box *child_child = child->first;
						 child_child != 0;
						 child_child = child_child->next)
					{
						if (child_child->layout_style.size[axis].kind == UI_SizeKind_Pct)
						{
							child_child->fixed_size.v[axis] = child->fixed_size.v[axis] * child_child->layout_style.size[axis].value;
							child_child->fixed_size.v[axis] = f32_floor(child_child->fixed_size.v[axis]);
						}
					}
				}
			}
		}
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_solve_size_violations(child, axis);
	}
}

internal Void
ui_calculate_final_rect(UI_Box *root, Axis2 axis)
{

	F32 offset = 0;

	// TODO(hampus): Optimize. This if runs for every box *except* the root box.
	if (root->parent)
	{
		if (!ui_box_has_flag(root, (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis)))
		{
			if (axis == root->parent->layout_style.child_layout_axis)
			{
				UI_Box *prev = 0;
				for (prev = root->prev;
					 prev != 0;
					 prev = prev->prev)
				{
					if (!ui_box_has_flag(prev, (UI_BoxFlags) (UI_BoxFlag_FloatingX << axis)))
					{
						break;
					}
				}

				if (prev)
				{
					root->rel_pos.v[axis] = prev->rel_pos.v[axis] + prev->fixed_size.v[axis];
				}
				else
				{
					root->rel_pos.v[axis] = 0;
				}
			}
			else
			{
				root->rel_pos.v[axis] = 0;
			}
		}
		else
		{
			// NOTE(hampus): We have already set the box's position
			// that we want
		}

		root->rel_pos.v[axis] -= root->scroll.v[axis];
		offset = root->parent->fixed_rect.min.v[axis];
	}

	F32 animation_delta = (F32)(1.0 - f64_pow(2.0, -ui_animation_speed() * ui_ctx->dt));

	if (f32_abs(root->rel_pos_animated.v[axis] - root->rel_pos.v[axis]) <= 0.5f)
	{
		root->rel_pos_animated.v[axis] = root->rel_pos.v[axis];
	}
	else
	{
		root->rel_pos_animated.v[axis] += (F32)(root->rel_pos.v[axis] - root->rel_pos_animated.v[axis]) * animation_delta;
	}

	if (f32_abs(root->fixed_size_animated.v[axis] - root->fixed_size.v[axis]) <= 0.5f)
	{
		root->fixed_size_animated.v[axis] = root->fixed_size.v[axis];
	}
	else
	{
		root->fixed_size_animated.v[axis] += (F32)(root->fixed_size.v[axis] - root->fixed_size_animated.v[axis]) * animation_delta;
	}

	if (ui_box_has_flag(root, (UI_BoxFlags) (UI_BoxFlag_AnimateX << axis)) &&
		ui_animations_enabled())
	{
		if (root->first_frame_touched_index == root->last_frame_touched_index)
		{
			root->rel_pos_animated.v[axis]    = root->rel_pos.v[axis];
		}

		root->fixed_rect.min.v[axis] = offset + root->rel_pos_animated.v[axis];
	}
	else
	{
		root->fixed_rect.min.v[axis] = offset + root->rel_pos.v[axis];
	}

	if (ui_box_has_flag(root, (UI_BoxFlags) (UI_BoxFlag_AnimateWidth << axis)) &&
		ui_animations_enabled())
	{
		if (root->first_frame_touched_index == root->last_frame_touched_index)
		{
			root->fixed_size_animated.v[axis] = root->fixed_size.v[axis];
		}

		root->fixed_rect.max.v[axis] = root->fixed_rect.min.v[axis] + root->fixed_size_animated.v[axis];
	}
	else
	{
		root->fixed_rect.max.v[axis] = root->fixed_rect.min.v[axis] + root->fixed_size.v[axis];
	}

	root->fixed_rect.min.v[axis] = f32_floor(root->fixed_rect.min.v[axis]);
	root->fixed_rect.max.v[axis] = f32_floor(root->fixed_rect.max.v[axis]);

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_calculate_final_rect(child, axis);
	}
}

internal Vec2F32
ui_align_text_in_rect(Render_Font *font, Str8 string, RectF32 rect, UI_TextAlign align, Vec2F32 padding)
{
	Vec2F32 result = {0};

	Vec2F32 rect_dim = v2f32_sub_v2f32(rect.max, rect.min);

	Vec2F32 text_dim = render_measure_text(font, string);

	switch (align)
	{
		case UI_TextAlign_Center:
		{
			result = v2f32_div_f32(v2f32_sub_v2f32(rect_dim, text_dim), 2.0f);
		} break;

		case UI_TextAlign_Right:
		{
			result.y = (rect_dim.y - text_dim.y) / 2;
			result.x = rect_dim.x - text_dim.x - padding.width / 2;
		} break;

		case UI_TextAlign_Left:
		{
			result.y = (rect_dim.y - text_dim.y) / 2;
			result.x = padding.width / 2;
		} break;

		invalid_case;
	}

	result = v2f32_add_v2f32(result, rect.min);

	return(result);
}

internal Vec2F32
ui_align_character_in_rect(Render_Font *font, U32 codepoint, RectF32 rect, UI_TextAlign align)
{
	Vec2F32 result = {0};

	Vec2F32 rect_dim = v2f32_sub_v2f32(rect.max, rect.min);

	Vec2F32 text_dim = render_measure_character(font, codepoint);

	switch (align)
	{
		case UI_TextAlign_Center:
		{
			result = v2f32_div_f32(v2f32_sub_v2f32(rect_dim, text_dim), 2.0f);
		} break;

		case UI_TextAlign_Right:
		{
			result.y = (rect_dim.y - text_dim.y) / 2;
			result.x = rect_dim.x - text_dim.x;
		} break;

		case UI_TextAlign_Left:
		{
			result.y = (rect_dim.y - text_dim.y) / 2;
			result.x = 0;
		} break;

		invalid_case;
	}

	result = v2f32_add_v2f32(result, rect.min);

	return(result);
}

////////////////////////////////
//~ hampus: Draw pass

internal Void
ui_draw(UI_Box *root)
{
	render_push_clip(ui_ctx->renderer, root->clip_rect->rect->min, root->clip_rect->rect->max, root->clip_rect->clip_to_parent);
	if (root->custom_draw)
	{
		root->custom_draw(root);
	}
	else
	{
		F32 animation_delta = (F32)(1.0 - f64_pow(2.0, 3.0f*-ui_animation_speed() * ui_ctx->dt));
		if (ui_box_is_active(root))
		{
			root->active_t += (1.0f - root->active_t) * animation_delta;
		}
		else
		{
			root->active_t += (0.0f - root->active_t) * animation_delta;
		}

		if (ui_box_is_hot(root))
		{
			root->hot_t += (1.0f - root->hot_t) * animation_delta;
		}
		else
		{
			root->hot_t += (0.0f - root->hot_t) * animation_delta;
		}

		root->active_t = f32_clamp(0, root->active_t, 1.0f);
		root->hot_t = f32_clamp(0, root->hot_t, 1.0f);

		UI_RectStyle *rect_style = &root->rect_style;
		UI_TextStyle *text_style = &root->text_style;

		Render_Font *font = render_font_from_key(ui_ctx->renderer, ui_font_key_from_text_style(text_style));

		if (ui_box_has_flag(root, UI_BoxFlag_DrawDropShadow))
		{
			Vec2F32 min = v2f32_sub_v2f32(root->fixed_rect.min, v2f32(10, 10));
			Vec2F32 max = v2f32_add_v2f32(root->fixed_rect.max, v2f32(15, 15));
			// TODO(hampus): Make softness em dependent
			Render_RectInstance *instance = render_rect(ui_ctx->renderer, min, max,
														.softness = 15,
														.color = v4f32(0, 0, 0, 1));
			memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
		}

		if (ui_box_has_flag(root, UI_BoxFlag_DrawBackground))
		{
			// TODO(hampus): Correct darkening/lightening
			Render_RectInstance *instance = 0;

			F32 d = 0;
			if (ui_box_has_flag(root, UI_BoxFlag_ActiveAnimation))
			{
				d += 0.2f * root->active_t;
			}

			if (ui_box_has_flag(root, UI_BoxFlag_HotAnimation))
			{
				d += 0.2f * root->hot_t;
			}

			rect_style->color[Corner_TopLeft] = v4f32_add_v4f32(rect_style->color[Corner_TopLeft],
																v4f32(d, d, d, 0));
			rect_style->color[Corner_TopRight] = v4f32_add_v4f32(rect_style->color[Corner_TopRight],
																 v4f32(d, d, d, 0));

			instance = render_rect(ui_ctx->renderer, root->fixed_rect.min, root->fixed_rect.max,
								   .softness = rect_style->softness,
								   .slice = rect_style->slice,
								   .use_nearest = rect_style->texture_filter);
			memory_copy_array(instance->colors, rect_style->color);
			memory_copy_array(instance->radies, rect_style->radies.v);
		}

		if (ui_box_has_flag(root, UI_BoxFlag_DrawBorder))
		{
			F32 d = 0;
			if (ui_box_has_flag(root, UI_BoxFlag_HotAnimation))
			{
				d += 0.3f * root->hot_t;
			}

			rect_style->border_color = v4f32_add_v4f32(rect_style->border_color, v4f32(d, d, d, 0));
			if (ui_box_has_flag(root, UI_BoxFlag_FocusAnimation))
			{
				if (ui_box_is_focused(root))
				{
					rect_style->border_color = v4f32(1.0f, 0.8f, 0.0f, 1.0f);
				}
			}

			Render_RectInstance *instance = render_rect(ui_ctx->renderer, root->fixed_rect.min, root->fixed_rect.max, .border_thickness = rect_style->border_thickness, .color = rect_style->border_color, .softness = rect_style->softness);
			memory_copy(instance->radies, rect_style->radies.v, sizeof(Vec4F32));
		}

		if (ui_box_has_flag(root, UI_BoxFlag_DrawText))
		{
			if (text_style->icon)
			{
				Vec2F32 text_pos = ui_align_character_in_rect(font, text_style->icon, root->fixed_rect, text_style->align);
				render_character_internal(ui_ctx->renderer, text_pos, text_style->icon, font, text_style->color);
			}
			else
			{
				Vec2F32 text_pos = ui_align_text_in_rect(font, root->string, root->fixed_rect, text_style->align, text_style->padding);
				render_text_internal(ui_ctx->renderer, text_pos, root->string, font, text_style->color);
			}
		}

		render_pop_clip(ui_ctx->renderer);
		if (ui_ctx->show_debug_lines)
		{
			render_rect(ui_ctx->renderer, root->fixed_rect.min, root->fixed_rect.max, .border_thickness = 1, .color = v4f32(1, 0, 1, 1));
		}

		for (UI_Box *child = root->last;
			 child != 0;
			 child = child->prev)
		{
			ui_draw(child);
		}
	}
}

////////////////////////////////
//~ hampus: Stack helpers

internal F32
ui_top_font_line_height(Void)
{
	UI_TextStyle *text_style = ui_top_text_style();
	Render_Font *font = render_font_from_key(ui_ctx->renderer, ui_font_key_from_text_style(ui_top_text_style()));
	F32 result = 0;
	if (render_font_is_loaded(font))
	{
		result = font->line_height;
	}
	return(result);
}

internal Render_FontKey
ui_font_key_from_text_style(UI_TextStyle *text_style)
{
	Render_FontKey result = {0};
	result.path = text_style->font;
	result.font_size = text_style->font_size;
	return(result);
}

////////////////////////////////
//~ hampus: Stack managing

internal UI_Box *
ui_top_parent(Void)
{
	UI_Box *result = 0;
	if (ui_ctx->parent_stack)
	{
		result = ui_ctx->parent_stack->box;
	}
	return(result);
}

internal UI_Box *
ui_push_parent(UI_Box *box)
{
	UI_ParentStackNode *node = push_struct(ui_frame_arena(), UI_ParentStackNode);
	node->box = box;
	stack_push(ui_ctx->parent_stack, node);
	if (ui_box_has_flag(box, UI_BoxFlag_Clip))
	{
		// TODO(hampus): Add an option to clip to parent
		ui_push_clip_rect(&box->fixed_rect, false);
	}
	ui_stats_inc_val(parent_push_count);
	return(ui_top_parent());
}

internal UI_Box *
ui_pop_parent(Void)
{
	UI_Box *parent = ui_top_parent();
	if (ui_box_has_flag(parent, UI_BoxFlag_Clip))
	{
		ui_pop_clip_rect();
	}
	stack_pop(ui_ctx->parent_stack);
	return(parent);
}

internal UI_Key
ui_top_seed(Void)
{
	UI_Key result = ui_key_null();
	result = ui_ctx->seed_stack->key;
	return(result);
}

internal UI_Key
ui_push_seed(UI_Key key)
{
	UI_KeyStackNode *node = push_struct(ui_frame_arena(), UI_KeyStackNode);
	node->key = key;
	stack_push(ui_ctx->seed_stack, node);
	ui_stats_inc_val(seed_push_count);
	return(ui_top_seed());
}

internal UI_Key
ui_pop_seed(Void)
{
	stack_pop(ui_ctx->seed_stack);
	return(ui_top_seed());
}

internal Void
ui_push_string(Str8 string)
{
	ui_push_seed(ui_key_from_string(ui_top_seed(), string));
}

internal Void
ui_pop_string(Void)
{
	ui_pop_seed();
}

internal Void
ui_push_clip_rect(RectF32 *rect, B32 clip_to_parent)
{
	UI_ClipRectStackNode *node = push_struct(ui_frame_arena(), UI_ClipRectStackNode);
	node->rect = rect;
	node->clip_to_parent = clip_to_parent;
	ui_stats_inc_val(clip_rect_push_count);
	stack_push(ui_ctx->clip_rect_stack.first, node);
}

internal Void
ui_pop_clip_rect(Void)
{
	stack_pop(ui_ctx->clip_rect_stack.first);
}

internal UI_RectStyle *
ui_top_rect_style(Void)
{
	UI_RectStyle *result = ui_ctx->rect_style_stack.first;
	return(result);
}

internal UI_RectStyle *
ui_push_rect_style(Void)
{
	UI_RectStyle *rect_style = push_struct(ui_frame_arena(), UI_RectStyle);
	UI_RectStyle *first = ui_top_rect_style();
	if (first)
	{
		memory_copy_struct(rect_style, first);
	}
	rect_style->stack_next = first;
	ui_ctx->rect_style_stack.first = rect_style;

	ui_stats_inc_val(rect_style_push_count);

	return(rect_style);
}

internal void
ui_pop_rect_style(Void)
{
	ui_ctx->rect_style_stack.first = ui_ctx->rect_style_stack.first->stack_next;
}

internal UI_RectStyle *
ui_get_auto_pop_rect_style(Void)
{
	UI_RectStyle *rect_style = ui_top_rect_style();
	if (!ui_ctx->rect_style_stack.auto_pop)
	{
		rect_style = ui_push_rect_style();
		ui_ctx->rect_style_stack.auto_pop = true;
	}
	return(rect_style);
}

internal UI_TextStyle *
ui_top_text_style(Void)
{
	UI_TextStyle *result = ui_ctx->text_style_stack.first;
	return(result);
}

internal UI_TextStyle *
ui_push_text_style(Void)
{
	UI_TextStyle *text_style = push_struct(ui_frame_arena(), UI_TextStyle);
	UI_TextStyle *first = ui_top_text_style();
	if (first)
	{
		memory_copy_struct(text_style, first);
	}
	text_style->stack_next = first;
	ui_ctx->text_style_stack.first = text_style;

	ui_stats_inc_val(text_style_push_count);
	return(text_style);
}

internal UI_TextStyle *
ui_pop_text_style(Void)
{
	ui_ctx->text_style_stack.first = ui_ctx->text_style_stack.first->stack_next;
	return(ui_ctx->text_style_stack.first);
}

internal UI_TextStyle *
ui_get_auto_pop_text_style(Void)
{
	UI_TextStyle *text_style = ui_top_text_style();
	if (!ui_ctx->text_style_stack.auto_pop)
	{
		text_style = ui_push_text_style();
		ui_ctx->text_style_stack.auto_pop = true;
	}
	return(text_style);
}

internal UI_LayoutStyle *
ui_top_layout_style(Void)
{
	return(ui_ctx->layout_style_stack.first);
}

internal UI_LayoutStyle *
ui_push_layout_style(Void)
{
	UI_LayoutStyle *layout = push_struct(ui_frame_arena(), UI_LayoutStyle);
	if (ui_ctx->layout_style_stack.first)
	{
		memory_copy_struct(layout, ui_ctx->layout_style_stack.first);
	}
	layout->stack_next = ui_ctx->layout_style_stack.first;
	ui_ctx->layout_style_stack.first = layout;

	ui_stats_inc_val(layout_style_push_count);

	return(layout);
}

internal void
ui_pop_layout_style(Void)
{
	ui_ctx->layout_style_stack.first = ui_ctx->layout_style_stack.first->stack_next;
}

internal UI_LayoutStyle *
ui_get_auto_pop_layout_style(Void)
{
	UI_LayoutStyle *layout = ui_top_layout_style();
	if (!ui_ctx->layout_style_stack.auto_pop)
	{
		layout = ui_push_layout_style();
		ui_ctx->layout_style_stack.auto_pop = true;
	}
	return(layout);
}
