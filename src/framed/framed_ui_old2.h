#ifndef UI_PANEL_TEST_H
#define UI_PANEL_TEST_H

typedef struct FramedUI_Panel FramedUI_Panel;
typedef struct FramedUI_Tab FramedUI_Tab;
typedef struct FramedUI_Window FramedUI_Window;
typedef struct FramedUI_TabViewInfo FramedUI_TabViewInfo;

#define FRAMED_UI_TAB_VIEW(name) Void name(struct FramedUI_TabViewInfo *view_info)
typedef FRAMED_UI_TAB_VIEW(FramedUI_TabViewProc);

////////////////////////////////
//~ hampus: Tab types

typedef enum FramedUI_TabReleaseKind FramedUI_TabReleaseKind;
enum FramedUI_TabReleaseKind
{
    FramedUI_TabReleaseKind_Center,
    FramedUI_TabReleaseKind_Left,
    FramedUI_TabReleaseKind_Right,
    FramedUI_TabReleaseKind_Top,
    FramedUI_TabReleaseKind_Bottom,
    FramedUI_TabReleaseKind_COUNT
};

typedef enum FramedUI_DragStatus FramedUI_DragStatus;
enum FramedUI_DragStatus
{
    FramedUI_DragStatus_Inactive,
    FramedUI_DragStatus_Reordering,
    FramedUI_DragStatus_WaitingForDragThreshold,
    FramedUI_DragStatus_Dragging,
    FramedUI_DragStatus_Released,
};

typedef enum FramedUI_TabFlag FramedUI_TabFlag;
enum FramedUI_TabFlag
{
    FramedUI_TabFlag_MarkedForDeletion = (1 << 0),
    FramedUI_TabFlag_Pinned            = (1 << 1),
};

typedef struct FramedUI_TabViewInfo FramedUI_TabViewInfo;
struct FramedUI_TabViewInfo
{
    FramedUI_TabViewProc *function;
    Void *data;
    Arena *arena;
};

typedef struct FramedUI_FreeTab FramedUI_FreeTab;
struct FramedUI_FreeTab
{
    FramedUI_FreeTab *next;
};

typedef struct FramedUI_Tab FramedUI_Tab;
struct FramedUI_Tab
{
    FramedUI_Tab *next;
    FramedUI_Tab *prev;
    FramedUI_Panel *panel;
    Arena *arena;
    Str8 display_string;
    FramedUI_TabFlag flags;
    FramedUI_TabViewInfo view_info;
};

typedef struct FramedUI_TabGroup FramedUI_TabGroup;
struct FramedUI_TabGroup
{
    FramedUI_Tab *active_tab;
    FramedUI_Tab *first;
    FramedUI_Tab *last;
    U64 count;
};

typedef struct FramedUI_DragData FramedUI_DragData;
struct FramedUI_DragData
{
    FramedUI_TabGroup tab_group;
    FramedUI_Panel *hovered_panel;
    Vec2F32 drag_origin;
    Vec2F32 offset;
};

////////////////////////////////
//~ hampus: Panel types

typedef enum FramedUI_PanelFlag FramedUI_PanelFlag;
enum FramedUI_PanelFlag
{
    FramedUI_PanelFlag_MarkedForDeletion = (1 << 0),
};

typedef struct FramedUI_FreePanel FramedUI_FreePanel;
struct FramedUI_FreePanel
{
    FramedUI_FreePanel *next;
};

typedef struct FramedUI_Panel FramedUI_Panel;
struct FramedUI_Panel
{
    FramedUI_Panel *children[Side_COUNT];
    FramedUI_Panel *sibling;
    FramedUI_Panel *parent;
    FramedUI_TabGroup tab_group;
    FramedUI_Window *window;
    FramedUI_PanelFlag flags;
    Axis2 split_axis;
    F32 pct_of_parent;
};

////////////////////////////////
//~ hampus: Window types

typedef struct FramedUI_FreeWindow FramedUI_FreeWindow;
struct FramedUI_FreeWindow
{
    FramedUI_FreeWindow *next;
};

typedef struct FramedUI_Window FramedUI_Window;
struct FramedUI_Window
{
    FramedUI_Window *next;
    FramedUI_Window *prev;
    FramedUI_Panel *root_panel;
    Arena *arena;
    RectF32 rect;
    Str8 string;
};

typedef struct FramedUI_WindowList FramedUI_WindowList;
struct FramedUI_WindowList
{
    FramedUI_Window *first;
    FramedUI_Window *last;
    U64 count;
};

////////////////////////////////
//~ hampus: Command types

typedef enum FramedUI_CommandKind FramedUI_CommandKind;
enum FramedUI_CommandKind
{
    FramedUI_CommandKind_RemoveTab,
    FramedUI_CommandKind_InsertTab,
    FramedUI_CommandKind_CloseTab,
    FramedUI_CommandKind_SplitPanel,
    FramedUI_CommandKind_SplitPanelAndInsertTab,
    FramedUI_CommandKind_SplitPanelAndInsertTabGroup,
    FramedUI_CommandKind_SetTabActive,
    FramedUI_CommandKind_ClosePanel,
    FramedUI_CommandKind_CloseWindow,
    FramedUI_CommandKind_PushWindowToFront,
};

typedef struct FramedUI_CommandParams FramedUI_CommandParams;
struct FramedUI_CommandParams
{
    FramedUI_Window *window;
    FramedUI_Panel *panel;
    FramedUI_Tab *tab;
    FramedUI_TabGroup tab_group;
    Axis2 axis;
    Side side;
    B32 set_active;
};

typedef struct FramedUI_Command FramedUI_Command;
struct FramedUI_Command
{
    FramedUI_CommandKind kind;
    FramedUI_CommandParams params;
};

typedef struct FramedUI_CommandNode FramedUI_CommandNode;
struct FramedUI_CommandNode
{
    FramedUI_CommandNode *next;
    FramedUI_CommandNode *prev;
    FramedUI_Command command;
};

typedef struct FramedUI_CommandList FramedUI_CommandList;
struct FramedUI_CommandList
{
    FramedUI_CommandNode *first;
    FramedUI_CommandNode *last;
};

////////////////////////////////
//~ hampus: Settings types

typedef enum FramedUI_Color FramedUI_Color;
enum FramedUI_Color
{
    FramedUI_Color_PanelBackground,
    FramedUI_Color_PanelBorderActive,
    FramedUI_Color_PanelBorderInactive,
    FramedUI_Color_PanelOverlayInactive,

    FramedUI_Color_TabBarBackground,
    FramedUI_Color_TabBackgroundActive,
    FramedUI_Color_TabBackgroundInactive,
    FramedUI_Color_TabForeground,
    FramedUI_Color_TabBorder,
    FramedUI_Color_TabBarButtonsBackground,

    FramedUI_Color_COUNT,
};

global Str8 framed_ui_string_color_table[] =
{
    [FramedUI_Color_PanelBackground]      = str8_comp("Panel background"),
    [FramedUI_Color_PanelBorderActive]    = str8_comp("Panel border active"),
    [FramedUI_Color_PanelBorderInactive]  = str8_comp("Panel border inactive"),
    [FramedUI_Color_PanelOverlayInactive] = str8_comp("Panel overlay inactive"),

    [FramedUI_Color_TabBarBackground]        = str8_comp("Tab bar background"),
    [FramedUI_Color_TabBarButtonsBackground] = str8_comp("Tab bar buttons background"),
    [FramedUI_Color_TabBackgroundActive]     = str8_comp("Tab background active"),
    [FramedUI_Color_TabBackgroundInactive]   = str8_comp("Tab background inactive"),
    [FramedUI_Color_TabForeground] = str8_comp("Tab foreground"),
    [FramedUI_Color_TabBorder]     = str8_comp("Tab border"),
};

typedef enum FramedUI_FontScale FramedUI_FontScale;
enum FramedUI_FontScale
{
    FramedUI_FontScale_Smaller,
    FramedUI_FontScale_Normal,
    FramedUI_FontScale_Larger,
};

typedef struct FramedUI_Settings FramedUI_Settings;
struct FramedUI_Settings
{
    U32 font_size;
    Vec4F32 theme_colors[FramedUI_Color_COUNT];
};

////////////////////////////////
//~ hampus: State

typedef enum FramedUI_TabView FramedUI_TabView;
enum FramedUI_TabView
{
    FramedUI_TabView_Zones,
    FramedUI_TabView_Settings,
    FramedUI_TabView_About,
    FramedUI_TabView_COUNT,
};

typedef struct FramedUI_State FramedUI_State;
struct FramedUI_State
{
    Arena *perm_arena;
    Arena *frame_arena;

    U64 num_panels;
    U64 num_tabs;
    U64 num_windows;

    FramedUI_WindowList open_windows;

    FramedUI_Window *master_window;
    FramedUI_Window *next_top_most_window;

    FramedUI_Panel *focused_panel;
    FramedUI_Panel *next_focused_panel;

    FramedUI_FreeTab *first_free_tab;
    FramedUI_FreePanel *first_free_panel;
    FramedUI_FreeWindow *first_free_window;

    FramedUI_CommandList cmd_list;

    FramedUI_DragStatus drag_status;
    FramedUI_DragData drag_data;

    FramedUI_Settings settings;

    FramedUI_Tab *tab_view_table[FramedUI_TabView_COUNT];
    FramedUI_TabViewProc *tab_view_function_table[FramedUI_TabView_COUNT];
    Str8 tab_view_string_table[FramedUI_TabView_COUNT];
};

////////////////////////////////
//~ hampus: Tab view

FRAMED_UI_TAB_VIEW(framed_ui_tab_view_default);

////////////////////////////////
//~ hampus: Settings

internal U32 framed_ui_font_size_from_scale(FramedUI_FontScale scale);
internal Vec4F32 framed_ui_color_from_theme(FramedUI_Color color);
internal Void framed_ui_set_color(FramedUI_Color color, Vec4F32 value);

////////////////////////////////
//~ hampus: Command

internal Void framed_ui_command_push(FramedUI_CommandKind kind, FramedUI_CommandParams params);

////////////////////////////////
//~ hampus: Tab dragging

internal Void framed_ui_drag_begin_reordering(FramedUI_Tab *tab, B32 drag_whole_tab_group, Vec2F32 offset);
internal Void framed_ui_wait_for_drag_threshold(Void);
internal Void framed_ui_drag_release(Void);
internal Void framed_ui_drag_end(Void);
internal B32 framed_ui_is_dragging(Void);
internal B32 framed_ui_is_tab_reordering(Void);
internal B32 framed_ui_is_waiting_for_drag_threshold(Void);
internal B32 framed_ui_drag_is_inactive(Void);

////////////////////////////////
//~ hampus: Tab

internal B32 framed_ui_tab_has_flag(FramedUI_Tab *tab, FramedUI_TabFlag flag);
internal B32 framed_ui_tab_is_nil(FramedUI_Tab *tab);
internal FramedUI_Tab *framed_ui_tab_alloc(Void);
internal Void framed_ui_tab_free(FramedUI_Tab *tab);
internal Void framed_ui_tab_equip_view_info(FramedUI_Tab *tab, FramedUI_TabViewInfo view_info);
internal FramedUI_Tab *framed_ui_tab_make(FramedUI_TabViewProc *function, Void *data, Str8 display_string);
internal B32 framed_ui_tab_is_active(FramedUI_Tab *tab);
internal B32 framed_ui_tab_is_dragged(FramedUI_Tab *tab);
internal UI_Box *framed_ui_tab_button(FramedUI_Tab *tab);

////////////////////////////////
//~ hampus: Panel

internal Void framed_ui_panel_set_active_tab(FramedUI_Panel *panel, FramedUI_Tab *tab);
internal Void framed_ui_panel_insert_tab(FramedUI_Panel *panel, FramedUI_Tab *tab);
internal Void framed_ui_panel_remove_tab(FramedUI_Tab *tab);
internal Void framed_ui_panel_split(FramedUI_Panel *panel, Axis2 split_axis);
internal Void framed_ui_panel_close(FramedUI_Panel *panel);
internal B32 framed_ui_panel_is_nil(FramedUI_Panel *panel);
internal FramedUI_Panel *framed_ui_panel_alloc(Void);
internal Void framed_ui_panel_free(FramedUI_Panel *panel);
internal Void framed_ui_panel_free_recursively(FramedUI_Panel *root);
internal FramedUI_Panel *framed_ui_panel_make(Void);
internal Side framed_ui_panel_get_side(FramedUI_Panel *panel);
internal B32 framed_ui_panel_is_leaf(FramedUI_Panel *panel);
internal UI_Comm framed_ui_panel_hover_type(Str8 string, F32 width_in_em, FramedUI_Panel *root, Axis2 axis, B32 center, Side side);
internal Void framed_ui_panel_update(FramedUI_Panel *root);

////////////////////////////////
//~ hampus: Window

internal Void framed_ui_window_push_to_front(FramedUI_Window *window);
internal Void framed_ui_window_close(FramedUI_Window *window);
internal Void framed_ui_window_set_pos(FramedUI_Window *window, Vec2F32 pos);
internal FramedUI_Window *framed_ui_window_alloc(Void);
internal Void framed_ui_window_free(FramedUI_Window *window);
internal FramedUI_Window *framed_ui_window_make(Vec2F32 min, Vec2F32 max);
internal UI_Comm framed_ui_window_edge_resizer(FramedUI_Window *window, Str8 string, Axis2 axis, Side side, F32 size_in_em);
internal UI_Comm framed_ui_window_corner_resizer(FramedUI_Window *window, Str8 string, Corner corner, F32 size_in_em);
internal Void framed_ui_window_update(FramedUI_Window *window);

////////////////////////////////
//~ hampus: Tab views

#define framed_ui_get_view_data(view_info, type) framed_ui_get_or_push_view_data_(view_info, sizeof(type))
internal Void *framed_ui_get_or_push_view_data_(FramedUI_TabViewInfo *view_info, U64 size);

////////////////////////////////
//~ hampus: Main update

internal Void framed_ui_update(Render_Context *renderer, Gfx_EventList *event_list);

#endif
