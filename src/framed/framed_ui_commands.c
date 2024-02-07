PROFILER_UI_COMMAND(panel_close);

////////////////////////////////
//~ hampus: Tab commands

PROFILER_UI_COMMAND(tab_close)
{
	FramedUI_TabDelete *data = (FramedUI_TabDelete *) params;

	FramedUI_Tab *tab = data->tab;
	FramedUI_Panel *panel = tab->panel;
	if (tab == panel->tab_group.active_tab)
	{
		if (!framed_ui_tab_is_nil(tab->prev))
		{
			panel->tab_group.active_tab = tab->prev;
		}
		else if (!framed_ui_tab_is_nil(tab->next))
		{
			panel->tab_group.active_tab = tab->next;
		}
		else
		{
			panel->tab_group.active_tab = &g_nil_tab;
		}
	}
	dll_remove_npz(panel->tab_group.first, panel->tab_group.last, tab, next, prev, &g_nil_tab);
	panel->tab_group.count--;
	tab->next = tab->prev = &g_nil_tab;
	if (panel->tab_group.count == 0)
	{
		FramedUI_PanelClose close =
		{
			.panel = panel,
		};
		framed_ui_command_panel_close(&close);
	}
	log_info("Executed command: tab_close (%"PRISTR8")", str8_expand(tab->string));
}

PROFILER_UI_COMMAND(tab_attach)
{
	FramedUI_TabAttach *data = (FramedUI_TabAttach *) params;

	FramedUI_Panel *panel = data->panel;
	FramedUI_Tab *tab = data->tab;
	dll_push_back_npz(panel->tab_group.first, panel->tab_group.last, tab, next, prev, &g_nil_tab);
	tab->panel = panel;
	B32 set_active = panel->tab_group.count == 0 || data->set_active;
	if (set_active)
	{
		panel->tab_group.active_tab = tab;
	}
	panel->tab_group.count++;
	framed_ui_state->focused_panel = panel;
	log_info("Executed command: tab_attach (%"PRISTR8""" -> %"PRISTR8")", str8_expand(tab->string), str8_expand(panel->string));
}

PROFILER_UI_COMMAND(tab_swap)
{
	FramedUI_TabSwap *data = (FramedUI_TabSwap *) params;
	FramedUI_Panel *panel = data->tab0->panel;

	FramedUI_Tab *tab0 = data->tab0;
	FramedUI_Tab *tab1 = data->tab1;

	if (tab0 == panel->tab_group.first)
	{
		panel->tab_group.first = tab1;
	}
	else if (tab1 == panel->tab_group.first)
	{
		panel->tab_group.first = tab0;
	}

	if (tab0 == panel->tab_group.last)
	{
		panel->tab_group.last = tab1;
	}
	else if (tab1 == panel->tab_group.last)
	{
		panel->tab_group.last = tab0;
	}

	FramedUI_Tab *tab0_prev = &g_nil_tab;
	FramedUI_Tab *tab0_next = &g_nil_tab;

	FramedUI_Tab *tab1_prev = &g_nil_tab;
	FramedUI_Tab *tab1_next = &g_nil_tab;

	tab0_prev = tab1->prev;
	if (tab0->prev == tab1)
	{
		tab0_next = tab1;
	}
	else
	{
		tab0_next = tab1->next;
	}

	tab1_prev = tab0->prev;
	if (tab1->prev == tab0)
	{
		tab1_next = tab0;
	}
	else
	{
		tab1_next = tab0->next;
	}

	tab0->next = tab0_next;
	tab0->prev = tab0_prev;

	tab1->next = tab1_next;
	tab1->prev = tab1_prev;

	if (!framed_ui_tab_is_nil(tab0->next))
	{
		tab0->next->prev = tab0;
	}

	if (!framed_ui_tab_is_nil(tab1->next))
	{
		tab1->next->prev = tab1;
	}

	if (!framed_ui_tab_is_nil(tab0->prev))
	{
		tab0->prev->next = tab0;
	}

	if (!framed_ui_tab_is_nil(tab1->prev))
	{
		tab1->prev->next = tab1;
	}

	log_info("Executed command: tab_reorder");
}

////////////////////////////////
//~ hampus: UI_Panel commands

PROFILER_UI_COMMAND(panel_set_active_tab)
{
	FramedUI_PanelSetActiveTab *data = (FramedUI_PanelSetActiveTab *) params;
	FramedUI_Panel *panel = data->panel;
	FramedUI_Tab *tab = data->tab;
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
	FramedUI_PanelSplit *data  = (FramedUI_PanelSplit *) params;
	Axis2 split_axis  = data->axis;
	FramedUI_Panel *child0     = data->panel;
	FramedUI_Panel *child1     = framed_ui_panel_alloc(framed_ui_state->perm_arena);
	child1->window = child0->window;
	FramedUI_Panel *new_parent = framed_ui_panel_alloc(framed_ui_state->perm_arena);
	new_parent->window = child0->window;
	new_parent->pct_of_parent = child0->pct_of_parent;
	new_parent->split_axis = split_axis;
	FramedUI_Panel *children[Side_COUNT] = { child0, child1 };

	// NOTE(hampus): Hook the new parent as a sibling
	// to the panel's sibling
	if (!framed_ui_panel_is_nil(child0->sibling))
	{
		child0->sibling->sibling = new_parent;
	}
	new_parent->sibling = child0->sibling;

	// NOTE(hampus): Hook the new parent as a child
	// to the panels parent
	if (!framed_ui_panel_is_nil(child0->parent))
	{
		Side side = framed_ui_get_panel_side(child0);
		child0->parent->children[side] = new_parent;
	}
	new_parent->parent = child0->parent;

	// NOTE(hampus): Make the children siblings
	// and hook them into the new parent
	for (Side side = (Side) 0; side < Side_COUNT; side++)
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
	FramedUI_PanelSplitAndAttach *data = (FramedUI_PanelSplitAndAttach *) params;

	B32 releasing_on_same_panel =
		data->panel == data->tab->panel &&
		data->panel->tab_group.count == 0;

	FramedUI_PanelSplit split_data =
	{
		.panel      = data->panel,
		.axis       = data->axis,
	};

	framed_ui_command_panel_split(&split_data);

	// NOTE(hampus): panel_split always put the new panel
	// to the left/top
	if (releasing_on_same_panel)
	{
		if (data->panel_side == Side_Max)
		{
			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], FramedUI_Panel *);
		}
	}
	else
	{
		if (data->panel_side == Side_Min)
		{
			swap(data->panel->parent->children[Side_Min], data->panel->parent->children[Side_Max], FramedUI_Panel *);
		}
	}

#if 0
	if (releasing_on_same_panel)
	{
		// NOTE(hampus): This is needed because if we're releasing
		// on the same panel as we dragged the tab away from, and it
		// was the last tab, we will have to allocate a new tab for the split
		Tab *tab = ui_tab_make(framed_ui_state->perm_arena, 0, 0);
		UI_TabAttach attach =
		{
			.tab = tab,
			.panel = data->panel->parent->children[side_flip(data->panel_side)],
			.set_active = true,
		};

		ui_command_tab_attach(&attach);
	}
#endif

	FramedUI_Panel *panel = data->panel->parent->children[data->panel_side];

	FramedUI_TabAttach tab_attach_data =
	{
		.tab        = data->tab,
		.panel      = panel,
		.set_active = true,
	};

	framed_ui_state->next_focused_panel = panel;
	if (data->panel->window != framed_ui_state->master_window)
	{
		framed_ui_state->next_top_most_window = data->panel->window;
	}

	framed_ui_command_tab_attach(&tab_attach_data);
	log_info("Executed command: panel_split_and_attach");
}

PROFILER_UI_COMMAND(panel_close)
{
	FramedUI_PanelClose *data = (FramedUI_PanelClose *) params;
	FramedUI_Panel *root = data->panel;
	if (!framed_ui_panel_is_nil(root->parent))
	{
		B32 is_first       = root->parent->children[0] == root;
		FramedUI_Panel *replacement = root->sibling;
		if (!framed_ui_panel_is_nil(root->parent->parent))
		{
			if (root == framed_ui_state->focused_panel)
			{
				if (!framed_ui_panel_is_nil(root->sibling))
				{
					framed_ui_state->next_focused_panel = root->sibling;
				}
				else
				{
					framed_ui_state->next_focused_panel = root->parent;
				}
			}
			Side parent_side = framed_ui_get_panel_side(root->parent);
			Side flipped_parent_side = side_flip(parent_side);

			root->parent->parent->children[parent_side] = replacement;
			root->parent->parent->children[flipped_parent_side]->sibling = replacement;
			replacement->sibling = root->parent->parent->children[flipped_parent_side];
			replacement->pct_of_parent = 1.0f - replacement->sibling->pct_of_parent;
			replacement->parent = root->parent->parent;
		}
		else if (!framed_ui_panel_is_nil(root->parent))
		{
			if (root == framed_ui_state->focused_panel)
			{
				framed_ui_state->next_focused_panel = root->sibling;
			}
			// NOTE(hampus): We closed one of the root's children
			root->window->root_panel = replacement;
			root->window->root_panel->sibling = &g_nil_panel;
			root->window->root_panel->parent = &g_nil_panel;
		}
	}
	else
	{
		FramedUI_Window *window = root->window;
		if (window != framed_ui_state->master_window)
		{
			dll_remove(framed_ui_state->window_list.first, framed_ui_state->window_list.last, window);
		}
	}
	log_info("Executed command: panel_close");
}

PROFILER_UI_COMMAND(window_push_to_front)
{
	FramedUI_WindowPushToFront *data = params;
	FramedUI_Window *window = data->window;
	framed_ui_window_push_to_front(window);
	log_info("Executed command: window_push_to_front");
}

PROFILER_UI_COMMAND(window_remove_from_list)
{
	FramedUI_WindowRemoveFromList *data = params;
	FramedUI_Window *window = data->window;
	framed_ui_window_remove_from_list(window);
	log_info("Executed command: window_remove_from_list");
}
