internal FUI_PanelRec
fui_panel_rec_depth_first_pre_order(FUI_Panel *panel)
{
    FUI_PanelRec result = {0};
    if (panel->first != 0)
    {
        result.next = panel->first;
        result.push_count += 1;
    }
    else
    {
        for (FUI_Panel *p = panel; p != 0; p = p->parent)
        {
            if (p->next != 0)
            {
                result.next = p->next;
                break;
            }
            result.pop_count += 1;
        }
    }
    return result;
}

internal RectF32
fui_rect_from_panel_child_rect(FUI_Panel *child, RectF32 rect)
{
    RectF32 result    = {0};
    FUI_Panel *parent = child->parent;
    if (parent != 0)
    {
        result.s[parent->split_axis];
        for (FUI_Panel *p = parent->first; p != 0; p = p->next)
        {
        }
    }
    return result;
}