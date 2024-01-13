#ifndef UI_PANEL_TEST_H
#define UI_PANEL_TEST_H

typedef enum CmdKind CmdKind;
enum CmdKind
{
	CmdKind_TabAttach,
	CmdKind_TabClose,

	CmdKind_PanelSplit,
	CmdKind_PanelSplitAndAttach,
	CmdKind_PanelSetActiveTab,
	CmdKind_PanelClose,

	CmdKind_WindowRemoveFromList,
	CmdKind_WindowPushToFront,
};

typedef enum TabReleaseKind TabReleaseKind;
enum TabReleaseKind
{
	TabReleaseKind_Center,

	TabReleaseKind_Left,
	TabReleaseKind_Right,

	TabReleaseKind_Top,
	TabReleaseKind_Bottom,

	TabReleaseKind_COUNT
};

typedef struct Panel Panel;
typedef struct Cmd Cmd;
typedef struct Window Window;

typedef struct SplitPanelResult SplitPanelResult;
struct SplitPanelResult
{
	Panel *panels[Side_COUNT];
};

#define UI_TAB_VIEW(name) Void name(Void *data)
typedef UI_TAB_VIEW(UI_TabViewProc);

typedef struct TabViewInfo TabViewInfo;
struct TabViewInfo
{
	UI_TabViewProc *function;
	Void *data;
};

typedef struct Tab Tab;
struct Tab
{
	Tab *next;
	Tab *prev;

	Str8 string;
	B32 pinned;

	Panel *panel;

	UI_Box *box;

	TabViewInfo view_info;

	// NOTE(hampus): For debugging
	U64 frame_index;
};

typedef struct TabGroup TabGroup;
struct TabGroup
{
	U64 count;
	Tab *active_tab;
	Tab *first;
	Tab *last;
};

typedef struct Panel Panel;
struct Panel
{
	Panel *children[Side_COUNT];
	Panel *sibling;
	Panel *parent;
	Window *window;

	TabGroup tab_group;

	Axis2 split_axis;
	F32 pct_of_parent;

	Str8 string;

	UI_Box *box;

	// NOTE(hampus): For debugging
	UI_Box *dragger;
	U64 frame_index;
};

typedef struct Window Window;
struct Window
{
	Window *next;
	Window *prev;

	Vec2F32 pos;
	Vec2F32 size;

	Panel *root_panel;

	B32 dragging;

	Str8 string;
};

typedef struct WindowList WindowList;
struct WindowList
{
	Window *first;
	Window *last;
	U64 count;
};

typedef struct TabDelete TabDelete;
struct TabDelete
{
	Tab *tab;
};

typedef struct TabAttach TabAttach;
struct TabAttach
{
	Tab   *tab;
	Panel *panel;
	B32    set_active;
};

typedef struct PanelSplit PanelSplit;
struct PanelSplit
{
	Panel *panel;
	Axis2  axis;
	B32 alloc_new_tab;
	TabViewInfo tab_view_info;
};

typedef struct PanelSplitAndAttach PanelSplitAndAttach;
struct PanelSplitAndAttach
{
	Panel *panel;
	Tab *tab;
	Axis2 axis;
	Side panel_side;  // Which side to put this panel on
};

typedef struct PanelSetActiveTab PanelSetActiveTab;
struct PanelSetActiveTab
{
	Panel *panel;
	Tab *tab;
};

typedef struct PanelClose PanelClose;
struct PanelClose
{
	Panel *panel;
};

typedef struct WindowRemoveFromList WindowRemoveFromList;
struct WindowRemoveFromList
{
	Window *window;
};

typedef struct WindowPushToFront WindowPushToFront;
struct WindowPushToFront
{
	Window *window;
};

typedef struct Cmd Cmd;
struct Cmd
{
	CmdKind kind;
	U8 data[512];
};

#define CMD_BUFFER_SIZE 16
typedef struct CmdBuffer CmdBuffer;
struct CmdBuffer
{
	U64 pos;
	U64 size;
	Cmd *buffer;
};

typedef enum DragStatus DragStatus;
enum DragStatus
{
	DragStatus_Inactive,
	DragStatus_WaitingForDragThreshold,
	DragStatus_Dragging,
	DragStatus_Released,
};

typedef struct DragData DragData;
struct DragData
{
	Tab    *tab;
	Panel  *hovered_panel;
	Vec2F32 drag_origin;
};

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
	U64 num_tabs;
	U64 num_windows;

	UI_Box *window_container;
	Window *master_window;
	Window *next_top_most_window;

	Panel *focused_panel;
	Panel *next_focused_panel;

	CmdBuffer cmd_buffer;

	WindowList window_list;

	// NOTE(hampus): Dragging

	DragStatus drag_status;
	DragData   drag_data;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

////////////////////////////////
//~ hampus: Panels

internal Void ui_attach_tab_to_panel(Panel *panel, Tab *tab, B32 set_active);

////////////////////////////////
//~ hampus: Window

internal Void ui_window_reorder_to_front(Window *window);
internal Void ui_window_push_to_front(Window *window);
internal Void ui_window_remove_from_list(Window *window);

#define UI_COMMAND(name) Void ui_command_##name(Void *params)
UI_COMMAND(tab_close);

#endif
