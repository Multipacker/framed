UI_COMMAND(panel_close);

////////////////////////////////
//~ hampus: Tab commands

UI_COMMAND(tab_close)
{
	UI_TabDelete *data = (UI_TabDelete *)params;

	UI_Tab *tab = data->tab;
	UI_Panel *panel = tab->panel;
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
	panel->tab_group.overflow -= tab->box->fixed_size.x;
	if (panel->tab_group.count == 0)
	{
		UI_PanelClose close =
		{
			.panel = panel,
		};
		ui_command_panel_close(&close);
	}
}

UI_COMMAND(tab_attach)
{
	UI_TabAttach *data = (UI_TabAttach *)params;

	UI_Panel *panel = data->panel;
	UI_Tab *tab = data->tab;
	dll_push_back(panel->tab_group.first, panel->tab_group.last, tab);
	tab->panel = panel;
	B32 set_active = panel->tab_group.count == 0 || data->set_active;
	if (set_active)
	{
		panel->tab_group.active_tab = tab;
	}
	panel->tab_group.count++;
}

////////////////////////////////
//~ hampus: UI_Panel commands

UI_COMMAND(panel_set_active_tab)
{
	UI_PanelSetActiveTab *data = (UI_PanelSetActiveTab *)params;
	UI_Panel *panel = data->panel;
	UI_Tab *tab = data->tab;
	panel->tab_group.active_tab = tab;
}

// NOTE(hampus): This _always_ put the new child to the right or bottom
UI_COMMAND(panel_split)
{
	// NOTE(hampus): We will create a new parent that will
	// have this panel and a new child as children:
	//
	//               c
	//     a  -->   / \
	//             a   b
	//
	// where c is ´new_parent´, and b is ´child1´
	UI_PanelSplit *data  = (UI_PanelSplit *)params;
	Axis2 split_axis  = data->axis;
	UI_Panel *child0     = data->panel;
	UI_Panel *child1     = ui_panel_alloc(app_state->perm_arena);
	child1->window = child0->window;
	UI_Panel *new_parent = ui_panel_alloc(app_state->perm_arena);
	new_parent->window = child0->window;
	new_parent->pct_of_parent = child0->pct_of_parent;
	new_parent->split_axis = split_axis;
	UI_Panel *children[Side_COUNT] = {child0, child1};

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
		Side side = ui_get_panel_side(child0);
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

	if (data->alloc_new_tab)
	{
		UI_TabViewInfo view_info = data->tab_view_info;
		UI_Tab *tab = ui_tab_make(app_state->perm_arena, view_info.function, view_info.data);
		UI_TabAttach attach =
		{
			.tab = tab,
			.panel = child1,
			.set_active = true,
		};
		ui_command_tab_attach(&attach);
	}
}

UI_COMMAND(panel_split_and_attach)
{
	UI_PanelSplitAndAttach *data = (UI_PanelSplitAndAttach *)params;

	B32 releasing_on_same_panel =
		data->panel == data->tab->panel &&
		data->panel->tab_group.count == 0;

	UI_PanelSplit split_data =
	{
		.panel      = data->panel,
		.axis       = data->axis,
	};

	ui_command_panel_split(&split_data);

	// NOTE(hampus): panel_split always put the new panel
	// to the left/top
	if (releasing_on_same_panel)
	{
		if (data->panel_side == Side_Max)
		{
			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], UI_Panel *);
		}
	}
	else
	{
		if (data->panel_side == Side_Min)
		{
			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], UI_Panel *);
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

	UI_Panel *panel = data->panel->parent->children[data->panel_side];

	UI_TabAttach tab_attach_data =
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

	ui_command_tab_attach(&tab_attach_data);
}

UI_COMMAND(panel_close)
{
	UI_PanelClose *data = (UI_PanelClose *)params;
	UI_Panel *root = data->panel;
	if (root->parent)
	{
		B32 is_first       = root->parent->children[0] == root;
		UI_Panel *replacement = root->sibling;
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
			Side parent_side = ui_get_panel_side(root->parent);
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
		UI_Window *window = root->window;
		if (window != app_state->master_window)
		{
			dll_remove(app_state->window_list.first, app_state->window_list.last, window);
		}
	}
}

UI_COMMAND(window_push_to_front)
{
	UI_WindowPushToFront *data = params;
	UI_Window *window = data->window;
	ui_window_push_to_front(window);
}

UI_COMMAND(window_remove_from_list)
{
	UI_WindowRemoveFromList *data = params;
	UI_Window *window = data->window;
	ui_window_remove_from_list(window);
}