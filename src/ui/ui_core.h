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
};

typedef enum UI_SizeKind UI_SizeKind;
 enum UI_SizeKind
{
	UI_SizeKind_Null,
	UI_SizeKind_Pixels,
	UI_SizeKind_TextContent,
	UI_SizeKind_PercentOfParent,
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
	F32 border_thickness;
	Vec4F32 radies;
	F32 softness;
};

typedef struct UI_TextStyle UI_TextStyle;
struct UI_TextStyle
{
	UI_TextStyle *stack_next;

	Vec4F32 color;
	S32 font_size;
	UI_TextAlign text_align;
	F32 text_padding[Axis2_COUNT];
	U64 icon;
};

typedef struct UI_LayoutStyle UI_LayoutStyle;
struct UI_LayoutStyle
{
	UI_LayoutStyle *stack_next;

	UI_Size size[Axis2_COUNT];
	F32 relative_pos[Axis2_COUNT];
	Axis2 child_layout_axis;
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
	B8 clicked;
	B8 double_clicked;
	B8 right_clicked;
	B8 pressed;
	B8 released;
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
	UI_Box **box_hash_map;
	U64 box_hash_map_count;

	UI_ParentStackNode *parent_stack;
	UI_KeyStackNode    *seed_stack;

	UI_RectStyleStack  rect_stack;
	UI_TextStyleStack  text_stack;
	UI_LayoutStyleStack layout_stack;

	UI_Box *root;

	UI_Key active_key;
	UI_Key hot_key;

	R_FontKey font;

	Gfx_EventList *event_list;
	R_Context *renderer;

	U64 frame_index;
};

internal UI_Context *ui_init(Void);

internal Void ui_begin(UI_Context *ui_ctx, Gfx_EventList *event_list, R_Context *renderer, R_FontKey font);
internal Void ui_end(Void);

internal UI_Size ui_pixels(F32 value, F32 strictness);
internal UI_Size ui_text_content(F32 strictness);

internal UI_Comm ui_comm_from_box(UI_Box *box);

internal UI_Key ui_key_null(Void);
internal B32    ui_key_is_null(UI_Key key);
internal B32    ui_key_match(UI_Key a, UI_Key b);
internal UI_Key ui_key_from_string(UI_Key seed, Str8 string);
internal UI_Key ui_key_from_string_f(UI_Key seed, CStr fmt, ...);

internal UI_Box *ui_box_make(UI_BoxFlags flags, Str8 string);
internal UI_Box *ui_box_make_f(UI_BoxFlags flags, CStr fmt, ...);

internal Void ui_box_equip_display_string(UI_Box *widget, Str8 string);
internal Void ui_box_equip_child_layout_axis(UI_Box *widget, Axis2 axis);

internal UI_Box *ui_top_parent(Void);
internal UI_Box *ui_push_parent(UI_Box *widget);
internal UI_Box *ui_pop_parent(Void);

internal UI_Key ui_top_seed(Void);
internal UI_Key ui_push_seed(UI_Key key);
internal UI_Key ui_pop_seed(Void);

#define ui_next_size(axis, sz)         (ui_get_auto_pop_layout_style()->size[axis] = sz)
#define ui_push_size(axis, sz)         (ui_push_layout_style()->size[axis] = sz)
#define ui_pop_size()                    (ui_pop_layout_style())

#endif //UI_CORE_H