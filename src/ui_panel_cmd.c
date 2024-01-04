			   UI_CMD(tab_delete)
{
	TabDelete *data = (TabDelete *)cmd->data;
	
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
}

UI_CMD(tab_attach)
{
	TabAttach *data = (TabAttach *)cmd->data;
	
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
	PanelSetActiveTab *data = (PanelSetActiveTab *)cmd->data;
	Panel *panel = data->panel;
	Tab *tab = data->tab;
	panel->tab_group.active_tab = tab;
}

UI_CMD(panel_split)
{
	PanelSplit *data = (PanelSplit *)cmd->data;
	
	Panel *first     = data->panel;
	Axis2 split_axis = data->axis;
	
	Panel *new_parent = ui_panel_alloc(app_state->perm_arena);
	Panel *second     = ui_panel_alloc(app_state->perm_arena);
	
	Tab *tab = ui_tab_alloc(app_state->perm_arena);
	ui_attach_tab_to_panel(second, tab, 1);
	
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
	
}
UI_CMD(panel_close)
{
	PanelClose *data = (PanelClose *)cmd->data;
	Panel *root = data->panel;
	if (root->parent)
	{
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
	}
	}