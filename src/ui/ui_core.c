// TODO(hampus):
// [] - Styling
// [] - Input
// [] - More layout sizes
// [] - Basic widgets
// [] - Hover cursor
// [] - Context menu
// [] - Tooltip
// [] - Custom draw functions
// [] - Keyboard navigation

global UI_Context *g_ui_ctx;

internal B32
ui_box_is_active(UI_Box *box)
{
	B32 result = false;
	if (!ui_key_is_null(box->key))
	{
		result = ui_key_match(g_ui_ctx->active_key, box->key);
	}
	return(result);
}

internal B32
ui_box_is_hot(UI_Box *box)
{
	B32 result = false;
	if (!ui_key_is_null(box->key))
	{
		result = ui_key_match(g_ui_ctx->hot_key, box->key);
	}
	return(result);
}

internal UI_Box *
ui_box_alloc(Void)
{
	assert(g_ui_ctx->box_storage.num_free_boxes > 0);
	UI_Box *result = (UI_Box *)g_ui_ctx->box_storage.first_free_box;
	g_ui_ctx->box_storage.first_free_box = g_ui_ctx->box_storage.first_free_box->next;

	memory_zero_struct(result);

	g_ui_ctx->box_storage.num_free_boxes--;
	return(result);
}

internal Void
ui_box_free(UI_Box *box)
{
	UI_FreeBox *free_box = (UI_FreeBox *)box;
	free_box->next = g_ui_ctx->box_storage.first_free_box;
	g_ui_ctx->box_storage.first_free_box = free_box;
	g_ui_ctx->box_storage.num_free_boxes++;
}

internal Arena *
ui_permanent_arena(Void)
{
	Arena *result = g_ui_ctx->permanent_arena;
	return(result);
}

internal Arena *
ui_frame_arena(Void)
{
	Arena *result = g_ui_ctx->frame_arena;
	return(result);
}

internal B32
ui_box_has_flag(UI_Box *box, UI_BoxFlags flag)
{
	B32 result = (box->flags & flag) != 0;
	return(result);
}

internal UI_RectStyle *
ui_top_rect_style(Void)
{
	UI_RectStyle *result = g_ui_ctx->rect_stack.first;
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
	g_ui_ctx->rect_stack.first = rect_style;

	return(rect_style);
}

internal void
ui_pop_rect_style(Void)
{
	g_ui_ctx->rect_stack.first = g_ui_ctx->rect_stack.first->stack_next;
}

internal UI_RectStyle *
ui_get_auto_pop_rect_style(Void)
{
	UI_RectStyle *rect_style = ui_top_rect_style();
	if (!g_ui_ctx->rect_stack.auto_pop)
	{
		rect_style = ui_push_rect_style();
		g_ui_ctx->rect_stack.auto_pop = true;
	}
	return(rect_style);
}

internal UI_TextStyle *
ui_top_text_style(Void)
{
	UI_TextStyle *result = g_ui_ctx->text_stack.first;
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
	g_ui_ctx->text_stack.first = text_style;

	return(text_style);
}

internal UI_TextStyle *
ui_pop_text_style(Void)
{
	g_ui_ctx->text_stack.first = g_ui_ctx->text_stack.first->stack_next;
	return(g_ui_ctx->text_stack.first);
}

internal UI_TextStyle *
ui_get_auto_pop_text_style(Void)
{
	UI_TextStyle *text_style = ui_top_text_style();
	if (!g_ui_ctx->text_stack.auto_pop)
	{
		text_style = ui_push_text_style();
		g_ui_ctx->text_stack.auto_pop = true;
	}
	return(text_style);
}

internal UI_LayoutStyle *
ui_top_layout_style(Void)
{
	return(g_ui_ctx->layout_stack.first);
}

internal UI_LayoutStyle *
ui_push_layout_style(Void)
{
	UI_LayoutStyle *layout = push_struct(ui_frame_arena(), UI_LayoutStyle);
	if (g_ui_ctx->layout_stack.first)
	{
		memory_copy_struct(layout, g_ui_ctx->layout_stack.first);
	}
	layout->stack_next = g_ui_ctx->layout_stack.first;
	g_ui_ctx->layout_stack.first = layout;

	return(layout);
}

internal void
ui_pop_layout_style(Void)
{
	g_ui_ctx->layout_stack.first = g_ui_ctx->layout_stack.first->stack_next;
}

internal UI_LayoutStyle *
ui_get_auto_pop_layout_style(Void)
{
	UI_LayoutStyle *layout = ui_top_layout_style();
	if (!g_ui_ctx->layout_stack.auto_pop)
	{
		layout = ui_push_layout_style();
		g_ui_ctx->layout_stack.auto_pop = true;
	}
	return(layout);
}

internal UI_Context *
ui_init(Void)
{
	UI_Context *result = 0;

	Arena *permanent_arena = arena_create();
	Arena *frame_arena = arena_create();

	result = push_struct(permanent_arena, UI_Context);
	g_ui_ctx = result;

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

	g_ui_ctx = 0;

	return(result);
}

internal Void
ui_begin(UI_Context *ui_ctx, Gfx_EventList *event_list, R_Context *renderer, R_FontKey font)
{
	g_ui_ctx = ui_ctx;
	g_ui_ctx->renderer = renderer;
	g_ui_ctx->font = font;

	B32 left_mouse_released = false;
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
						left_mouse_released = true;
					} break;

					default:
					{
					} break;
				}
			} break;

			default:
			{
			} break;
		}
	}

	for (U64  i = 0; i < g_ui_ctx->box_hash_map_count; ++i)
	{
		UI_Box *box = g_ui_ctx->box_hash_map[i];
		while (box)
		{
			UI_Box *next = box->hash_next;
				if (ui_box_is_active(box))
				{
					if (left_mouse_released)
					{
						g_ui_ctx->active_key = ui_key_null();
					}
			}

			B32 evict = box->last_frame_touched_index < (g_ui_ctx->frame_index - 1) ||
				ui_key_is_null(box->key);
				if (evict)
				{
					if (box == g_ui_ctx->box_hash_map[i])
					{
					g_ui_ctx->box_hash_map[i] = box->hash_next;
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

	// NOTE(hampus): Setup default styling

	Vec4F32 default_color = v4f32(1, 1, 1, 1);

	UI_RectStyle *rect_style = ui_push_rect_style();
	rect_style->color[0] = default_color;
	rect_style->color[1] = default_color;
	rect_style->color[2] = default_color;
	rect_style->color[3] = default_color;
	rect_style->border_thickness = 1;
	rect_style->radies = v4f32(5, 5, 5, 5);
	rect_style->softness = 1;

	UI_TextStyle *text_style = ui_push_text_style();
	text_style->color = v4f32(0, 0, 0, 0);
	text_style->font_size = 15;

	UI_LayoutStyle *layout_style = ui_push_layout_style();
	layout_style->size[Axis2_X] = ui_text_content(1);
	layout_style->size[Axis2_Y] = ui_text_content(1);
	layout_style->child_layout_axis = Axis2_Y;

	g_ui_ctx->root = ui_box_make(0, str8_lit("Root"));

	ui_push_parent(g_ui_ctx->root);
	ui_push_seed(ui_key_from_string(ui_key_null(), str8_lit("RootSeed")));
}

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

internal Void
ui_solve_independent_sizes(UI_Box *root, Axis2 axis)
{
	R_Font *font = render_font_from_key(g_ui_ctx->renderer, g_ui_ctx->font);
	switch (root->layout_style.size[axis].kind)
	{
		case UI_SizeKind_Null:
		{
			assert(false);
		} break;

		case UI_SizeKind_Pixels:
		{
			root->computed_size[axis] = root->layout_style.size[axis].value;
		} break;

		case UI_SizeKind_TextContent:
		{
	Vec2F32 text_dim = render_measure_text(font, root->string);
			root->computed_size[axis] = text_dim.v[axis];
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
ui_calculate_final_rect(UI_Box *root, Axis2 axis)
{
	if (root->parent)
	{
		if (axis == root->parent->layout_style.child_layout_axis)
		{
			for (UI_Box *prev = root->prev;
				 prev != 0;
				 prev = prev->prev)
			{
				root->computed_rel_position[axis] = prev->computed_rel_position[axis] + prev->computed_size[axis];
			}
		}
		else
		{
			root->computed_rel_position[axis] = 0;
		}

		root->rect.min.v[axis] = root->parent->rect.min.v[axis] + root->computed_rel_position[axis];
		root->rect.max.v[axis] = root->rect.min.v[axis] + root->computed_size[axis];
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_calculate_final_rect(child, axis);
	}
}

internal Void
ui_layout(UI_Box *root)
{
	for (Axis2 axis = Axis2_X; axis < Axis2_COUNT; ++axis)
	{
		ui_solve_independent_sizes(root, axis);
		ui_calculate_final_rect(root, axis);
	}
}

internal Void
ui_draw(UI_Box *root)
{
	UI_RectStyle *rect_style = &root->rect_style;
	UI_TextStyle *text_style = &root->text_style;

	if (ui_box_has_flag(root, UI_BoxFlag_DrawDropShadow))
	{
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBackground))
	{
		R_RectInstance *instance = render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .softness = rect_style->softness);
		memory_copy_array(instance->colors, rect_style->color);
		memory_copy(instance->radies, &rect_style->radies, sizeof(Vec4F32));
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawBorder))
	{
		render_rect(g_ui_ctx->renderer, root->rect.min, root->rect.max, .border_thickness = rect_style->border_thickness, .color = rect_style->border_color, .softness = rect_style->softness);
	}

	if (ui_box_has_flag(root, UI_BoxFlag_DrawText))
	{
	}

	for (UI_Box *child = root->first;
		 child != 0;
		 child = child->next)
	{
		ui_draw(child);
	}
}

internal Void
ui_end(Void)
{
	ui_pop_seed();
	ui_pop_parent();

	ui_layout(g_ui_ctx->root);
	ui_draw(g_ui_ctx->root);

	g_ui_ctx->parent_stack = 0;
	g_ui_ctx->seed_stack = 0;
	g_ui_ctx->frame_index++;
	arena_pop_to(ui_frame_arena(), 0);
	g_ui_ctx->rect_stack.first = 0;
	g_ui_ctx->text_stack.first = 0;
	g_ui_ctx->layout_stack.first = 0;
	g_ui_ctx = 0;
}

internal UI_Comm
ui_comm_from_box(UI_Box *box)
{
	UI_Comm result = {0};
	return(result);
}

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

internal UI_Key
ui_key_from_string(UI_Key seed, Str8 string)
{
	UI_Key result = {0};

	if (string.size != 0)
	{
		memory_copy_struct(&result, &seed);
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
	Arena_Temporary scratch = get_scratch(0, 0);
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(scratch.arena, fmt, args);
	result = ui_key_from_string(seed, string);
	va_end(args);
	release_scratch(scratch);
	return(result);
}

internal UI_Box *
ui_box_from_key(UI_Key key)
{
	UI_Box *result = 0;

	if (!ui_key_is_null(key))
	{
		U64 slot_index = key.value % g_ui_ctx->box_hash_map_count;
		for (result = g_ui_ctx->box_hash_map[slot_index];
			 result != 0;
			 result = result->hash_next)
		{
			if (ui_key_match(key, result->key))
			{
				break;
			}
		}
	}

	return(result);
}

internal UI_Box *
ui_box_make(UI_BoxFlags flags, Str8 string)
{
	UI_Key key = ui_key_from_string(ui_top_seed(), string);

	UI_Box *result = ui_box_from_key(key);

	if (!result)
	{
		result = ui_box_alloc();
		result->key = key;

		U64 slot_index = result->key.value % g_ui_ctx->box_hash_map_count;
		UI_Box *box = g_ui_ctx->box_hash_map[slot_index];
		if (box)
		{
			box->hash_prev = result;
			result->hash_next = box;
		}

		g_ui_ctx->box_hash_map[slot_index] = result;
	}

	result->first = 0;
	result->last = 0;
		UI_Box *parent = ui_top_parent();

		if (parent)
		{
			// NOTE(hampus): This would not be the case for the root.
			dll_push_back(parent->first, parent->last, result);
		}

		result->parent = parent;

	result->flags = flags;

	result->rect_style   = *ui_top_rect_style();
	result->text_style   = *ui_top_text_style();
	result->layout_style = *ui_top_layout_style();

	result->last_frame_touched_index = g_ui_ctx->frame_index;
	return(result);
}

internal UI_Box *
ui_box_make_f(UI_BoxFlags flags, CStr fmt, ...)
{
	UI_Box *result = {0};
	Arena_Temporary scratch = get_scratch(0, 0);
	va_list args;
	va_start(args, fmt);
	Str8 string = str8_pushfv(scratch.arena, fmt, args);
	result = ui_box_make(flags, string);
	va_end(args);
	release_scratch(scratch);
	return(result);
}

internal Void
ui_box_equip_display_string(UI_Box *box, Str8 string)
{
	box->string = string;
}

internal Void
ui_box_equip_child_layout_axis(UI_Box *box, Axis2 axis)
{
	box->layout_style.child_layout_axis = axis;
}

internal UI_Box *
ui_top_parent(Void)
{
	UI_Box *result = 0;
	if (g_ui_ctx->parent_stack)
	{
		result = g_ui_ctx->parent_stack->box;
	}
	return(result);
}

internal UI_Box *
ui_push_parent(UI_Box *box)
{
	UI_ParentStackNode *node = push_struct(ui_frame_arena(), UI_ParentStackNode);
	node->box = box;
	stack_push(g_ui_ctx->parent_stack, node);
	return(ui_top_parent());
}

internal UI_Box *
ui_pop_parent(Void)
{
	stack_pop(g_ui_ctx->parent_stack);
	return(ui_top_parent());
}

internal UI_Key
ui_top_seed(Void)
{
	UI_Key result = ui_key_null();
	if (g_ui_ctx->seed_stack)
	{
		result = g_ui_ctx->seed_stack->key;
	}
	return(result);
}

internal UI_Key
ui_push_seed(UI_Key key)
{
	UI_KeyStackNode *node = push_struct(ui_frame_arena(), UI_KeyStackNode);
	node->key = key;
	stack_push(g_ui_ctx->seed_stack, node);
	return(ui_top_seed());
}

internal UI_Key
ui_pop_seed(Void)
{
	stack_pop(g_ui_ctx->seed_stack);
	return(ui_top_seed());
}