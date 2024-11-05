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
fui_child_rect_from_parent_rect(FUI_Panel *child, RectF32 parent_rect)
{
    RectF32 result    = parent_rect;
    FUI_Panel *parent = child->parent;
    if (parent != 0)
    {
        Vec2F32 parent_rect_dim         = rectf32_dim(parent_rect);
        result.p1.v[parent->split_axis] = result.p0.v[parent->split_axis];
        for (FUI_Panel *p = parent->first; p != child && p != 0; p = p->next)
        {
            result.p0.v[parent->split_axis] += p->pct_of_parent * parent_rect_dim.v[parent->split_axis];
            result.p1.v[parent->split_axis] = result.p0.v[parent->split_axis];
        }
        result.p1.v[parent->split_axis] += child->pct_of_parent * parent_rect_dim.v[parent->split_axis];
    }
    return result;
}