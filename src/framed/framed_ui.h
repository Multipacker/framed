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

#endif // FRAMED_UI_H