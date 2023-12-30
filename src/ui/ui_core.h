#ifndef UI_CORE_H
#define UI_CORE_H

typedef enum UI_BoxFlags UI_BoxFlags;
enum UI_BoxFlags
{
	UI_BoxFlag_Clickable       = (1 << 0),
	UI_BoxFlag_DrawText        = (1 << 1),
	UI_BoxFlag_DrawBorder      = (1 << 2),
	UI_BoxFlag_DrawBackground  = (1 << 3),
	UI_BoxFlag_DrawDropShadow  = (1 << 4),
	UI_BoxFlag_HotAnimation    = (1 << 5),
	UI_BoxFlag_ActiveAnimation = (1 << 6),
	UI_BoxFlag_FocusAnimation  = (1 << 7),

	// NOTE(hampus): This decides if the box
	// should apply a scroll value to its children
	// TODO(hampus): Implement this
	UI_BoxFlag_ViewScroll      = (1 << 8),

	// NOTE(hampus): This decides if the children
	// of the box are allowed to go outside of
	// the parent region
	// TODO(hampus): Implement this
	UI_BoxFlag_OverflowX       = (1 << 9),
	UI_BoxFlag_OverflowY       = (1 << 10),

	// NOTE(hampus): This decides if the box's
	// rect should be pushed as a clip rect
	// when it is pushed to the parent stack
	// TODO(hampus): Implement this
	UI_BoxFlag_Clip            = (1 << 11),

	// NOTE(hampus): These makes the ui auto-layouting
	// algorithm skip this box. Useful if you want
	// the box to have an absolute position.
	//
	// Set the position with ui_push/next_relative_pos
	// It will be relative to the parent.
	// TODO(hampus): Implement this
	UI_BoxFlag_FloatingX       = (1 << 12),
	UI_BoxFlag_FloatingY       = (1 << 13),

	// TODO(hampus): Implement this
	UI_BoxFlag_AnimateX        = (1 << 14),
	UI_BoxFlag_AnimateY        = (1 << 15),

	// TODO(hampus): Implement this
	UI_BoxFlag_AnimateWidth    = (1 << 16),
	UI_BoxFlag_AnimateHeight   = (1 << 17),

	UI_BoxFlag_FloatingPos     = UI_BoxFlag_FloatingX | UI_BoxFlag_FloatingY,
	UI_BoxFlag_AnimatePos      = UI_BoxFlag_AnimateX | UI_BoxFlag_AnimateY,
	UI_BoxFlag_AnimateDim      = UI_BoxFlag_AnimateWidth | UI_BoxFlag_AnimateHeight,
};

typedef enum UI_SizeKind UI_SizeKind;
enum UI_SizeKind
{
	UI_SizeKind_Null,
	UI_SizeKind_Pixels,
	UI_SizeKind_TextContent,
	UI_SizeKind_Pct,
	UI_SizeKind_ChildrenSum,
};

typedef struct UI_Size UI_Size;
struct UI_Size
{
	UI_SizeKind kind;
	F32 value;
	F32 strictness;
};

typedef struct UI_Key UI_Key;
struct UI_Key
{
	U64 value;
};

typedef enum UI_TextAlign UI_TextAlign;
enum UI_TextAlign
{
	UI_TextAlign_Center,
	UI_TextAlign_Left,
	UI_TextAlign_Right,

	UI_TextAlign_COUNT,
};

typedef struct UI_RectStyle UI_RectStyle;
struct UI_RectStyle
{
	UI_RectStyle *stack_next;

	Vec4F32 color[4];
	Vec4F32 border_color;
	F32     border_thickness;
	Vec4F32 radies;
	F32     softness;
};

typedef struct UI_TextStyle UI_TextStyle;
struct UI_TextStyle
{
	UI_TextStyle *stack_next;

	Vec4F32      color;
	UI_TextAlign align;
	F32          padding[Axis2_COUNT];
	U64          icon;
	R_FontKey    font;
};

typedef struct UI_LayoutStyle UI_LayoutStyle;
struct UI_LayoutStyle
{
	UI_LayoutStyle *stack_next;

	UI_Size     size[Axis2_COUNT];
	F32         relative_pos[Axis2_COUNT];
	Axis2       child_layout_axis;
	UI_BoxFlags box_flags;
};

typedef struct UI_RectStyleStack UI_RectStyleStack;
struct UI_RectStyleStack
{
	UI_RectStyle *first;
	B32 auto_pop;
};

typedef struct UI_TextStyleStack UI_TextStyleStack;
struct UI_TextStyleStack
{
	UI_TextStyle *first;
	B32 auto_pop;
};

typedef struct UI_LayoutStyleStack UI_LayoutStyleStack;
struct UI_LayoutStyleStack
{
	UI_LayoutStyle *first;
	B32 auto_pop;
};

typedef struct UI_Box UI_Box;
struct UI_Box
{
	UI_Box *first;
	UI_Box *last;
	UI_Box *next;
	UI_Box *prev;
	UI_Box *parent;

	UI_Box *hash_next;
	UI_Box *hash_prev;

	UI_Key key;
	U64    last_frame_touched_index;

	F32 computed_rel_position[Axis2_COUNT];
	F32 computed_size[Axis2_COUNT];
	RectF32 rect;

	UI_BoxFlags flags;
	Str8        string;
#if !BUILD_MODE_RELEASE
	Str8        debug_string;
#endif

	UI_RectStyle   rect_style;
	UI_TextStyle   text_style;
	UI_LayoutStyle layout_style;

	F32 hot_t;
	F32 active_t;
};

typedef struct UI_Comm UI_Comm;
struct UI_Comm
{
	UI_Box *widget;
	Vec2F32 mouse;
	Vec2F32 drag_delta;
	B8 pressed;
	B8 released;
	B8 double_clicked;
	B8 right_pressed;
	B8 right_released;
	B8 dragging;
	B8 hovering;
};

typedef struct UI_FreeBox UI_FreeBox;
struct UI_FreeBox
{
	UI_FreeBox *next;
};

typedef struct UI_BoxStorage UI_BoxStorage;
struct UI_BoxStorage
{
	UI_FreeBox *first_free_box;
	UI_Box *boxes;
	U64 storage_count;
	U64 num_free_boxes;
};

typedef struct UI_ParentStackNode UI_ParentStackNode;
struct UI_ParentStackNode
{
	UI_ParentStackNode *next;
	UI_Box *box;
};

typedef struct UI_ParentStack UI_ParentStack;
struct UI_ParentStack
{
	UI_ParentStackNode *first;
};

typedef struct UI_KeyStackNode UI_KeyStackNode;
struct UI_KeyStackNode
{
	UI_KeyStackNode *next;
	UI_Key key;
};

typedef struct UI_Context UI_Context;
struct UI_Context
{
	Arena *permanent_arena;
	Arena *frame_arena;

	UI_BoxStorage box_storage;
	UI_Box      **box_hash_map;
	U64           box_hash_map_count;

	UI_ParentStackNode *parent_stack;
	UI_KeyStackNode    *seed_stack;

	UI_RectStyleStack   rect_style_stack;
	UI_TextStyleStack   text_style_stack;
	UI_LayoutStyleStack layout_style_stack;

	UI_Box *root;

	UI_Key active_key;
	UI_Key hot_key;

	Gfx_EventList *event_list;
	R_Context     *renderer;

	U64 frame_index;
};

internal UI_Context *ui_init(Void);

internal Void ui_begin(UI_Context *ui_ctx, Gfx_EventList *event_list, R_Context *renderer);
internal Void ui_end(Void);

internal UI_Size ui_pixels(F32 value, F32 strictness);
internal UI_Size ui_text_content(F32 strictness);
internal UI_Size ui_pct(F32 value, F32 strictness);
internal UI_Size ui_children_sum(F32 strictness);

internal UI_Comm ui_comm_from_box(UI_Box *box);

internal UI_Key ui_key_null(Void);
internal B32    ui_key_is_null(UI_Key key);
internal B32    ui_key_match(UI_Key a, UI_Key b);
internal UI_Key ui_key_from_string(UI_Key seed, Str8 string);
internal UI_Key ui_key_from_string_f(UI_Key seed, CStr fmt, ...);

internal UI_Box *ui_box_make(UI_BoxFlags flags, Str8 string);
internal UI_Box *ui_box_make_f(UI_BoxFlags flags, CStr fmt, ...);

internal Void ui_box_equip_display_string(UI_Box *widget, Str8 string);

internal UI_Box *ui_top_parent(Void);
internal UI_Box *ui_push_parent(UI_Box *widget);
internal UI_Box *ui_pop_parent(Void);

internal UI_Key ui_top_seed(Void);
internal UI_Key ui_push_seed(UI_Key key);
internal UI_Key ui_pop_seed(Void);

// NOTE(hampus): Rect styling

#define ui_next_colors(c0, c1, c2, c3) \
{ \
UI_RectStyle *style = ui_get_auto_pop_rect_style(); \
style->color[0] = c0; \
style->color[1] = c1; \
style->color[2] = c2; \
style->color[3] = c3; \
}

#define ui_push_colors(c0, c1, c2, c3) \
{ \
UI_RectStyle *style = ui_push_rect_style(); \
style->color[0] = c0; \
style->color[1] = c1; \
style->color[2] = c2; \
style->color[3] = c3; \
}

#define ui_pop_colors() stmnt(ui_pop_rect_style();)

#define ui_next_color(c) ui_next_colors(c, c, c, c)
#define ui_push_color(c) ui_push_colors(c, c, c, c)
#define ui_pop_color()   ui_pop_colors()

#define ui_next_vert_gradient(top, bottom) ui_next_colors(top, top, bottom, bottom)
#define ui_push_vert_gradient(top, bottom) ui_push_colors(top, top, bottom, bottom)
#define ui_pop_vert_gradient(top, bottom)  ui_pop_colors()

#define ui_next_hori_gradient(left, right) ui_next_colors(left, right, left, right)
#define ui_push_hori_gradient(left, right) ui_push_colors(left, right, left, right)
#define ui_pop_hori_gradient(left, right)  ui_pop_colors()

#define ui_next_border_color(color) stmnt(ui_get_auto_pop_rect_style()->border_color = color;)
#define ui_push_border_color(color) stmnt(ui_push_rect_style()->border_color = color;)
#define ui_pop_border_color()    stmnt(ui_pop_rect_style();)

#define ui_next_border_thickness(th) stmnt(ui_get_auto_pop_rect_style()->border_thickness = th;)
#define ui_push_border_thickness(th) stmnt(ui_push_rect_style()->border_thickness = th;)
#define ui_pop_border_thickness()    stmnt(ui_pop_rect_style();)

#define ui_next_corner_radies(r0, r1, r2, r3) \
{ \
UI_RectStyle *style = ui_get_auto_pop_rect_style(); \
style->radies.v[0] = r0; \
style->radies.v[1] = r1; \
style->radies.v[2] = r2; \
style->radies.v[3] = r3; \
}

#define ui_push_corner_radies(r0, r1, r2, r3) \
{ \
UI_RectStyle *style = ui_push_rect_style(); \
style->radies.v[0] = r0; \
style->radies.v[1] = r1; \
style->radies.v[2] = r2; \
style->radies.v[3] = r3; \
}

#define ui_pop_corner_radies() stmnt(ui_pop_rect_style();)

#define ui_next_corner_radius(c) ui_next_corner_radies(c, c, c, c)
#define ui_push_corner_radius(c) ui_push_corner_radies(c, c, c, c)
#define ui_corner_pop_radius()   ui_pop_corner_radies()

#define ui_next_vert_corner_radius(top, bottom) ui_next_corner_radies(top, top, bottom, bottom)
#define ui_push_vert_corner_radius(top, bottom) ui_push_corner_radies(top, top, bottom, bottom)
#define ui_pop_vert_corner_radius(top, bottom)  ui_pop_corner_radies()

#define ui_next_hori_corner_radius(left, right) ui_next_corner_radies(left, right, left, right)
#define ui_push_hori_corner_radius(left, right) ui_push_corner_radies(left, right, left, right)
#define ui_pop_hori_corner_radius(left, right)  ui_pop_corner_radies()

#define ui_next_softness(s)  stmnt(ui_get_auto_pop_rect_style()->softness = s;)
#define ui_push_softness(s)  stmnt(ui_push_rect_style()->softness = s;)
#define ui_pop_softness()    stmnt(ui_pop_rect_style();)

// NOTE(hampus): Text styling

#define ui_next_text_color(c)  stmnt(ui_get_auto_pop_text_style()->color = c;)
#define ui_push_text_color(c)  stmnt(ui_push_text_style()->color = c;)
#define ui_pop_text_color()    stmnt(ui_pop_text_style();)

#define ui_next_text_align(x)  stmnt(ui_get_auto_pop_text_style()->align = x;)
#define ui_push_text_align(x)  stmnt(ui_push_text_style()->align = x;)
#define ui_pop_text_align()    stmnt(ui_pop_text_style();)

#define ui_next_text_padding(axis, x)  stmnt(ui_get_auto_pop_text_style()->padding[axis] = x;)
#define ui_push_text_padding(axis, x)  stmnt(ui_push_text_style()->padding[axis] = x;)
#define ui_pop_text_padding()          stmnt(ui_pop_text_style();)

#define ui_next_icon(x)  stmnt(ui_get_auto_pop_text_style()->icon = x;)
#define ui_push_icon(x)  stmnt(ui_push_text_style()->icon = x;)
#define ui_pop_icon()    stmnt(ui_pop_text_style();)

#define ui_next_font(x)  stmnt(ui_get_auto_pop_text_style()->font = x;)
#define ui_push_font(x)  stmnt(ui_push_text_style()->font = x;)
#define ui_pop_font()    stmnt(ui_pop_text_style();)

// NOTE(hampus): Layout styling

#define ui_next_size(axis, sz) stmnt(ui_get_auto_pop_layout_style()->size[axis] = sz;)
#define ui_push_size(axis, sz) stmnt(ui_push_layout_style()->size[axis] = sz;)
#define ui_pop_size()          stmnt(ui_pop_layout_style();)

#define ui_next_width(sz) ui_next_size(Axis2_X, sz)
#define ui_push_width(sz) ui_push_size(Axis2_X, sz)
#define ui_pop_width()    ui_pop_size()

#define ui_next_height(sz) ui_next_size(Axis2_Y, sz)
#define ui_push_height(sz) ui_push_size(Axis2_Y, sz)
#define ui_pop_height()    ui_pop_size()

// NOTE(hampus): For these to work, the box *has* to have either
// UI_BoxFlag_FloatingX and/or UI_BoxFlag_FloatingY flag
#define ui_next_relative_pos(axis, p) stmnt(ui_get_auto_pop_layout_style()->relative_pos[axis] = p;)
#define ui_push_relative_pos(axis, p) stmnt(ui_push_layout_style()->relative_pos[axis] = p;)
#define ui_pop_relative_pos()         stmnt(ui_pop_layout_style();)

#define ui_next_child_layout_axis(axis) stmnt(ui_get_auto_pop_layout_style()->child_layout_axis = axis;)
#define ui_push_child_layout_axis(axis) stmnt(ui_push_layout_style()->child_layout_axis = axis;)
#define ui_pop_child_layout_axis()      stmnt(ui_pop_layout_style();)

#define ui_next_extra_box_flags(x) stmnt(ui_get_auto_pop_layout_style()->box_flags = x;)
#define ui_push_extra_box_flags(x) stmnt(ui_push_layout_style()->box_flags = x;)
#define ui_pop_extra_box_flags()   stmnt(ui_pop_layout_style();)

#endif //UI_CORE_H