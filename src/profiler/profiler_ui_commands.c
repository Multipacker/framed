PROFILER_UI_COMMAND(panel_close);

////////////////////////////////
//~ hampus: Tab commands

PROFILER_UI_COMMAND(tab_close)
{
	ProfilerUI_TabDelete *data = (ProfilerUI_TabDelete *)params;

	ProfilerUI_Tab *tab = data->tab;
	ProfilerUI_Panel *panel = tab->panel;
	if (tab == panel->tab_group.active_tab)
	{
		if (tab->prev)
		{
			panel->tab_group.active_tab = tab->prev;
		}
		else if (tab->next)
		{
			panel->tab_group.active_tab = tab->next;
		}
		else
		{
			panel->tab_group.active_tab = 0;
		}
	}
	dll_remove(panel->tab_group.first, panel->tab_group.last, tab);
	panel->tab_group.count--;
	tab->next = 0;
	tab->prev = 0;
	panel->tab_group.view_offset_x += tab->box->fixed_size.x;
	if (panel->tab_group.count == 0)
	{
		ProfilerUI_PanelClose close =
		{
			.panel = panel,
		};
		profiler_ui_command_panel_close(&close);
	}
	log_info("Executed command: tab_close (%"PRISTR8")", str8_expand(tab->string));
}

PROFILER_UI_COMMAND(tab_attach)
{
	ProfilerUI_TabAttach *data = (ProfilerUI_TabAttach *)params;

	ProfilerUI_Panel *panel = data->panel;
	ProfilerUI_Tab *tab = data->tab;
	dll_push_back(panel->tab_group.first, panel->tab_group.last, tab);
	tab->panel = panel;
	B32 set_active = panel->tab_group.count == 0 || data->set_active;
	if (set_active)
	{
		panel->tab_group.active_tab = tab;
	}
	panel->tab_group.count++;
	log_info("Executed command: tab_attach (%"PRISTR8""" -> %"PRISTR8")", str8_expand(tab->string), str8_expand(panel->string));
}

PROFILER_UI_COMMAND(tab_reorder)
{
	ProfilerUI_TabReorder *data = (ProfilerUI_TabReorder *)params;
	ProfilerUI_Panel *panel = data->tab->panel;

	ProfilerUI_Tab *tab = data->tab;
	ProfilerUI_Tab *next = data->next;

	if (next == panel->tab_group.first)
	{
		panel->tab_group.first = tab;
	}

	if (data->side == Side_Min)
	{
		if (next->prev)
		{
			next->prev->next = tab;
		}
		tab->prev = next->prev;

		next->next = tab->next;
		if (tab->next)
		{
			tab->next->prev = next;
		}

		tab->next = next;
		next->prev = tab;
	}

	log_info("Executed command: tab_reorder");
}

////////////////////////////////
//~ hampus: UI_Panel commands

PROFILER_UI_COMMAND(panel_set_active_tab)
{
	ProfilerUI_PanelSetActiveTab *data = (ProfilerUI_PanelSetActiveTab *)params;
	ProfilerUI_Panel *panel = data->panel;
	ProfilerUI_Tab *tab = data->tab;
	panel->tab_group.active_tab = tab;
	log_info("Executed command: panel_set_active_tab");
}

// NOTE(hampus): This _always_ put the new child to the right or bottom
PROFILER_UI_COMMAND(panel_split)
{
	// NOTE(hampus): We will create a new parent that will
	// have this panel and a new child as children:
	//
	//               c
	//     a  -->   / \
	//             a   b
	//
	// where c is ´new_parent´, and b is ´child1´
	ProfilerUI_PanelSplit *data  = (ProfilerUI_PanelSplit *)params;
	Axis2 split_axis  = data->axis;
	ProfilerUI_Panel *child0     = data->panel;
	ProfilerUI_Panel *child1     = profiler_ui_panel_alloc(app_state->perm_arena);
	child1->window = child0->window;
	ProfilerUI_Panel *new_parent = profiler_ui_panel_alloc(app_state->perm_arena);
	new_parent->window = child0->window;
	new_parent->pct_of_parent = child0->pct_of_parent;
	new_parent->split_axis = split_axis;
	ProfilerUI_Panel *children[Side_COUNT] = {child0, child1};

	// NOTE(hampus): Hook the new parent as a sibling
	// to the panel's sibling
	if (child0->sibling)
	{
		child0->sibling->sibling = new_parent;
	}
	new_parent->sibling = child0->sibling;

	// NOTE(hampus): Hook the new parent as a child
	// to the panels parent
	if (child0->parent)
	{
		Side side = profiler_ui_get_panel_side(child0);
		child0->parent->children[side] = new_parent;
	}
	new_parent->parent = child0->parent;

	// NOTE(hampus): Make the children siblings
	// and hook them into the new parent
	for (Side side = (Side)0;
		 side < Side_COUNT;
		 side++)
	{
		children[side]->sibling = children[side_flip(side)];
		children[side]->parent = new_parent;
		children[side]->pct_of_parent = 0.5f;
		new_parent->children[side] = children[side];
		memory_zero_array(children[side]->children);
	}

	if (child0 == child0->window->root_panel)
	{
		child0->window->root_panel = new_parent;
	}
	log_info("Executed command: panel_split");
}

PROFILER_UI_COMMAND(panel_split_and_attach)
{
	ProfilerUI_PanelSplitAndAttach *data = (ProfilerUI_PanelSplitAndAttach *)params;

	B32 releasing_on_same_panel =
		data->panel == data->tab->panel &&
		data->panel->tab_group.count == 0;

	ProfilerUI_PanelSplit split_data =
	{
		.panel      = data->panel,
		.axis       = data->axis,
	};

	profiler_ui_command_panel_split(&split_data);

	// NOTE(hampus): panel_split always put the new panel
	// to the left/top
	if (releasing_on_same_panel)
	{
		if (data->panel_side == Side_Max)
		{

			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], ProfilerUI_Panel *);
		}
	}
	else
	{
		if (data->panel_side == Side_Min)
		{
			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], ProfilerUI_Panel *);
		}
	}

#if 0
	if (releasing_on_same_panel)
	{
		// NOTE(hampus): This is needed because if we're releasing
		// on the same panel as we dragged the tab away from, and it
		// was the last tab, we will have to allocate a new tab for the split
		Tab *tab = ui_tab_make(app_state->perm_arena, 0, 0);
		UI_TabAttach attach =
		{
			.tab = tab,
			.panel = data->panel->parent->children[side_flip(data->panel_side)],
			.set_active = true,
		};

		ui_command_tab_attach(&attach);
	}
#endif

	ProfilerUI_Panel *panel = data->panel->parent->children[data->panel_side];

	ProfilerUI_TabAttach tab_attach_data =
	{
		.tab        = data->tab,
		.panel      = panel,
		.set_active = true,
	};

	app_state->next_focused_panel = panel;
	if (data->panel->window != app_state->master_window)
	{
		app_state->next_top_most_window = data->panel->window;
	}

	profiler_ui_command_tab_attach(&tab_attach_data);
	log_info("Executed command: panel_split_and_attach");
}

PROFILER_UI_COMMAND(panel_close)
{
	ProfilerUI_PanelClose *data = (ProfilerUI_PanelClose *)params;
	ProfilerUI_Panel *root = data->panel;
	if (root->parent)
	{
		B32 is_first       = root->parent->children[0] == root;
		ProfilerUI_Panel *replacement = root->sibling;
		if (root->parent->parent)
		{
			if (root == app_state->focused_panel)
			{
				if (root->sibling)
				{
					app_state->next_focused_panel = root->sibling;
				}
				else
				{
					app_state->next_focused_panel = root->parent;
				}
			}
			Side parent_side = profiler_ui_get_panel_side(root->parent);
			Side flipped_parent_side = side_flip(parent_side);

			root->parent->parent->children[parent_side] = replacement;
			root->parent->parent->children[flipped_parent_side]->sibling = replacement;
			replacement->sibling = root->parent->parent->children[flipped_parent_side];
			replacement->pct_of_parent = 1.0f - replacement->sibling->pct_of_parent;
			replacement->parent = root->parent->parent;
		}
		else if (root->parent)
		{
			if (root == app_state->focused_panel)
			{
				app_state->next_focused_panel = root->sibling;
			}
			// NOTE(hampus): We closed one of the root's children
			root->window->root_panel = replacement;
			root->window->root_panel->sibling = 0;
			root->window->root_panel->parent = 0;
		}
	}
	else
	{
		ProfilerUI_Window *window = root->window;
		if (window != app_state->master_window)
		{
			dll_remove(app_state->window_list.first, app_state->window_list.last, window);
		}
	}
	log_info("Executed command: panel_close");
}

PROFILER_UI_COMMAND(window_push_to_front)
{
	ProfilerUI_WindowPushToFront *data = params;
	ProfilerUI_Window *window = data->window;
	profiler_ui_window_push_to_front(window);
	log_info("Executed command: window_push_to_front");
}

PROFILER_UI_COMMAND(window_remove_from_list)
{
	ProfilerUI_WindowRemoveFromList *data = params;
	ProfilerUI_Window *window = data->window;
	profiler_ui_window_remove_from_list(window);
	log_info("Executed command: window_remove_from_list");
}
