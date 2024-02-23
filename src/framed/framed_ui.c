////////////////////////////////
// hampus: Short term
//
// [ ] @bug The user can drop a panel on the menu bar which will hide the tab bar
// [ ] @polish Dragging by a tab bar should not offset the new window at (0, 0)
// [ ] @polish Memory leak when closing/opening a window due to new view data is
//             is pushed every time the window opens again
// [ ] @polish Simplify memory management of panels, tabs & windows

////////////////////////////////
// hampus: Medium term
//
// [ ] @code @feature UI startup builder
// [ ] @polish Resizing panels with tab animation doesn't look that good right now.
// [ ] @feature Tab reordering again
// [ ] @polish Tab dragging new window offset should be relative to where you began to drag the tab

////////////////////////////////
// hampus: Long term
//
// [ ] @feature Move around windows that have multiple panels
// [ ] @feature Be able to pin windows which disables closing
// [ ] @feature Scroll tabs horizontally if there are too many to fit
//                 - Partially fixed. You can navigate tabs by pressing the arrows to the right

////////////////////////////////
//~ hampus: Globals

extern FramedUI_Tab g_nil_tab;

global FramedUI_State *framed_ui_state;

read_only FramedUI_Panel g_nil_panel =
{
    {&g_nil_panel, &g_nil_panel},
    &g_nil_panel,
    &g_nil_panel,
    {&g_nil_tab, &g_nil_tab, &g_nil_tab},
};

read_only FramedUI_Tab g_nil_tab =
{
    &g_nil_tab,
    &g_nil_tab,
    &g_nil_panel,
};

////////////////////////////////
//~ hampus: Settings

internal U32
framed_ui_font_size_from_scale(FramedUI_FontScale scale)
{
    U32 result = 0;

    switch(scale)
    {
        case FramedUI_FontScale_Smaller:
        {
            result = (U32) f32_round((F32) framed_ui_state->settings.font_size * 0.75f);
        } break;
        case FramedUI_FontScale_Normal:
        {
            result = framed_ui_state->settings.font_size;
        } break;
        case FramedUI_FontScale_Larger:
        {
            result = (U32) f32_round((F32) framed_ui_state->settings.font_size * 1.25f);
        } break;
        invalid_case;
    }

    return(result);
}

internal Vec4F32
framed_ui_color_from_theme(FramedUI_Color color)
{
    Vec4F32 result = framed_ui_state->settings.theme_colors[color];
    return(result);
}

internal Void
framed_ui_set_color(FramedUI_Color color, Vec4F32 value)
{
    framed_ui_state->settings.theme_colors[color] = value;
}

internal Str8
framed_ui_string_from_color(FramedUI_Color color)
{
    Str8 result = str8_lit("");
    switch (color)
    {
        case FramedUI_Color_Panel: { result = str8_lit("Panel background"); } break;
        case FramedUI_Color_PanelBorderInactive: { result = str8_lit("Inactive panel border"); } break;
        case FramedUI_Color_PanelBorderActive: { result = str8_lit("Active panel border"); } break;
        case FramedUI_Color_PanelOverlayInactive: { result = str8_lit("Inactive panel overlay"); } break;
        case FramedUI_Color_TabBar: { result = str8_lit("Tab bar background"); } break;
        case FramedUI_Color_TabActive: { result = str8_lit("Active tab background"); } break;
        case FramedUI_Color_TabInactive: { result = str8_lit("Inactive tab background"); } break;
        case FramedUI_Color_TabTitle: { result = str8_lit("Tab foreground"); } break;
        case FramedUI_Color_TabBorder: { result = str8_lit("Tab border"); } break;
        case FramedUI_Color_TabBarButtons: { result = str8_lit("Tab bar buttons background"); } break;
        invalid_case;
    }
    return(result);
}

////////////////////////////////
//~ hampus: Command

internal Void
framed_ui_command_push(FramedUI_CommandKind kind, FramedUI_CommandParams params)
{
    FramedUI_CommandNode *node = push_struct(framed_ui_state->frame_arena, FramedUI_CommandNode);
    node->command.kind = kind;
    node->command.params = params;
    dll_push_back(framed_ui_state->cmd_list.first, framed_ui_state->cmd_list.last, node);
}

////////////////////////////////
//~ hampus: Tab dragging

internal Void
framed_ui_drag_begin_reordering(FramedUI_Tab *tab)
{
    if (!framed_ui_tab_has_flag(tab, FramedUI_TabFlag_Pinned))
    {
        FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
        drag_data->tab = tab;
        framed_ui_state->drag_data.drag_origin = ui_mouse_pos();
        framed_ui_state->drag_status = FramedUI_DragStatus_Reordering;
        // NOTE(hampus): Null the active key so the split previews
        // can take input. This is needed because else the box will
        // persist as the active & hot box
        // TODO(hampus): Figure out some other way to do this.
        ui_ctx->active_key = ui_key_null();
        log_info("Drag: reordering");
    }
}

internal Void
framed_ui_wait_for_drag_threshold(Void)
{
    framed_ui_state->drag_status = FramedUI_DragStatus_WaitingForDragThreshold;
    log_info("Drag: waiting for drag threshold");
}

internal Void
framed_ui_drag_release(Void)
{
    framed_ui_state->drag_status = FramedUI_DragStatus_Released;
    log_info("Drag: released");
}

internal Void
framed_ui_drag_end(Void)
{
    framed_ui_state->drag_status = FramedUI_DragStatus_Inactive;
    memory_zero_struct(&framed_ui_state->drag_data);
    log_info("Drag: end");
}

internal B32
framed_ui_is_dragging(Void)
{
    B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Dragging;
    return(result);
}

internal B32
framed_ui_is_tab_reordering(Void)
{
    B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Reordering;
    return(result);
}

internal B32
framed_ui_is_waiting_for_drag_threshold(Void)
{
    B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_WaitingForDragThreshold;
    return(result);
}

internal B32
framed_ui_drag_is_inactive(Void)
{
    B32 result = framed_ui_state->drag_status == FramedUI_DragStatus_Inactive;
    return(result);
}

////////////////////////////////
//~ hampus: Tab

internal B32
framed_ui_tab_has_flag(FramedUI_Tab *tab, FramedUI_TabFlag flag)
{
    B32 result = (tab->flags & flag) != 0;
    return(result);
}

internal B32
framed_ui_tab_is_nil(FramedUI_Tab *tab)
{
    B32 result = (!tab || tab == &g_nil_tab);
    return(result);
}

internal FramedUI_Tab *
framed_ui_tab_alloc(Void)
{
    FramedUI_Tab *result = (FramedUI_Tab *) framed_ui_state->first_free_tab;
    if (framed_ui_tab_is_nil(result))
    {
        result = push_struct(framed_ui_state->perm_arena, FramedUI_Tab);
        log_info("Allocated tab: %p", result);
    }
    else
    {
        ASAN_UNPOISON_MEMORY_REGION(result, sizeof(FramedUI_Tab));
        framed_ui_state->first_free_tab = framed_ui_state->first_free_tab->next;
    }
    memory_zero_struct(result);
    result->next = result->prev = &g_nil_tab;
    result->panel = &g_nil_panel;
    return(result);
}

internal Void
framed_ui_tab_free(FramedUI_Tab *tab)
{
    arena_destroy(tab->view_info.arena);
    arena_destroy(tab->arena);
    assert(!framed_ui_tab_is_nil(tab));
    FramedUI_FreeTab *free_tab = (FramedUI_FreeTab *) tab;
    free_tab->next = framed_ui_state->first_free_tab;
    framed_ui_state->first_free_tab = free_tab;
    ASAN_POISON_MEMORY_REGION(tab, sizeof(FramedUI_Tab));
}

internal Void
framed_ui_tab_equip_view_info(FramedUI_Tab *tab, FramedUI_TabViewInfo view_info)
{
    tab->view_info.function = view_info.function;
    tab->view_info.data = view_info.data;
    arena_pop_to(tab->view_info.arena, 0);
}

internal FramedUI_Tab *
framed_ui_tab_make(FramedUI_TabViewProc *function, Void *data, Str8 display_string)
{
    // TODO(hampus): Check for name collision with other tabs

    //- hampus: Allocate tab

    FramedUI_Tab *result = framed_ui_tab_alloc();
    result->arena = arena_create("%"PRISTR8, str8_expand(display_string));
    arena_scratch(0, 0)
    {
        Str8List string_list = {0};
        str8_list_push(scratch, &string_list, display_string);
        str8_list_push(scratch, &string_list, str8_lit("ViewInfo"));
        Str8 view_info_arena_string = str8_join(result->arena, &string_list);
        result->view_info.arena = arena_create("%"PRISTR8, str8_expand(view_info_arena_string));
    }

    //- hampus: Equip with display string

    if (!display_string.size)
    {
        // NOTE(hampus): We probably won't do this in the future because
        // you won't probably be able to have unnamed tabs.
        result->display_string = str8_pushf(result->arena, "Tab%"PRIS32, framed_ui_state->num_tabs);
    }
    else
    {
        result->display_string = str8_copy(result->arena, display_string);
    }


    //- hampus: Equip with view info

    result->view_info.function = framed_ui_tab_view_default;
    result->view_info.data = result;

    if (function)
    {
        FramedUI_TabViewInfo view_info = {function, data};
        framed_ui_tab_equip_view_info(result, view_info);
    }

    framed_ui_state->num_tabs++;
    return(result);
}

internal B32
framed_ui_tab_is_active(FramedUI_Tab *tab)
{
    B32 result = tab->panel->tab_group.active_tab == tab;
    return(result);
}

internal B32
framed_ui_tab_is_dragged(FramedUI_Tab *tab)
{
    B32 result = false;
    if (framed_ui_state->drag_status == FramedUI_DragStatus_Dragging)
    {
        if (framed_ui_state->drag_data.tab == tab)
        {
            result = true;
        }
    }
    return(result);
}

internal UI_Box *
framed_ui_tab_button(FramedUI_Tab *tab)
{
    ui_push_string(tab->display_string);

    B32 active = framed_ui_tab_is_active(tab);

    F32 height_em = 1.2f;
    F32 corner_radius = (F32) ui_top_font_line_height() * 0.2f;

    Vec4F32 color = active ?
        framed_ui_color_from_theme(FramedUI_Color_TabActive) :
    framed_ui_color_from_theme(FramedUI_Color_TabInactive);
    ui_next_color(color);
    ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
    ui_next_vert_corner_radius(corner_radius, 0);
    ui_next_child_layout_axis(Axis2_X);
    ui_next_width(ui_children_sum(1));
    ui_next_height(ui_em(height_em, 1));
    ui_next_hover_cursor(Gfx_Cursor_Hand);
    UI_Box *title_container = ui_box_make(UI_BoxFlag_DrawBackground |
                                          UI_BoxFlag_HotAnimation |
                                          UI_BoxFlag_ActiveAnimation |
                                          UI_BoxFlag_Clickable  |
                                          UI_BoxFlag_DrawBorder |
                                          UI_BoxFlag_AnimateY,
                                          str8_lit("TitleContainer"));

    B32 pinned = framed_ui_tab_has_flag(tab, FramedUI_TabFlag_Pinned);

    ui_parent(title_container)
    {
        ui_next_height(ui_pct(1, 1));
        ui_next_width(ui_em(1, 1));
        ui_next_icon(RENDER_ICON_PIN);
        ui_next_hover_cursor(Gfx_Cursor_Hand);
        ui_next_corner_radies(corner_radius, 0, 0, 0);
        ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
        ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabInactive));
        UI_BoxFlags pin_box_flags = UI_BoxFlag_Clickable;
        if (pinned)
        {
            pin_box_flags |= UI_BoxFlag_DrawText;
        }
        UI_Box *pin_box = ui_box_make(pin_box_flags, str8_lit("PinButton"));

        ui_next_text_color(framed_ui_color_from_theme(FramedUI_Color_TabTitle));
        UI_Box *title = ui_box_make(UI_BoxFlag_DrawText, tab->display_string);

        ui_box_equip_display_string(title, tab->display_string);

        ui_next_height(ui_pct(1, 1));
        ui_next_width(ui_em(1, 1));
        ui_next_icon(RENDER_ICON_CROSS);
        ui_next_hover_cursor(Gfx_Cursor_Hand);
        ui_next_corner_radies(0, corner_radius, 0, 0);
        ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_TabBorder));
        ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabInactive));
        UI_Box *close_box = ui_box_make(UI_BoxFlag_Clickable, str8_lit("CloseButton"));

        // TODO(hampus): We shouldn't need to do this here
        // since there shouldn't even be any input events
        // left in the queue if dragging is ocurring.
        if (framed_ui_is_tab_reordering())
        {
            FramedUI_Tab *drag_tab = framed_ui_state->drag_data.tab;
            if (tab != drag_tab)
            {
                Vec2F32 mouse_pos = ui_mouse_pos();
                F32 center = (title_container->parent->fixed_rect.x0 +  title_container->parent->fixed_rect.x1) / 2;
                B32 hovered = mouse_pos.x >= center - ui_top_font_line_height() * 0.2f && mouse_pos.x <= center + ui_top_font_line_height() * 0.2f;
                if (hovered)
                {
                    // framed_ui_swap_tabs(framed_ui_state->drag_data.tab, tab);
                }
            }
        }

        if (framed_ui_drag_is_inactive())
        {
            UI_Comm pin_box_comm = ui_comm_from_box(pin_box);
            UI_Comm close_box_comm = {0};
            if (!pinned)
            {
                close_box_comm = ui_comm_from_box(close_box);
            }

            UI_Comm title_comm = ui_comm_from_box(title_container);
            if (title_comm.pressed)
            {
                FramedUI_CommandParams params = {0};
                params.tab = tab;
                framed_ui_command_push(FramedUI_CommandKind_SetTabActive, params);
            }

            if (title_comm.dragging && !title_comm.hovering)
            {
                framed_ui_drag_begin_reordering(tab);
            }

            // NOTE(hampus): Icon appearance
            UI_BoxFlags icon_hover_flags = UI_BoxFlag_DrawBackground | UI_BoxFlag_HotAnimation | UI_BoxFlag_ActiveAnimation | UI_BoxFlag_DrawText;

            if (pin_box_comm.hovering)
            {
                pin_box->flags |= icon_hover_flags;
            }

            if (close_box_comm.hovering)
            {
                if (!pinned)
                {
                    close_box->flags |= icon_hover_flags;
                }
            }

            if (close_box_comm.hovering || pin_box_comm.hovering || title_comm.hovering)
            {
                pin_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
                if (!pinned)
                {
                    close_box->flags |= UI_BoxFlag_DrawText | UI_BoxFlag_DrawBorder;
                }
            }

            if (pin_box_comm.pressed)
            {
                tab->flags ^= FramedUI_TabFlag_Pinned;
            }

            if (close_box_comm.pressed && !pinned)
            {
                tab->flags |= FramedUI_TabFlag_MarkedForDeletion;
            }
        }
    }
    ui_pop_string();
    return(title_container);
}

////////////////////////////////
//~ hampus: Panel

internal Void
framed_ui_panel_set_active_tab(FramedUI_Panel *panel, FramedUI_Tab *tab)
{
    panel->tab_group.active_tab = tab;
}

internal B32
framed_ui_panel_has_flag(FramedUI_Panel *panel, FramedUI_PanelFlag flag)
{
    B32 result = (panel->flags & flag) != 0;
    return(result);
}

internal Void
framed_ui_panel_insert_tab(FramedUI_Panel *panel, FramedUI_Tab *tab)
{
    dll_push_back_npz(panel->tab_group.first, panel->tab_group.last, tab, next, prev, &g_nil_tab);
    tab->panel = panel;
    if (panel->tab_group.count == 0)
    {
        panel->tab_group.active_tab = tab;
    }
    panel->tab_group.count++;
    framed_ui_state->focused_panel = panel;
}

internal Void
framed_ui_panel_remove_tab(FramedUI_Tab *tab)
{
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
        framed_ui_panel_close(panel);
    }
}

internal Void
framed_ui_panel_split(FramedUI_Panel *panel, Axis2 axis)
{
    // NOTE(hampus): We will create a new parent that will
    // have this panel and a new child as children:
    //
    //               c
    //     a  -->   / \
    //             a   b
    //
    // where c is ´new_parent´, and b is ´child1´
    Axis2 split_axis  = axis;
    FramedUI_Panel *child0     = panel;
    FramedUI_Panel *child1     = framed_ui_panel_make();
    child1->window = child0->window;
    FramedUI_Panel *new_parent = framed_ui_panel_make();
    new_parent->window = child0->window;
    new_parent->pct_of_parent = child0->pct_of_parent;
    new_parent->split_axis = split_axis;
    FramedUI_Panel *children[Side_COUNT] = {child0, child1};

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
        Side side = framed_ui_panel_get_side(child0);
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
}

internal Void
framed_ui_panel_close(FramedUI_Panel *panel)
{
    FramedUI_Panel *root = panel;
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
            Side parent_side = framed_ui_panel_get_side(root->parent);
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
        framed_ui_panel_free(panel);
    }
    else
    {
        FramedUI_Window *window = root->window;
        if (window != framed_ui_state->master_window)
        {
            framed_ui_panel_free(panel);
            dll_remove(framed_ui_state->open_windows.first, framed_ui_state->open_windows.last, window);
        }
    }
}

internal B32
framed_ui_panel_is_nil(FramedUI_Panel *panel)
{
    B32 result = (!panel || panel == &g_nil_panel);
    return(result);
}

internal FramedUI_Panel *
framed_ui_panel_alloc(Void)
{
    FramedUI_Panel *result = (FramedUI_Panel *) framed_ui_state->first_free_panel;
    if (framed_ui_panel_is_nil(result))
    {
        result = push_struct(framed_ui_state->perm_arena, FramedUI_Panel);
        log_info("Allocated panel: %p", result);
    }
    else
    {
        ASAN_UNPOISON_MEMORY_REGION(result, sizeof(FramedUI_Panel));
        framed_ui_state->first_free_panel = framed_ui_state->first_free_panel->next;
    }
    memory_zero_struct(result);
    result->children[Side_Min] = result->children[Side_Max] = result->sibling = result->parent = &g_nil_panel;
    result->tab_group.first = result->tab_group.active_tab = result->tab_group.last = &g_nil_tab;
    return(result);
}

internal Void
framed_ui_panel_free(FramedUI_Panel *panel)
{
    assert(!framed_ui_panel_is_nil(panel));
    FramedUI_FreePanel *free_panel = (FramedUI_FreePanel *) panel;
    free_panel->next = framed_ui_state->first_free_panel;
    framed_ui_state->first_free_panel = free_panel;
    ASAN_POISON_MEMORY_REGION(panel, sizeof(FramedUI_Panel));
}

internal Void
framed_ui_panel_free_recursively(FramedUI_Panel *root)
{
    if (!framed_ui_panel_is_nil(root))
    {
        for (Side side = (Side) 0; side < Side_COUNT; ++side)
        {
            FramedUI_Panel *child = root->children[side];
            framed_ui_panel_free_recursively(child);
        }
        FramedUI_Tab *next_tab = &g_nil_tab; // NOTE(hampus): Needed because framed_ui_tab_free will poison the tab
        for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = next_tab)
        {
            next_tab = tab->next;
            framed_ui_tab_free(tab);
        }
        framed_ui_panel_free(root);
    }
}

internal FramedUI_Panel *
framed_ui_panel_make(Void)
{
    FramedUI_Panel *result = framed_ui_panel_alloc();
    framed_ui_state->num_panels++;
    return(result);
}

internal Side
framed_ui_panel_get_side(FramedUI_Panel *panel)
{
    assert(panel->parent);
    Side result = panel->parent->children[Side_Min] == panel ? Side_Min : Side_Max;
    assert(panel->parent->children[result] == panel);
    return(result);
}

internal B32
framed_ui_panel_is_leaf(FramedUI_Panel *panel)
{
    B32 result = (framed_ui_panel_is_nil(panel->children[0]) || framed_ui_panel_is_nil(panel->children[1]));
    return(result);
}

internal UI_Comm
framed_ui_panel_hover_type(Str8 string, F32 width_in_em, FramedUI_Panel *root, Axis2 axis, B32 center, Side side)
{
    ui_next_width(ui_em(width_in_em, 1));
    ui_next_height(ui_em(width_in_em, 1));
    ui_next_color(ui_top_text_style()->color);

    UI_Box *box = ui_box_make(UI_BoxFlag_DrawBackground |
                              UI_BoxFlag_Clickable,
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
framed_ui_panel_update(FramedUI_Panel *root)
{
    if (framed_ui_panel_has_flag(root, FramedUI_PanelFlag_MarkedForDeletion))
    {
        FramedUI_CommandParams params = {0};
        params.panel = root;
        framed_ui_command_push(FramedUI_CommandKind_ClosePanel, params);
    }

    switch (root->split_axis)
    {
        case Axis2_X: ui_next_child_layout_axis(Axis2_X); break;
        case Axis2_Y: ui_next_child_layout_axis(Axis2_Y); break;
        invalid_case;
    }

    FramedUI_Panel *parent = root->parent;

    if (framed_ui_panel_is_nil(parent))
    {
        ui_next_width(ui_fill());
        ui_next_height(ui_fill());
    }
    else
    {
        Axis2 flipped_split_axis = !parent->split_axis;
        ui_next_size(parent->split_axis, ui_pct(root->pct_of_parent, 0));
        ui_next_size(flipped_split_axis, ui_fill());
    }

    //- hampus: Create root box

    // NOTE(hampus): It is clickable and has view scroll so that it can consume
    // all the click and scroll events at the end to prevent boxes from behind
    // it to take them.
    Str8 root_string = str8_pushf(framed_ui_state->frame_arena, "PanelRoot%p", root);
    ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
    UI_Box *box = ui_box_make(UI_BoxFlag_Clip |
                              UI_BoxFlag_Clickable |
                              UI_BoxFlag_ViewScroll,
                              root_string);

    ui_push_parent(box);
#if 1
    if (!framed_ui_panel_is_leaf(root))
    {
        //- hampus: Update children

        FramedUI_Panel *child0 = root->children[Side_Min];
        FramedUI_Panel *child1 = root->children[Side_Max];

        framed_ui_panel_update(child0);

        B32 dragging = false;
        F32 drag_delta = 0;
        ui_seed(root_string)
        {
            ui_next_size(root->split_axis, ui_em(0.2f, 1));
            ui_next_size(!root->split_axis, ui_pct(1, 1));
            ui_next_corner_radius(0);
            ui_next_hover_cursor(root->split_axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
            ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
            UI_Box *draggable_box = ui_box_make(UI_BoxFlag_Clickable |
                                                UI_BoxFlag_DrawBackground,
                                                str8_lit("DraggableBox"));
            UI_Comm comm = ui_comm_from_box(draggable_box);
            if (comm.dragging)
            {
                dragging = true;
                drag_delta = comm.drag_delta.v[root->split_axis];
            }
        }

        framed_ui_panel_update(child1);

        if (dragging)
        {
            child0->pct_of_parent -= drag_delta / box->fixed_size.v[root->split_axis];
            child1->pct_of_parent += drag_delta / box->fixed_size.v[root->split_axis];

            child0->pct_of_parent = f32_clamp(0, child0->pct_of_parent, 1.0f);
            child1->pct_of_parent = f32_clamp(0, child1->pct_of_parent, 1.0f);
        }
    }
    else
    {
        ui_push_string(root_string);
        box->layout_style.child_layout_axis = Axis2_Y;

        //- hampus: Check for any click events

        Gfx_EventList *event_list = ui_events();
        if (ui_mouse_is_inside_box(box) && !rectf32_contains_v2f32(ui_ctx->ctx_menu_root->fixed_rect, ui_mouse_pos()))
        {
            for (Gfx_Event *node = event_list->first; node != 0; node = node->next)
            {
                if (node->kind == Gfx_EventKind_KeyPress &&
                    (node->key == Gfx_Key_MouseLeft ||
                     node->key == Gfx_Key_MouseRight ||
                     node->key == Gfx_Key_MouseMiddle))
                {
                    if (framed_ui_panel_is_nil(framed_ui_state->next_focused_panel))
                    {
                        framed_ui_state->next_focused_panel = root;
                    }
                    if (root->window != framed_ui_state->master_window)
                    {
                        FramedUI_CommandParams params = {0};
                        params.window = root->window;
                        framed_ui_command_push(FramedUI_CommandKind_PushWindowToFront, params);
                    }
                }
            }
        }

        // NOTE(hampus): Axis2_COUNT is the center
        Axis2 hover_axis = Axis2_COUNT;
        Side hover_side = 0;
        UI_Comm tab_release_comms[FramedUI_TabReleaseKind_COUNT] = {0};
        B32 hovering_any_symbols = false;

        //- hampus: Drag & split symbols

        if (framed_ui_is_dragging())
        {
            FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pct(1, 1));
            UI_Box *split_symbols_container = ui_box_make(UI_BoxFlag_FixedPos, str8_lit("SplitSymbolsContainer"));
            if (root == drag_data->hovered_panel)
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
                        tab_release_comms[FramedUI_TabReleaseKind_Top] = framed_ui_panel_hover_type(
                                                                                                    str8_lit("TabReleaseTop"), size, root, Axis2_Y, false, Side_Min);

                        ui_spacer(ui_fill());
                    }
                    ui_spacer(ui_em(1, 1));

                    ui_next_width(ui_fill());
                    ui_row()
                    {
                        ui_spacer(ui_fill());

                        ui_next_child_layout_axis(Axis2_X);
                        tab_release_comms[FramedUI_TabReleaseKind_Left] = framed_ui_panel_hover_type(str8_lit("TabReleaseLeft"), size, root, Axis2_X, false, Side_Min);

                        ui_spacer(ui_em(1, 1));

                        tab_release_comms[FramedUI_TabReleaseKind_Center] = framed_ui_panel_hover_type(str8_lit("TabReleaseCenter"), size, root, Axis2_X, true, Side_Min);

                        ui_spacer(ui_em(1, 1));

                        ui_next_child_layout_axis(Axis2_X);
                        tab_release_comms[FramedUI_TabReleaseKind_Right] = framed_ui_panel_hover_type(str8_lit("TabReleaseRight"), size, root, Axis2_X, false, Side_Max);

                        ui_spacer(ui_fill());
                    }

                    ui_spacer(ui_em(1, 1));

                    ui_next_width(ui_fill());
                    ui_row()
                    {
                        ui_spacer(ui_fill());
                        tab_release_comms[FramedUI_TabReleaseKind_Bottom] = framed_ui_panel_hover_type(str8_lit("TabReleaseBottom"), size, root, Axis2_Y, false, Side_Max);
                        ui_spacer(ui_fill());
                    }
                    ui_spacer(ui_fill());
                    ui_pop_string();
                }
            }

            if (root != drag_data->tab->panel)
            {
                UI_Comm panel_comm = ui_comm_from_box(split_symbols_container);
                if (panel_comm.hovering)
                {
                    drag_data->hovered_panel = root;
                }
            }

            for (FramedUI_TabReleaseKind i = (FramedUI_TabReleaseKind) 0; i < FramedUI_TabReleaseKind_COUNT; ++i)
            {
                UI_Comm *comm = tab_release_comms + i;
                if (comm->hovering)
                {
                    hovering_any_symbols = true;
                    switch (i)
                    {
                        case FramedUI_TabReleaseKind_Center:
                        {
                            hover_axis = Axis2_COUNT;
                        } break;

                        case FramedUI_TabReleaseKind_Left:
                        case FramedUI_TabReleaseKind_Right:
                        {
                            hover_axis = Axis2_X;
                            hover_side = i == FramedUI_TabReleaseKind_Left ? Side_Min : Side_Max;
                        } break;

                        case FramedUI_TabReleaseKind_Top:
                        case FramedUI_TabReleaseKind_Bottom:
                        {
                            hover_axis = Axis2_Y;
                            hover_side = i == FramedUI_TabReleaseKind_Top ? Side_Min : Side_Max;
                        } break;

                        invalid_case;
                    }
                }
                if (comm->released)
                {
                    switch (i)
                    {
                        case FramedUI_TabReleaseKind_Center:
                        {
                            FramedUI_CommandParams params = {0};
                            params.panel = root;
                            params.tab = drag_data->tab;
                            params.set_active = true;
                            framed_ui_command_push(FramedUI_CommandKind_InsertTab, params);
                            dll_remove(framed_ui_state->open_windows.first,
                                       framed_ui_state->open_windows.last,
                                       drag_data->tab->panel->window);
                            framed_ui_state->drag_status = FramedUI_DragStatus_Released;
                        } break;

                        case FramedUI_TabReleaseKind_Left:
                        case FramedUI_TabReleaseKind_Right:
                        case FramedUI_TabReleaseKind_Top:
                        case FramedUI_TabReleaseKind_Bottom:
                        {
                            FramedUI_CommandParams params = {0};
                            params.tab = drag_data->tab;
                            params.panel = root;
                            params.axis = hover_axis;
                            params.side = hover_side;
                            framed_ui_command_push(FramedUI_CommandKind_SplitPanelAndInsertTab, params);

                            dll_remove(framed_ui_state->open_windows.first,
                                       framed_ui_state->open_windows.last,
                                       drag_data->tab->panel->window);
                            framed_ui_state->drag_status = FramedUI_DragStatus_Released;
                        } break;

                        invalid_case;
                    }
                    framed_ui_drag_release();
                }
            }
        }

        //- hampus: Drag & split preview overlay

        if (hovering_any_symbols)
        {
            Vec4F32 top_color = ui_top_rect_style()->color[0];
            top_color.r += 0.2f;
            top_color.g += 0.2f;
            top_color.b += 0.2f;
            top_color.a = 0.5f;

            ui_next_width(ui_pct(1, 1));
            ui_next_height(ui_pct(1, 1));
            UI_Box *container = ui_box_make(UI_BoxFlag_FixedPos, str8_lit("OverlayBoxContainer"));
            ui_parent(container)
            {
                if (hover_axis == Axis2_COUNT)
                {
                    ui_next_width(ui_pct(1, 1));
                    ui_next_height(ui_pct(1, 1));
                }
                else
                {
                    container->layout_style.child_layout_axis = hover_axis;
                    if (hover_side == Side_Max)
                    {
                        ui_spacer(ui_fill());
                    }

                    ui_next_size(hover_axis, ui_pct(0.5f, 1));
                    ui_next_size(axis_flip(hover_axis), ui_pct(1, 1));
                }

                ui_next_vert_gradient(top_color, top_color);
                ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("OverlayBox"));
            }
        }

        //- hampus: Tab bar

        F32 title_bar_height_em = 1.2f;
        F32 tab_spacing_em = 0.2f;
        F32 tab_button_height_em = title_bar_height_em - 0.2f;;

        UI_Box *title_bar = &g_nil_box;
        UI_Box *tabs_container = &g_nil_box;
        ui_next_width(ui_fill());
        ui_row()
        {
            ui_next_color(v4f32(0.1f, 0.1f, 0.1f, 1.0f));
            ui_next_width(ui_fill());
            ui_next_height(ui_em(title_bar_height_em, 1));
            if (root->tab_group.count == 1)
            {
                ui_next_extra_box_flags(UI_BoxFlag_Clickable |
                                        UI_BoxFlag_HotAnimation |
                                        UI_BoxFlag_ActiveAnimation);
            }

            ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBar));
            title_bar = ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("TitleBar"));
            ui_parent(title_bar)
            {
                // NOTE(hampus): Tab dropdown menu

                ui_next_width(ui_fill());
                ui_next_height(ui_fill());
                ui_row()
                {
                    UI_Key tab_dropown_menu_key = ui_key_from_string(title_bar->key, str8_lit("TabDropdownMenu"));

                    //- hampus: Tab dropdown list

                    // NOTE(hampus): Calculate the largest tab to decice the dropdown list size
                    Vec2F32 largest_dim = v2f32(0, 0);
                    if (root->tab_group.count >= 2)
                    {
                        for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
                        {
                            Vec2F32 dim = render_measure_text(render_font_from_key(ui_renderer(), ui_top_font_key()), tab->display_string);
                            largest_dim.x = f32_max(largest_dim.x, dim.x);
                            largest_dim.y = f32_max(largest_dim.y, dim.y);
                        }

                        largest_dim.x += ui_top_font_line_height()*0.5f;
                        ui_ctx_menu(tab_dropown_menu_key)
                        {
                            ui_corner_radius(0)
                            {
                                for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
                                {
                                    ui_next_hover_cursor(Gfx_Cursor_Hand);
                                    ui_next_extra_box_flags(UI_BoxFlag_ActiveAnimation |
                                                            UI_BoxFlag_HotAnimation |
                                                            UI_BoxFlag_Clickable);
                                    ui_next_corner_radius(ui_top_font_line_height() * 0.1f);
                                    UI_Box *row_box = ui_named_row_beginf("TabDropDownListEntry%p", tab);

                                    ui_next_height(ui_em(1, 1.0f));
                                    ui_next_width(ui_pixels(largest_dim.x, 1));
                                    // TODO(hampus): Theming
                                    UI_Box *tab_box = ui_box_make(UI_BoxFlag_DrawText, str8_lit(""));

                                    ui_box_equip_display_string(tab_box, tab->display_string);
                                    ui_next_height(ui_em(1, 1));
                                    ui_next_width(ui_em(1, 1));
                                    ui_next_icon(RENDER_ICON_CROSS);
                                    ui_next_hover_cursor(Gfx_Cursor_Hand);
                                    ui_next_corner_radius(ui_top_font_line_height() * 0.1f);
                                    UI_Box *close_box = ui_box_makef(UI_BoxFlag_Clickable |
                                                                     UI_BoxFlag_DrawText |
                                                                     UI_BoxFlag_HotAnimation |
                                                                     UI_BoxFlag_ActiveAnimation,
                                                                     "CloseTabButton%p", tab);
                                    UI_Comm close_comm = ui_comm_from_box(close_box);
                                    if (close_comm.hovering)
                                    {
                                        close_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder;

                                    }
                                    if (close_comm.clicked && !framed_ui_tab_has_flag(tab, FramedUI_TabFlag_Pinned))
                                    {
                                        tab->flags |= FramedUI_TabFlag_MarkedForDeletion;
                                    }
                                    ui_named_row_end();
                                    UI_Comm row_comm = ui_comm_from_box(row_box);
                                    if (row_comm.pressed)
                                    {
                                        FramedUI_CommandParams params = {0};
                                        params.tab = tab;
                                        framed_ui_command_push(FramedUI_CommandKind_SetTabActive, params);
                                    }
                                    if (row_comm.hovering)
                                    {
                                        row_box->flags |= UI_BoxFlag_DrawBackground | UI_BoxFlag_DrawBorder;
                                    }
                                }
                            }
                        }

                        ui_next_extra_box_flags(UI_BoxFlag_Clip);
                        ui_next_height(ui_pct(1, 1));
                        ui_named_column(str8_lit("TabDropDownContainer"))
                        {
                            F32 corner_radius = (F32) ui_top_font_line_height() * 0.25f;
                            ui_spacer(ui_em(0.1f, 1));
                            ui_next_icon(RENDER_ICON_LIST);
                            ui_next_width(ui_em(title_bar_height_em+0.3f, 1));
                            ui_next_height(ui_em(title_bar_height_em+0.3f, 1));
                            ui_next_hover_cursor(Gfx_Cursor_Hand);
                            ui_next_vert_corner_radius(corner_radius, 0);
                            ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
                            UI_Box *tab_dropdown_list_box = ui_box_make(UI_BoxFlag_DrawBackground |
                                                                        UI_BoxFlag_DrawBorder |
                                                                        UI_BoxFlag_HotAnimation |
                                                                        UI_BoxFlag_ActiveAnimation |
                                                                        UI_BoxFlag_Clickable |
                                                                        UI_BoxFlag_DrawText,
                                                                        str8_lit("TabDropdownList"));
                            UI_Comm tab_dropdown_list_comm = ui_comm_from_box(tab_dropdown_list_box);
                            if (tab_dropdown_list_comm.pressed)
                            {
                                ui_ctx_menu_open(tab_dropdown_list_comm.box->key,
                                                 v2f32(0, -ui_em(0.1f, 1).value), tab_dropown_menu_key);
                            }
                        }
                    }

                    ui_spacer(ui_em(tab_spacing_em, 1));

                    //- hampus: Tab buttons

                    Arena_Temporary scratch = get_scratch(0, 0);

                    typedef struct Task Task;
                    struct Task
                    {
                        Task *next;
                        Task *prev;
                        FramedUI_Tab *tab;
                        UI_Box *box;
                    };

                    UI_Box *active_box = &g_nil_box;
                    Task *first_task = 0;
                    Task *last_task = 0;

                    B32 tab_overflow = false;
                    ui_next_width(ui_fill());
                    ui_next_height(ui_pct(1, 1));
                    ui_next_extra_box_flags(UI_BoxFlag_Clip | UI_BoxFlag_AnimateScroll);
                    tabs_container = ui_named_row_begin(str8_lit("TabsContainer"));
                    for (FramedUI_Tab *tab = root->tab_group.first; !framed_ui_tab_is_nil(tab); tab = tab->next)
                    {
                        if (framed_ui_tab_has_flag(tab, FramedUI_TabFlag_MarkedForDeletion))
                        {
                            FramedUI_CommandParams params = {0};
                            params.tab = tab;
                            framed_ui_command_push(FramedUI_CommandKind_CloseTab, params);
                        }
                        ui_next_height(ui_pct(1, 1));
                        ui_next_width(ui_children_sum(1));
                        ui_next_child_layout_axis(Axis2_Y);
                        UI_Box *tab_column = ui_box_makef(UI_BoxFlag_AnimateX, "TabColumn%p", tab);
                        ui_parent(tab_column)
                        {
                            if (tab != root->tab_group.active_tab)
                            {
                                ui_spacer(ui_em(0.1f, 1));
                            }
                            else
                            {
                                active_box = tab_column;
                            }
                            ui_spacer(ui_em(0.1f, 1));
                            UI_Box *tab_box = framed_ui_tab_button(tab);
                            Task *task = push_struct(scratch.arena, Task);
                            task->tab = tab;
                            task->box = tab_column;
                            dll_push_back(first_task, last_task, task);
                        }
                        ui_spacer(ui_em(tab_spacing_em, 1));
                    }

                    ui_named_row_end();

                    //- hampus: Offset active tab into view

                    if (root->tab_group.count > 0)
                    {
                        FramedUI_Tab *active_tab = root->tab_group.active_tab;
                        UI_Box *last_box = &g_nil_box;
                        for (Task *task = last_task; task; task = task->prev)
                        {
                            last_box = task->box;
                            if (!ui_box_was_created_this_frame(last_box))
                            {
                                break;
                            }
                        }

                        tab_overflow = tabs_container->scroll.x != 0 || (last_box->rel_pos.x+last_box->fixed_size.x) > tabs_container->fixed_size.x;

                        Vec2F32 tab_visiblity_range = v2f32(active_box->rel_pos.x,
                                                            active_box->rel_pos.x + active_box->fixed_size.x);

                        tab_visiblity_range.x = f32_max(0, tab_visiblity_range.x);
                        tab_visiblity_range.y = f32_max(0, tab_visiblity_range.y);

                        Vec2F32 tab_bar_visiblity_range = v2f32(tabs_container->scroll.x,
                                                                tabs_container->scroll.x + tabs_container->fixed_size.x);

                        F32 delta_left = tab_visiblity_range.x - tab_bar_visiblity_range.x ;
                        F32 delta_right = tab_visiblity_range.y - tab_bar_visiblity_range.y;
                        delta_left = f32_min(delta_left, 0);
                        delta_right = f32_max(delta_right, 0);

                        tabs_container->scroll.x += delta_right;
                        tabs_container->scroll.x += delta_left;
                        tabs_container->scroll.x = f32_max(0, tabs_container->scroll.x);
                    }

                    release_scratch(scratch);

                    //- hampus: Tab bar navigaton buttons

                    if (tab_overflow)
                    {
                        ui_next_height(ui_em(title_bar_height_em, 1));
                        ui_next_width(ui_em(title_bar_height_em, 1));
                        ui_next_icon(RENDER_ICON_LEFT_OPEN);
                        ui_next_hover_cursor(Gfx_Cursor_Hand);
                        ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
                        UI_Box *prev_tab_button = ui_box_make(UI_BoxFlag_Clickable |
                                                              UI_BoxFlag_DrawText |
                                                              UI_BoxFlag_HotAnimation |
                                                              UI_BoxFlag_ActiveAnimation |
                                                              UI_BoxFlag_DrawBackground,
                                                              str8_lit("PrevTabButton"));

                        UI_Comm prev_tab_comm = ui_comm_from_box(prev_tab_button);
                        if (prev_tab_comm.pressed)
                        {
                            FramedUI_CommandParams params = {0};
                            if (!framed_ui_tab_is_nil(root->tab_group.active_tab->prev))
                            {
                                params.tab = root->tab_group.active_tab->prev;
                            }
                            else
                            {
                                params.tab = root->tab_group.last;
                            }
                            framed_ui_command_push(FramedUI_CommandKind_SetTabActive, params);
                        }

                        ui_next_height(ui_em(title_bar_height_em, 1));
                        ui_next_width(ui_em(title_bar_height_em, 1));
                        ui_next_icon(RENDER_ICON_RIGHT_OPEN);
                        ui_next_hover_cursor(Gfx_Cursor_Hand);
                        ui_next_color(framed_ui_color_from_theme(FramedUI_Color_TabBarButtons));
                        UI_Box *next_tab_button = ui_box_make(UI_BoxFlag_Clickable |
                                                              UI_BoxFlag_DrawText |
                                                              UI_BoxFlag_HotAnimation |
                                                              UI_BoxFlag_ActiveAnimation |
                                                              UI_BoxFlag_DrawBackground,
                                                              str8_lit("NextTabButton"));

                        UI_Comm next_tab_comm = ui_comm_from_box(next_tab_button);
                        if (next_tab_comm.pressed)
                        {
                            FramedUI_CommandParams params = {0};
                            if (!framed_ui_tab_is_nil(root->tab_group.active_tab->next))
                            {
                                params.tab = root->tab_group.active_tab->next;
                            }
                            else
                            {
                                params.tab = root->tab_group.first;
                            }
                            framed_ui_command_push(FramedUI_CommandKind_SetTabActive, params);
                        }
                    }

                    //- hampus: Tab close button

                    ui_next_height(ui_em(title_bar_height_em, 1));
                    ui_next_width(ui_em(title_bar_height_em, 1));
                    ui_next_icon(RENDER_ICON_CROSS);
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
                        root->flags |= FramedUI_PanelFlag_MarkedForDeletion;
                    }
                }
            }

            if (root->tab_group.count == 1 &&
                !framed_ui_is_dragging())
            {
                UI_Comm title_bar_comm = ui_comm_from_box(title_bar);
                if (title_bar_comm.pressed)
                {
                    framed_ui_drag_begin_reordering(root->tab_group.active_tab);
                    framed_ui_wait_for_drag_threshold();
                }
            }
        }

        //- hampus: Tab content

        ui_next_width(ui_fill());
        ui_next_height(ui_fill());
        ui_next_color(framed_ui_color_from_theme(FramedUI_Color_PanelOverlayInactive));
        UI_Box *content_dim = ui_box_make(UI_BoxFlag_FixedPos, str8_lit("ContentDim"));
        content_dim->flags |= (UI_BoxFlags) (UI_BoxFlag_DrawBackground * (root != framed_ui_state->focused_panel));

        if (ui_mouse_is_inside_box(content_dim) &&
            framed_ui_is_tab_reordering() &&
            !ui_mouse_is_inside_box(title_bar))
        {
            framed_ui_wait_for_drag_threshold();
        }


        ui_next_width(ui_fill());
        ui_next_height(ui_fill());
        UI_Box *content_box_container = ui_box_make(0, str8_lit("ContentBoxContainer"));
        ui_parent(content_box_container)
        {
            if (root == framed_ui_state->focused_panel)
            {
                ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_PanelBorderActive));
            }
            else
            {
                ui_next_border_color(framed_ui_color_from_theme(FramedUI_Color_PanelBorderInactive));
            }
            ui_next_width(ui_fill());
            ui_next_height(ui_fill());
            ui_next_child_layout_axis(Axis2_Y);
            // TODO(hampus): Should this actually be called panel color...
            ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
            UI_Box *content_box = ui_box_make(UI_BoxFlag_DrawBackground |
                                              UI_BoxFlag_DrawBorder |
                                              UI_BoxFlag_Clip,
                                              str8_lit("ContentBox"));

            ui_parent(content_box)
            {
                // NOTE(hampus): Add some padding for the content
                UI_Size padding = ui_em(0.3f, 1);
                ui_spacer(padding);
                ui_next_width(ui_fill());
                ui_next_height(ui_fill());
                ui_row()
                {
                    ui_spacer(padding);
                    if (!framed_ui_tab_is_nil(root->tab_group.active_tab))
                    {
                        FramedUI_Tab *tab = root->tab_group.active_tab;
                        ui_next_width(ui_fill());
                        ui_next_height(ui_fill());
                        ui_next_extra_box_flags(UI_BoxFlag_Clip);
                        ui_named_column(str8_lit("PaddedContentBox"))
                        {
                            tab->view_info.function(&tab->view_info);
                        }
                        ui_spacer(padding);
                    }
                }
                ui_spacer(padding);
            }
        }

        ui_pop_string();
    }

    //- hampus: Consume any input

    B32 take_input_from_root = true;
    if (framed_ui_is_dragging())
    {
        if (root == framed_ui_state->drag_data.tab->panel)
        {
            // NOTE(hampus): We don't want to consume all the events
            // if we're dragging a tab in order to make the others panels
            // able to get hovered
            take_input_from_root = false;
        }
    }

    if (take_input_from_root)
    {
        // NOTE(hampus): Consume all non-taken events
        UI_Comm root_comm = ui_comm_from_box(box);
    }
#endif
    ui_pop_parent();
}

////////////////////////////////
//~ hampus: Window

internal Void
framed_ui_window_push_to_front(FramedUI_Window *window)
{
    if (window != framed_ui_state->master_window)
    {
        dll_remove(framed_ui_state->open_windows.first, framed_ui_state->open_windows.last, window);
        dll_push_front(framed_ui_state->open_windows.first, framed_ui_state->open_windows.last, window);
    }
}

internal Void
framed_ui_window_close(FramedUI_Window *window)
{
    FramedUI_Panel *root_panel = window->root_panel;
    framed_ui_panel_free_recursively(root_panel);
    dll_remove(framed_ui_state->open_windows.first, framed_ui_state->open_windows.last, window);
    framed_ui_window_free(window);
}

internal Void
framed_ui_window_set_pos(FramedUI_Window *window, Vec2F32 pos)
{
    Vec2F32 dim = rectf32_dim(window->rect);
    window->rect.min = pos;
    window->rect.max = v2f32_add_v2f32(pos, dim);
}

internal FramedUI_Window *
framed_ui_window_alloc(Void)
{
    FramedUI_Window *result = (FramedUI_Window *) framed_ui_state->first_free_window;
    if (result == 0)
    {
        result = push_struct(framed_ui_state->perm_arena, FramedUI_Window);
        log_info("Allocated window: %p", result);
    }
    else
    {
        ASAN_UNPOISON_MEMORY_REGION(result, sizeof(FramedUI_Window));
        framed_ui_state->first_free_window = framed_ui_state->first_free_window->next;
    }
    memory_zero_struct(result);
    result->root_panel = &g_nil_panel;
    return(result);
}

internal Void
framed_ui_window_free(FramedUI_Window *window)
{
    arena_destroy(window->arena);
    FramedUI_FreeWindow *free_window = (FramedUI_FreeWindow *) window;
    free_window->next = framed_ui_state->first_free_window;
    framed_ui_state->first_free_window = free_window;
    ASAN_POISON_MEMORY_REGION(window, sizeof(FramedUI_Window));
}

internal FramedUI_Window *
framed_ui_window_make(Vec2F32 min, Vec2F32 max)
{
    FramedUI_Window *result = framed_ui_window_alloc();
    result->arena = arena_create("Window%dArena", framed_ui_state->num_windows);
    result->string = str8_pushf(result->arena, "Window%d", framed_ui_state->num_windows);
    FramedUI_Panel *panel = framed_ui_panel_make();
    panel->window = result;
    result->rect.min = min;
    result->rect.max = max;
    result->root_panel = panel;
    framed_ui_state->num_windows++;
    return(result);
}

internal UI_Comm
framed_ui_window_edge_resizer(FramedUI_Window *window, Str8 string, Axis2 axis, Side side, F32 size_in_em)
{
    ui_next_size(axis, ui_em(size_in_em, 1));
    ui_next_size(axis_flip(axis), ui_fill());
    ui_next_hover_cursor(axis == Axis2_X ? Gfx_Cursor_SizeWE : Gfx_Cursor_SizeNS);
    UI_Box *box = ui_box_make(UI_BoxFlag_Clickable, string);

    Vec2U32 screen_size = gfx_get_window_client_area(ui_renderer()->gfx);
    UI_Comm comm = {0};
    if (!framed_ui_is_dragging())
    {
        comm = ui_comm_from_box(box);
        if (comm.dragging)
        {
            F32 drag_delta = comm.drag_delta.v[axis];
            window->rect.s[side].v[axis] -= drag_delta;
        }
    }
    return(comm);
}

internal UI_Comm
framed_ui_window_corner_resizer(FramedUI_Window *window, Str8 string, Corner corner, F32 size_in_em)
{
    ui_next_width(ui_em(size_in_em, 1));
    ui_next_height(ui_em(size_in_em, 1));
    ui_next_hover_cursor(corner == Corner_TopLeft || corner == Corner_BottomRight ?
                         Gfx_Cursor_SizeNWSE :
                         Gfx_Cursor_SizeNESW);
    UI_Box *box = ui_box_make(UI_BoxFlag_Clickable, string);
    UI_Comm comm = {0};
    if (framed_ui_drag_is_inactive())
    {
        comm = ui_comm_from_box(box);
        Vec2F32 screen_size = v2f32_from_v2u32(gfx_get_window_area(ui_renderer()->gfx));
        if (comm.dragging)
        {
            switch (corner)
            {
                case Corner_TopLeft:
                {
                    window->rect.min = v2f32_sub_v2f32(window->rect.min, comm.drag_delta);
                } break;

                case Corner_BottomLeft:
                {
                    window->rect.min.x -= comm.drag_delta.v[Axis2_X];
                    window->rect.max.y -= comm.drag_delta.v[Axis2_Y];
                } break;

                case Corner_TopRight:
                {
                    window->rect.min.y -= comm.drag_delta.v[Axis2_Y];
                    window->rect.max.x -= comm.drag_delta.v[Axis2_X];
                } break;

                case Corner_BottomRight:
                {
                    window->rect.max = v2f32_sub_v2f32(window->rect.max, comm.drag_delta);
                } break;

                invalid_case;
            }
        }
    }

    return(comm);
}

internal Void
framed_ui_window_update(FramedUI_Window *window)
{
    ui_seed(window->string)
    {
        if (window == framed_ui_state->master_window)
        {
            framed_ui_panel_update(window->root_panel);
        }
        else
        {
            Vec2U32 window_dim = gfx_get_window_client_area(ui_renderer()->gfx);
            window->rect.min.v[Axis2_X] = f32_clamp(0, window->rect.min.v[Axis2_X], (F32) window_dim.x);
            window->rect.min.v[Axis2_Y] = f32_clamp(0, window->rect.min.v[Axis2_Y], (F32) window_dim.y);
            // window->rect.max.v[Axis2_X] = f32_clamp(window->rect.min.v[Axis2_X]+ui_top_font_line_height(), window->rect.max.v[Axis2_X], (F32) window_dim.x);
            // window->rect.max.v[Axis2_Y] = f32_clamp(window->rect.min.v[Axis2_Y]+ui_top_font_line_height(), window->rect.max.v[Axis2_Y], (F32) window_dim.y);
            F32 resizer_size_in_em = 0.4f;
            // NOTE(hampus): Take the resizers into account and offset it by their size
            RectF32 rect = window->rect;
            rect.min.x -= ui_top_font_line_height() * resizer_size_in_em;
            rect.min.y -= ui_top_font_line_height() * resizer_size_in_em;
            ui_next_fixed_rect(rect);
            ui_next_child_layout_axis(Axis2_X);
            ui_next_color(framed_ui_color_from_theme(FramedUI_Color_Panel));
            UI_Box *window_container = ui_box_make(UI_BoxFlag_FixedRect |
                                                   UI_BoxFlag_DrawDropShadow,
                                                   str8_lit(""));
            ui_parent(window_container)
            {
                ui_next_height(ui_fill());
                ui_column()
                {
                    framed_ui_window_corner_resizer(window, str8_lit("TopLeftWindowResize"), Corner_TopLeft, resizer_size_in_em);
                    framed_ui_window_edge_resizer(window, str8_lit("TopWindowResize"), Axis2_X, Side_Min, resizer_size_in_em);
                    framed_ui_window_corner_resizer(window, str8_lit("BottomLeftWindowResize"), Corner_BottomLeft, resizer_size_in_em);
                }

                ui_next_width(ui_fill());
                ui_next_height(ui_fill());
                ui_column()
                {
                    framed_ui_window_edge_resizer(window, str8_lit("LeftWindowResize"), Axis2_Y, Side_Min, resizer_size_in_em);
                    framed_ui_panel_update(window->root_panel);
                    framed_ui_window_edge_resizer(window, str8_lit("RightWindowResize"), Axis2_Y, Side_Max, resizer_size_in_em);
                }

                ui_next_height(ui_fill());
                ui_column()
                {
                    framed_ui_window_corner_resizer(window, str8_lit("TopRightWindowResize"), Corner_TopRight, resizer_size_in_em);
                    framed_ui_window_edge_resizer(window, str8_lit("BottomWindowResize"), Axis2_X, Side_Max, resizer_size_in_em);
                    framed_ui_window_corner_resizer(window, str8_lit("BottomRightWindowResize"), Corner_BottomRight, resizer_size_in_em);
                }
            }
        }
    }
}

////////////////////////////////
//~ hampus: Tab view

internal Void *
framed_ui_get_or_push_view_data_(FramedUI_TabViewInfo *view_info, U64 size)
{
    if (view_info->data == 0)
    {
        view_info->data = push_array(view_info->arena, U8, size);
    }
    Void *result = view_info->data;
    return(result);
}

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_default)
{
    FramedUI_Tab *tab = view_info->data;
    FramedUI_Panel *panel = tab->panel;
    FramedUI_Window *window = panel->window;
    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    ui_row()
    {
        ui_next_width(ui_pct(1, 0));
        ui_next_height(ui_pct(1, 0));
        ui_column()
        {
            if (ui_button(str8_lit("Split panel X")).pressed)
            {
                framed_ui_panel_split(panel, Axis2_X);
            }
            ui_spacer(ui_em(0.5f, 1));
            if (ui_button(str8_lit("Split panel Y")).pressed)
            {
                framed_ui_panel_split(panel, Axis2_Y);
            }
            ui_spacer(ui_em(0.5f, 1));
            if (ui_button(str8_lit("Close panel")).pressed)
            {
                panel->flags |= FramedUI_PanelFlag_MarkedForDeletion;
            }
            ui_spacer(ui_em(0.5f, 1));
            if (ui_button(str8_lit("Add tab")).pressed)
            {
                FramedUI_Tab *new_tab = framed_ui_tab_make(0, 0, str8_lit(""));
                FramedUI_CommandParams params = {0};
                params.tab = new_tab;
                params.panel = panel;
                params.set_active = true;
                framed_ui_command_push(FramedUI_CommandKind_InsertTab, params);
            }
            ui_spacer(ui_em(0.5f, 1));
            if (ui_button(str8_lit("Close tab")).pressed)
            {
                if (!framed_ui_tab_has_flag(tab, FramedUI_TabFlag_Pinned))
                {
                    tab->flags |= FramedUI_TabFlag_MarkedForDeletion;
                }
            }
            ui_spacer(ui_em(0.5f, 1));
            ui_row()
            {
                ui_check(&ui_ctx->show_debug_lines, str8_lit("ShowDebugLines"));
                ui_spacer(ui_em(0.5f, 1));
                ui_text(str8_lit("Show debug lines"));
            }
        }
    }
}

////////////////////////////////
//~ hampus: Main update

internal Void
framed_ui_update(Render_Context *renderer, Gfx_EventList *event_list)
{
    Vec2F32 mouse_pos = ui_mouse_pos();

    //- hampus: Check for any interesting events

    B32 left_mouse_released = false;
    for (Gfx_Event *event = event_list->first; event != 0; event = event->next)
    {
        switch (event->kind)
        {
            case Gfx_EventKind_KeyRelease:
            {
                if (event->key == Gfx_Key_MouseLeft)
                {
                    left_mouse_released = true;
                }
            } break;

            default: break;
        }
    }

    framed_ui_state->focused_panel = framed_ui_state->next_focused_panel;
    framed_ui_state->next_focused_panel = &g_nil_panel;

    //- hampus: Update windows

    ui_next_width(ui_fill());
    ui_next_height(ui_fill());
    UI_Box *window_root_parent = ui_box_make(UI_BoxFlag_DrawBackground, str8_lit("RootWindow"));
    ui_parent(window_root_parent)
    {
        for (FramedUI_Window *window = framed_ui_state->open_windows.first; window != 0; window = window->next)
        {
            framed_ui_window_update(window);
        }
    }

    //- hampus: Update tab drag

    if (left_mouse_released && framed_ui_is_dragging())
    {
        framed_ui_drag_release();
    }

    FramedUI_DragData *drag_data = &framed_ui_state->drag_data;
    switch (framed_ui_state->drag_status)
    {
        case FramedUI_DragStatus_Inactive: break;
        case FramedUI_DragStatus_Reordering: break;
        case FramedUI_DragStatus_WaitingForDragThreshold:
        {
            F32 drag_threshold = ui_top_font_line_height() * 3;
            Vec2F32 delta = v2f32_sub_v2f32(mouse_pos, drag_data->drag_origin);
            if (f32_abs(delta.x) > drag_threshold || f32_abs(delta.y) > drag_threshold)
            {
                FramedUI_Tab *tab = drag_data->tab;
                FramedUI_Panel *tab_panel = tab->panel;
                B32 create_new_window = !(tab->panel == tab->panel->window->root_panel &&
                                          tab_panel->tab_group.count == 1 &&
                                          tab_panel->window != framed_ui_state->master_window);

                if (create_new_window)
                {
                    // NOTE(hampus): Calculate the new window size
                    FramedUI_Panel *panel_child = tab->panel;
                    Vec2F32 prev_panel_pct = v2f32(1, 1);
                    for (FramedUI_Panel *panel_parent = panel_child->parent; !framed_ui_panel_is_nil(panel_parent); panel_parent = panel_parent->parent)
                    {
                        Axis2 axis = panel_parent->split_axis;
                        prev_panel_pct.v[axis] *= panel_child->pct_of_parent;
                        panel_child = panel_parent;
                    }

                    Vec2F32 new_window_dim = v2f32_hadamard_v2f32(rectf32_dim(panel_child->window->rect), prev_panel_pct);

                    FramedUI_Window *new_window = framed_ui_window_make(v2f32(0, 0), new_window_dim);
                    framed_ui_panel_remove_tab(tab);
                    framed_ui_panel_insert_tab(new_window->root_panel, tab);
                }
                else
                {
                    drag_data->tab->panel->sibling = &g_nil_panel;
                }

                framed_ui_window_push_to_front(drag_data->tab->panel->window);
                framed_ui_window_set_pos(drag_data->tab->panel->window, ui_mouse_pos());
                framed_ui_state->next_focused_panel = drag_data->tab->panel;
                framed_ui_state->drag_status = FramedUI_DragStatus_Dragging;
                log_info("Drag: dragging");
            }
        } break;
        case FramedUI_DragStatus_Dragging:
        {
            FramedUI_Window *window = drag_data->tab->panel->window;
            Vec2F32 mouse_delta = v2f32_sub_v2f32(mouse_pos, ui_prev_mouse_pos());
            framed_ui_window_set_pos(drag_data->tab->panel->window, v2f32_add_v2f32(window->rect.min, mouse_delta));
        } break;
        case FramedUI_DragStatus_Released:
        {
            memory_zero_struct(&framed_ui_state->drag_data);
            framed_ui_state->drag_status = FramedUI_DragStatus_Inactive;
        } break;
        invalid_case;
    }

    if (left_mouse_released && framed_ui_state->drag_status != FramedUI_DragStatus_Inactive)
    {
        framed_ui_drag_end();
    }

    //- hampus: Commands

    for (FramedUI_CommandNode *node = framed_ui_state->cmd_list.first; node; node = node->next)
    {
        FramedUI_Command *cmd = &node->command;
        FramedUI_CommandParams *params = &cmd->params;
        switch (cmd->kind)
        {
            case FramedUI_CommandKind_RemoveTab:
            {
                log_info("Executed command: tab_deattach (%"PRISTR8")", str8_expand(params->tab->display_string));
                framed_ui_panel_remove_tab(params->tab);
            } break;
            case FramedUI_CommandKind_InsertTab:
            {
                framed_ui_panel_insert_tab(params->panel, params->tab);
                log_info("Executed command: tab_attach");
            } break;
            case FramedUI_CommandKind_CloseTab:
            {
                log_info("Executed command: tab_close (%"PRISTR8")", str8_expand(params->tab->display_string));
                for (U64 i = 0; i < array_count(framed_ui_state->tab_view_table); ++i)
                {
                    if (params->tab == framed_ui_state->tab_view_table[i])
                    {
                        framed_ui_state->tab_view_table[i] = &g_nil_tab;
                    }
                }
                framed_ui_panel_remove_tab(params->tab);
                framed_ui_tab_free(params->tab);
            }; break;
            case FramedUI_CommandKind_SplitPanel:
            {
                framed_ui_panel_split(params->panel, params->axis);
                log_info("Executed command: panel_split");
            } break;
            case FramedUI_CommandKind_SplitPanelAndInsertTab:
            {
                B32 releasing_on_same_panel =
                    params->panel == params->tab->panel &&
                    params->panel->tab_group.count == 0;

                framed_ui_panel_split(params->panel, params->axis);

                // NOTE(hampus): panel_split always put the new panel
                // to the left/top
                if (releasing_on_same_panel)
                {
                    if (params->side == Side_Max)
                    {
                        swap(params->panel->parent->children[Side_Min], params->panel->parent->children[Side_Max], FramedUI_Panel *);
                    }
                }
                else
                {
                    if (params->side == Side_Min)
                    {
                        swap(params->panel->parent->children[Side_Min], params->panel->parent->children[Side_Max], FramedUI_Panel *);
                    }
                }

                FramedUI_Panel *panel = params->panel->parent->children[params->side];

                framed_ui_state->next_focused_panel = panel;
                if (params->panel->window != framed_ui_state->master_window)
                {
                    framed_ui_state->next_top_most_window = params->panel->window;
                }
                framed_ui_panel_insert_tab(panel, params->tab);
                log_info("Executed command: panel_split_and_attach");
            } break;
            case FramedUI_CommandKind_SetTabActive:
            {
                framed_ui_panel_set_active_tab(params->tab->panel, params->tab);
                log_info("Executed command: panel_set_active_tab");
            }break;
            case FramedUI_CommandKind_ClosePanel:
            {
                framed_ui_panel_close(params->panel);
                log_info("Executed command: panel_close");
            } break;
            case FramedUI_CommandKind_CloseWindow:
            {
                framed_ui_window_close(params->window);
                log_info("Executed command: window_close");
            }break;
            case FramedUI_CommandKind_PushWindowToFront:
            {
                framed_ui_window_push_to_front(params->window);
                log_info("Executed command: window_set_top_most");
            }break;
            invalid_case;
        }
    }

    //- hampus: Frame end

    if (framed_ui_panel_is_nil(framed_ui_state->next_focused_panel))
    {
        framed_ui_state->next_focused_panel = framed_ui_state->focused_panel;
    }

    framed_ui_state->next_top_most_window = 0;
    framed_ui_state->cmd_list.first = 0;
    framed_ui_state->cmd_list.last = 0;
}

#if 0
FRAMED_UI_COMMAND(tab_swap)
{
    FramedUI_TabSwap *data = (FramedUI_TabSwap *) params;
    FramedUI_Panel *panel = params->tab0->panel;

    FramedUI_Tab *tab0 = params->tab0;
    FramedUI_Tab *tab1 = params->tab1;

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
#endif