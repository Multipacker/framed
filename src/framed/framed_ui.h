#ifndef UI_PANEL_TEST_H
#define UI_PANEL_TEST_H

#define FRAMED_UI_COMMAND(name) Void framed_ui_command_##name(Void *params)

#define FRAME_UI_TAB_VIEW(name) Void name(struct FramedUI_TabViewInfo *view_info)

typedef struct FramedUI_Panel FramedUI_Panel;
typedef struct FramedUI_Tab FramedUI_Tab;
typedef struct FramedUI_Window FramedUI_Window;
typedef struct FramedUI_TabViewInfo FramedUI_TabViewInfo;

typedef FRAME_UI_TAB_VIEW(FramedUI_TabViewProc);

typedef enum FramedUI_CommandKind FramedUI_CommandKind;
enum FramedUI_CommandKind
{
	FramedUI_CommandKind_TabDeattach,
	FramedUI_CommandKind_TabAttach,
	FramedUI_CommandKind_TabClose,
	FramedUI_CommandKind_TabSwap,
	FramedUI_CommandKind_PanelSplit,
	FramedUI_CommandKind_PanelSplitAndAttach,
	FramedUI_CommandKind_PanelSetActiveTab,
	FramedUI_CommandKind_PanelClose,
	FramedUI_CommandKind_WindowClose,
	FramedUI_CommandKind_WindowSetTopMost,
};

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

typedef enum FramedUI_Color FramedUI_Color;
enum FramedUI_Color
{
	FramedUI_Color_Panel,
	FramedUI_Color_InactivePanelBorder,
	FramedUI_Color_ActivePanelBorder,
	FramedUI_Color_InactivePanelOverlay,

	FramedUI_Color_TabBar,
	FramedUI_Color_ActiveTab,
	FramedUI_Color_InactiveTab,
	FramedUI_Color_TabTitle,
	FramedUI_Color_TabBorder,

	FramedUI_Color_TabBarButtons,

	FramedUI_Color_COUNT,
};

typedef struct FramedUI_Theme FramedUI_Theme;
struct FramedUI_Theme
{
	Vec4F32 colors[FramedUI_Color_COUNT];
};

typedef struct FramedUI_SplitPanelResult FramedUI_SplitPanelResult;
struct FramedUI_SplitPanelResult
{
	FramedUI_Panel *panels[Side_COUNT];
};

typedef struct FramedUI_TabViewInfo FramedUI_TabViewInfo;
struct FramedUI_TabViewInfo
{
	FramedUI_TabViewProc *function;
	Void *data;
};

typedef struct FramedUI_DragData FramedUI_DragData;
struct FramedUI_DragData
{
	FramedUI_Tab   *tab;
	FramedUI_Panel *hovered_panel;
	Vec2F32 drag_origin;
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

	Str8 string;
	B32 pinned;

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

	Axis2 split_axis;
	F32 pct_of_parent;

	Str8 string;
};

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

typedef struct FramedUI_Command FramedUI_Command;
struct FramedUI_Command
{
	FramedUI_CommandKind kind;
	U8 data[256];
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
	FramedUI_DragData   drag_data;

	FramedUI_Theme theme;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

typedef struct FramedUI_TabDelete FramedUI_TabDelete;
struct FramedUI_TabDelete
{
	FramedUI_Tab *tab;
};

typedef struct FramedUI_TabAttach FramedUI_TabAttach;
struct FramedUI_TabAttach
{
	FramedUI_Tab   *tab;
	FramedUI_Panel *panel;
	B32    set_active;
};

typedef struct FramedUI_TabSwap FramedUI_TabSwap;
struct FramedUI_TabSwap
{
	FramedUI_Tab *tab0;
	FramedUI_Tab *tab1;
};

typedef struct FramedUI_PanelSplit FramedUI_PanelSplit;
struct FramedUI_PanelSplit
{
	FramedUI_Panel *panel;
	Axis2  axis;
	FramedUI_TabViewInfo tab_view_info;
};

typedef struct FramedUI_PanelSplitAndAttach FramedUI_PanelSplitAndAttach;
struct FramedUI_PanelSplitAndAttach
{
	FramedUI_Panel *panel;
	FramedUI_Tab *tab;
	Axis2 axis;
	Side panel_side;  // Which side to put this panel on
};

typedef struct FramedUI_PanelSetActiveTab FramedUI_PanelSetActiveTab;
struct FramedUI_PanelSetActiveTab
{
	FramedUI_Panel *panel;
	FramedUI_Tab *tab;
};

typedef struct FramedUI_PanelClose FramedUI_PanelClose;
struct FramedUI_PanelClose
{
	FramedUI_Panel *panel;
};

typedef struct FramedUI_WindowClose FramedUI_WindowClose;
struct FramedUI_WindowClose
{
	FramedUI_Window *window;
};

typedef struct FramedUI_WindowSetTopMost FramedUI_WindowSetTopMost;
struct FramedUI_WindowSetTopMost
{
	FramedUI_Window *window;
};

FRAMED_UI_COMMAND(tab_close);
FRAMED_UI_COMMAND(window_set_top_most);

////////////////////////////////
// hampus: Command helpers

internal Void framed_ui_tab_close(FramedUI_Tab *tab);

internal Void framed_ui_panel_attach_tab(FramedUI_Panel *panel, FramedUI_Tab *tab, B32 set_active);
internal Void framed_ui_panel_split(FramedUI_Panel *first, Axis2 split_axis);
internal Void framed_ui_panel_split_and_attach_tab(FramedUI_Panel *panel, FramedUI_Tab *tab, Axis2 axis, Side side);

////////////////////////////////
// hampus: Panels

internal Void framed_ui_panel_split_and_attach_tab(FramedUI_Panel *panel, FramedUI_Tab *tab, Axis2 axis, Side side);
internal Void framed_ui_panel_free(FramedUI_Panel *panel);

////////////////////////////////
// hampus: Window

internal Void framed_ui_window_free(FramedUI_Window *window);
internal Void framed_ui_window_set_top_most(FramedUI_Window *window);
internal Void framed_ui_window_close(FramedUI_Window *window);

#endif
