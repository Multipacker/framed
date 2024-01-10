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
typedef struct Window Window ;

typedef struct SplitPanelResult SplitPanelResult;
struct SplitPanelResult
{
	Panel *panels[Side_COUNT];
};

typedef struct Tab Tab;
struct Tab
{
	Tab *next;
	Tab *prev;

	Str8 string;
	B32 pinned;

	Panel *panel;

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

typedef struct AppState AppState;
struct AppState
{
	Arena *perm_arena;
	U64 num_panels;
	U64 num_tabs;
	U64 num_windows;

	UI_Box *root_window;
	Window *master_window;
	
	Panel *focused_panel;
	Panel *next_focused_panel;
	
	CmdBuffer cmd_buffer;

	WindowList window_list;
	
	Window *top_most_window_next_frame;
	
	// NOTE(hampus): Dragging stuff
	Vec2F32 start_drag_pos;
	Vec2F32 current_drag_pos;
	Tab *drag_candidate;
	Tab *drag_tab;
	Panel *hovering_panel;
	Vec2F32 new_window_pct;
	B8 tab_released;
	B8 create_new_window;

	// NOTE(hampus): Debug purposes
	U64 frame_index;
};

////////////////////////////////
//~ hampus: Tab dragging

internal Void ui_end_drag(Void);

////////////////////////////////
//~ hampus: Panels

internal Void ui_attach_tab_to_panel(Panel *panel, Tab *tab, B32 set_active);

////////////////////////////////
//~ hampus: Window

internal Void ui_window_reorder_to_front(Window *window);
internal Void ui_window_push_to_front(Window *window);
internal Void ui_window_remove_from_list(Window *window);


#endif
