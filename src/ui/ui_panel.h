#ifndef UI_PANEL_TEST_H
#define UI_PANEL_TEST_H

#define UI_COMMAND(name) Void ui_command_##name(Void *params)


typedef enum UI_CommandKind UI_CommandKind;
enum UI_CommandKind
{
	UI_CommandKind_TabAttach,
	UI_CommandKind_TabClose,

	UI_CommandKind_PanelSplit,
	UI_CommandKind_PanelSplitAndAttach,
	UI_CommandKind_PanelSetActiveTab,
	UI_CommandKind_PanelClose,

	UI_CommandKind_WindowRemoveFromList,
	UI_CommandKind_WindowPushToFront,
};

typedef enum UI_TabReleaseKind UI_TabReleaseKind;
enum UI_TabReleaseKind
{
	UI_TabReleaseKind_Center,
	UI_TabReleaseKind_Left,
	UI_TabReleaseKind_Right,
	UI_TabReleaseKind_Top,
	UI_TabReleaseKind_Bottom,

	UI_TabReleaseKind_COUNT
};

typedef enum UI_DragStatus UI_DragStatus;
enum UI_DragStatus
{
	UI_DragStatus_Inactive,
	UI_DragStatus_WaitingForDragThreshold,
	UI_DragStatus_Dragging,
	UI_DragStatus_Released,
};

typedef struct UI_Panel UI_Panel;
typedef struct UI_Tab UI_Tab;
typedef struct UI_Cmd UI_Cmd;
typedef struct UI_Window UI_Window;

typedef struct UI_SplitPanelResult UI_SplitPanelResult;
struct UI_SplitPanelResult
{
	UI_Panel *panels[Side_COUNT];
};

#define UI_TAB_VIEW(name) Void name(Void *data)
typedef UI_TAB_VIEW(UI_TabViewProc);

typedef struct UI_TabViewInfo UI_TabViewInfo;
struct UI_TabViewInfo
{
	UI_TabViewProc *function;
	Void *data;
};

typedef struct UI_DragData UI_DragData;
struct UI_DragData
{
	UI_Tab    *tab;
	UI_Panel  *hovered_panel;
	Vec2F32 drag_origin;
};

typedef struct UI_Tab UI_Tab;
struct UI_Tab
{
	UI_Tab *next;
	UI_Tab *prev;

	Str8 string;
	B32 pinned;

	UI_Panel *panel;

	UI_Box *box;

	UI_TabViewInfo view_info;

	// NOTE(hampus): For debugging
	U64 frame_index;
};

typedef struct UI_TabGroup UI_TabGroup;
struct UI_TabGroup
{
	U64 count;
	UI_Tab *active_tab;
	UI_Tab *first;
	UI_Tab *last;
	F32 view_offset_x;
};

typedef struct UI_Panel UI_Panel;
struct UI_Panel
{
	UI_Panel *children[Side_COUNT];
	UI_Panel *sibling;
	UI_Panel *parent;
	UI_Window *window;

	UI_TabGroup tab_group;

	Axis2 split_axis;
	F32 pct_of_parent;

	Str8 string;

	UI_Box *box;

	// NOTE(hampus): For debugging
	UI_Box *dragger;
	U64 frame_index;
};

typedef struct UI_Window UI_Window;
struct UI_Window
{
	UI_Window *next;
	UI_Window *prev;

	Vec2F32 pos;
	Vec2F32 size;

	UI_Panel *root_panel;

	UI_Box *box;

	Str8 string;
};

typedef struct UI_WindowList UI_WindowList;
struct UI_WindowList
{
	UI_Window *first;
	UI_Window *last;
	U64 count;
};

typedef struct UI_TabDelete UI_TabDelete;
struct UI_TabDelete
{
	UI_Tab *tab;
};

typedef struct UI_TabAttach UI_TabAttach;
struct UI_TabAttach
{
	UI_Tab   *tab;
	UI_Panel *panel;
	B32    set_active;
};

typedef struct UI_PanelSplit UI_PanelSplit;
struct UI_PanelSplit
{
	UI_Panel *panel;
	Axis2  axis;
	UI_TabViewInfo tab_view_info;
};

typedef struct UI_PanelSplitAndAttach UI_PanelSplitAndAttach;
struct UI_PanelSplitAndAttach
{
	UI_Panel *panel;
	UI_Tab *tab;
	Axis2 axis;
	Side panel_side;  // Which side to put this panel on
};

typedef struct UI_PanelSetActiveTab UI_PanelSetActiveTab;
struct UI_PanelSetActiveTab
{
	UI_Panel *panel;
	UI_Tab *tab;
};

typedef struct UI_PanelClose UI_PanelClose;
struct UI_PanelClose
{
	UI_Panel *panel;
};

typedef struct UI_WindowRemoveFromList UI_WindowRemoveFromList;
struct UI_WindowRemoveFromList
{
	UI_Window *window;
};

typedef struct UI_WindowPushToFront UI_WindowPushToFront;
struct UI_WindowPushToFront
{
	UI_Window *window;
};

typedef struct UI_Cmd UI_Cmd;
struct UI_Cmd
{
	UI_CommandKind kind;
	U8 data[512];
};

#define CMD_BUFFER_SIZE 16
typedef struct UI_CommandBuffer UI_CommandBuffer;
struct UI_CommandBuffer
{
	U64 pos;
	U64 size;
	UI_Cmd *buffer;
};

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
	U64 num_tabs;
	U64 num_windows;

	UI_Box *window_container;
	UI_Window *master_window;
	UI_Window *next_top_most_window;

	UI_Panel *focused_panel;
	UI_Panel *next_focused_panel;

	UI_Panel *resizing_panel;

	UI_CommandBuffer cmd_buffer;

	UI_WindowList window_list;

	UI_DragStatus drag_status;
	UI_DragData   drag_data;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

UI_COMMAND(tab_close);

////////////////////////////////
//~ hampus: UI_Panels

internal Void ui_attach_tab_to_panel(UI_Panel *panel, UI_Tab *tab, B32 set_active);

////////////////////////////////
//~ hampus: Window

internal Void ui_window_reorder_to_front(UI_Window *window);
internal Void ui_window_push_to_front(UI_Window *window);
internal Void ui_window_remove_from_list(UI_Window *window);

#endif
