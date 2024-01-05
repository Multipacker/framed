UI_CMD(panel_close);

UI_CMD(tab_delete)
{
	TabDelete *data = (TabDelete *)params;

	Tab *tab = data->tab;
	Panel *panel = tab->panel;
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
	tab->next = 0;
	tab->prev = 0;
	tab->panel = 0;
	if (panel->tab_group.first == 0)
	{
		PanelClose close = 
		{
			.panel = panel,
		};
		panel_close(&close);
	}
}

UI_CMD(tab_attach)
{
	TabAttach *data = (TabAttach *)params;

	Panel *panel = data->panel;
	Tab *tab = data->tab;
	dll_push_back(panel->tab_group.first, panel->tab_group.last, tab);
	tab->panel = panel;
	if (data->set_active)
	{
		panel->tab_group.active_tab = tab;
	}
}

UI_CMD(panel_set_active_tab)
{
	PanelSetActiveTab *data = (PanelSetActiveTab *)params;
	Panel *panel = data->panel;
	Tab *tab = data->tab;
	panel->tab_group.active_tab = tab;
}

UI_CMD(panel_split)
{
	// NOTE(hampus): We will create a new parent that will
	// have this panel and a new child as children

	PanelSplit *data  = (PanelSplit *)params;
	Axis2 split_axis  = data->axis;
	Panel *child0     = data->panel;
	Panel *child1     = ui_panel_alloc(app_state->perm_arena);
	Panel *new_parent = ui_panel_alloc(app_state->perm_arena);
	new_parent->percent_of_parent = child0->percent_of_parent;
	new_parent->split_axis = split_axis;
	Panel *children[Side_COUNT] = {child0, child1};

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
		children[side]->percent_of_parent = 0.5f;
		new_parent->children[side] = children[side];
		memory_zero_array(children[side]->children);
	}
	
	if (child0 == app_state->root_panel)
	{
		app_state->root_panel = new_parent;
	}
	
	Tab *tab = ui_tab_alloc(app_state->perm_arena);
	TabAttach attach =
	{
		.tab = tab,
		.panel = child1,
		.set_active = true,
	};
	tab_attach(&attach);
}

UI_CMD(panel_split_and_attach)
{
	PanelSplitAndAttach *data = (PanelSplitAndAttach *)params;

	PanelSplit split_data =
	{
		.panel      = data->panel,
		.panel_side = data->panel_side,
		.axis       = data->axis,
	};

	panel_split(&split_data);
	
	if (data->panel_side == Side_Min)
	{
		swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], Panel *);
	}
	
	Panel *panel = data->panel->parent->children[data->panel_side];

	TabAttach tab_attach_data =
	{
		.tab        = data->tab,
		.panel      = panel,
		.set_active = true,
	};

	tab_attach(&tab_attach_data);
}

UI_CMD(panel_close)
{
	PanelClose *data = (PanelClose *)params;
	Panel *root = data->panel;
	if (root->parent)
	{
		B32 is_first       = root->parent->children[0] == root;
		Panel *replacement = root->sibling;
		if (root->parent->parent)
		{
			Side parent_side = ui_get_panel_side(root->parent);
			Side flipped_parent_side = side_flip(parent_side);

			root->parent->parent->children[parent_side] = replacement;
			root->parent->parent->children[flipped_parent_side]->sibling = replacement;
			replacement->sibling           = root->parent->parent->children[flipped_parent_side];
			replacement->percent_of_parent = 1.0f - replacement->sibling->percent_of_parent;
			replacement->parent            = root->parent->parent;
		}
		else if (root->parent)
		{
			// NOTE(hampus): We closed one of the root's children
			app_state->root_panel = replacement;
			app_state->root_panel->sibling = 0;
			app_state->root_panel->parent = 0;
		}
	}
	else
	{
		// NOTE(hampus): We tried to remove the root, big nono
	}
}