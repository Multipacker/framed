#ifndef FRAMED_UI_H
#define FRAMED_UI_H

typedef struct FUI_Panel FUI_Panel;
struct FUI_Panel
{
    FUI_Panel *first;
    FUI_Panel *last;
    FUI_Panel *next;
    FUI_Panel *prev;
    FUI_Panel *parent;
    F32 pct_of_parent;
    Axis2 split_axis;
};

typedef struct FUI_PanelRec FUI_PanelRec;
struct FUI_PanelRec
{
    FUI_Panel *next;
    S32 push_count;
    S32 pop_count;
};

typedef struct FUI_Window FUI_Window;
struct FUI_Window
{
    Arena *arena;

    FUI_Window *next;
    FUI_Window *prev;

    FUI_Panel *root_panel;
    FUI_Panel *first_free_panel;
};

internal FUI_PanelRec fui_panel_rec_depth_first_pre_order(FUI_Panel *panel);
internal RectF32 fui_child_rect_from_parent_rect(FUI_Panel *child, RectF32 parent_rect);

#endif // FRAMED_UI_H