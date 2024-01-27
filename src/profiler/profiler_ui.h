#ifndef UI_PANEL_TEST_H
#define UI_PANEL_TEST_H

#define PROFILER_UI_COMMAND(name) Void profiler_ui_command_##name(Void *params)

#define PROFILER_UI_TAB_VIEW(name) Void name(struct ProfilerUI_TabViewInfo *view_info)

typedef struct ProfilerUI_Panel ProfilerUI_Panel;
typedef struct ProfilerUI_Tab ProfilerUI_Tab;
typedef struct ProfilerUI_Command ProfilerUI_Commands;
typedef struct ProfilerUI_Window ProfilerUI_Window;
typedef struct ProfilerUI_TabViewInfo ProfilerUI_TabViewInfo;

typedef PROFILER_UI_TAB_VIEW(ProfilerUI_TabViewProc);

typedef enum ProfilerUI_CommandKind ProfilerUI_CommandKind;
enum ProfilerUI_CommandKind
{
	ProfilerUI_CommandKind_TabAttach,
	ProfilerUI_CommandKind_TabClose,
	ProfilerUI_CommandKind_TabReorder,
	ProfilerUI_CommandKind_PanelSplit,
	ProfilerUI_CommandKind_PanelSplitAndAttach,
	ProfilerUI_CommandKind_PanelSetActiveTab,
	ProfilerUI_CommandKind_PanelClose,
	ProfilerUI_CommandKind_WindowRemoveFromList,
	ProfilerUI_CommandKind_WindowPushToFront,
};

typedef enum ProfilerUI_TabReleaseKind ProfilerUI_TabReleaseKind;
enum ProfilerUI_TabReleaseKind
{
	ProfilerUI_TabReleaseKind_Center,
	ProfilerUI_TabReleaseKind_Left,
	ProfilerUI_TabReleaseKind_Right,
	ProfilerUI_TabReleaseKind_Top,
	ProfilerUI_TabReleaseKind_Bottom,
	ProfilerUI_TabReleaseKind_COUNT
};

typedef enum ProfilerUI_DragStatus ProfilerUI_DragStatus;
enum ProfilerUI_DragStatus
{
	ProfilerUI_DragStatus_Inactive,
	ProfilerUI_DragStatus_Reordering,
	ProfilerUI_DragStatus_WaitingForDragThreshold,
	ProfilerUI_DragStatus_Dragging,
	ProfilerUI_DragStatus_Released,
};

typedef enum ProfilerUI_Color ProfilerUI_Color;
enum ProfilerUI_Color
{
	ProfilerUI_Color_Panel,
	ProfilerUI_Color_InactivePanelBorder,
	ProfilerUI_Color_ActivePanelBorder,
	ProfilerUI_Color_InactivePanelOverlay,

	ProfilerUI_Color_TabBar,
	ProfilerUI_Color_ActiveTab,
	ProfilerUI_Color_InactiveTab,
	ProfilerUI_Color_TabTitle,
	ProfilerUI_Color_TabBorder,

	ProfilerUI_Color_TabBarButtons,

	ProfilerUI_Color_COUNT,
};

typedef struct ProfilerUI_Theme ProfilerUI_Theme;
struct ProfilerUI_Theme
{
	Vec4F32 colors[ProfilerUI_Color_COUNT];
};

typedef struct ProfilerUI_SplitPanelResult ProfilerUI_SplitPanelResult;
struct ProfilerUI_SplitPanelResult
{
	ProfilerUI_Panel *panels[Side_COUNT];
};

typedef struct ProfilerUI_TabViewInfo ProfilerUI_TabViewInfo;
struct ProfilerUI_TabViewInfo
{
	ProfilerUI_TabViewProc *function;
	Void *data;
};

typedef struct ProfilerUI_DragData ProfilerUI_DragData;
struct ProfilerUI_DragData
{
	ProfilerUI_Tab    *tab;
	ProfilerUI_Panel  *hovered_panel;
	Vec2F32   drag_origin;
};

typedef struct ProfilerUI_Tab ProfilerUI_Tab;
struct ProfilerUI_Tab
{
	ProfilerUI_Tab *next;
	ProfilerUI_Tab *prev;

	ProfilerUI_Panel *panel;

	Str8 string;
	B32 pinned;

	UI_Box *tab_container;
	UI_Box *tab_box;

	ProfilerUI_TabViewInfo view_info;

	// NOTE(hampus): For debugging
	U64 frame_index;
};

typedef struct ProfilerUI_TabGroup ProfilerUI_TabGroup;
struct ProfilerUI_TabGroup
{
	ProfilerUI_Tab *active_tab;
	ProfilerUI_Tab *first;
	ProfilerUI_Tab *last;
	U64 count;
};

typedef struct ProfilerUI_Panel ProfilerUI_Panel;
struct ProfilerUI_Panel
{
	ProfilerUI_Panel *children[Side_COUNT];
	ProfilerUI_Panel *sibling;
	ProfilerUI_Panel *parent;

	ProfilerUI_TabGroup tab_group;

	ProfilerUI_Window *window;

	UI_Box *box;

	Axis2 split_axis;
	F32 pct_of_parent;

	Str8 string;

	// NOTE(hampus): For debugging
	UI_Box *dragger;
	U64 frame_index;
};

typedef struct ProfilerUI_Window ProfilerUI_Window;
struct ProfilerUI_Window
{
	ProfilerUI_Window *next;
	ProfilerUI_Window *prev;

	Vec2F32 pos;
	Vec2F32 size;

	ProfilerUI_Panel *root_panel;

	UI_Box *box;

	Str8 string;
};

typedef struct ProfilerUI_WindowList ProfilerUI_WindowList;
struct ProfilerUI_WindowList
{
	ProfilerUI_Window *first;
	ProfilerUI_Window *last;
	U64 count;
};

typedef struct ProfilerUI_Command ProfilerUI_Command;
struct ProfilerUI_Command
{
	ProfilerUI_CommandKind kind;
	U8 data[512];
};

#define CMD_BUFFER_SIZE 16
typedef struct ProfilerUI_CommandBuffer ProfilerUI_CommandBuffer;
struct ProfilerUI_CommandBuffer
{
	U64 pos;
	U64 size;
	ProfilerUI_Command *buffer;
};

typedef struct ProfilerUI_State ProfilerUI_State;
struct ProfilerUI_State
{
	Arena *perm_arena;
	U64 num_panels;
	U64 num_tabs;
	U64 num_windows;

	UI_Box *window_container;
	ProfilerUI_Window *master_window;
	ProfilerUI_Window *next_top_most_window;

	ProfilerUI_Panel *focused_panel;
	ProfilerUI_Panel *next_focused_panel;

	ProfilerUI_CommandBuffer cmd_buffer;

	ProfilerUI_WindowList window_list;

	ProfilerUI_DragStatus drag_status;
	ProfilerUI_DragData   drag_data;

	ProfilerUI_Theme theme;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

typedef struct ProfilerUI_TabDelete ProfilerUI_TabDelete;
struct ProfilerUI_TabDelete
{
	ProfilerUI_Tab *tab;
};

typedef struct ProfilerUI_TabAttach ProfilerUI_TabAttach;
struct ProfilerUI_TabAttach
{
	ProfilerUI_Tab   *tab;
	ProfilerUI_Panel *panel;
	B32    set_active;
};

typedef struct ProfilerUI_TabReorder ProfilerUI_TabReorder;
struct ProfilerUI_TabReorder
{
	ProfilerUI_Tab *tab;
	ProfilerUI_Tab *next;
	Side side;
};

typedef struct ProfilerUI_PanelSplit ProfilerUI_PanelSplit;
struct ProfilerUI_PanelSplit
{
	ProfilerUI_Panel *panel;
	Axis2  axis;
	ProfilerUI_TabViewInfo tab_view_info;
};

typedef struct ProfilerUI_PanelSplitAndAttach ProfilerUI_PanelSplitAndAttach;
struct ProfilerUI_PanelSplitAndAttach
{
	ProfilerUI_Panel *panel;
	ProfilerUI_Tab *tab;
	Axis2 axis;
	Side panel_side;  // Which side to put this panel on
};

typedef struct ProfilerUI_PanelSetActiveTab ProfilerUI_PanelSetActiveTab;
struct ProfilerUI_PanelSetActiveTab
{
	ProfilerUI_Panel *panel;
	ProfilerUI_Tab *tab;
};

typedef struct ProfilerUI_PanelClose ProfilerUI_PanelClose;
struct ProfilerUI_PanelClose
{
	ProfilerUI_Panel *panel;
};

typedef struct ProfilerUI_WindowRemoveFromList ProfilerUI_WindowRemoveFromList;
struct ProfilerUI_WindowRemoveFromList
{
	ProfilerUI_Window *window;
};

typedef struct ProfilerUI_WindowPushToFront ProfilerUI_WindowPushToFront;
struct ProfilerUI_WindowPushToFront
{
	ProfilerUI_Window *window;
};

PROFILER_UI_COMMAND(tab_close);

////////////////////////////////
//~ hampus: Command helpers

internal Void profiler_ui_tab_close(ProfilerUI_Tab *tab);

internal Void profiler_ui_panel_attach_tab(ProfilerUI_Panel *panel, ProfilerUI_Tab *tab, B32 set_active);
internal Void profiler_ui_panel_split(ProfilerUI_Panel *first, Axis2 split_axis);
internal Void profiler_ui_panel_split_and_attach_tab(ProfilerUI_Panel *panel, ProfilerUI_Tab *tab, Axis2 axis, Side side);

////////////////////////////////
//~ hampus: Panels

internal Void profiler_ui_panel_split_and_attach_tab(ProfilerUI_Panel *panel, ProfilerUI_Tab *tab, Axis2 axis, Side side);

////////////////////////////////
//~ hampus: Window

internal Void profiler_ui_window_reorder_to_front(ProfilerUI_Window *window);
internal Void profiler_ui_window_push_to_front(ProfilerUI_Window *window);
internal Void profiler_ui_window_remove_from_list(ProfilerUI_Window *window);

#endif
