#ifndef UI_CORE_H
#define UI_CORE_H

#define UI_ICON_FONT_PATH "data/fonts/fontello.ttf"

#define UI_GATHER_STATS 1

#if UI_GATHER_STATS
#  define ui_stats_inc_val(name) ui_get_current_stats()->name++
#else
#  define ui_stats_inc_val(name)
#endif

typedef struct UI_Config UI_Config;
struct UI_Config
{
    B32 animations;
    F32 animation_speed;
    Render_FontKey icon_font;
};

typedef enum UI_BoxFlags UI_BoxFlags;
enum UI_BoxFlags
{
    // NOTE(hampus): This allows the box to
    // consume key press events which is the only
    // way right now to become active.
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
    UI_BoxFlag_ViewScroll      = (1 << 8),

    // NOTE(hampus): This decides if the children
    // of the box are allowed to go outside of
    // the parent region
    UI_BoxFlag_AllowOverflowX       = (1 << 9),
    UI_BoxFlag_AllowOverflowY       = (1 << 10),

    // NOTE(hampus): This decides if the box's rect should be pushed as a clip
    // rect when it is rendered and for input
    UI_BoxFlag_Clip            = (1 << 11),

    // NOTE(hampus): These makes the ui auto-layouting
    // algorithm skip this box. Useful if you want
    // the box to have an absolute position.
    //
    // Set the position with ui_push/next_relative_pos
    // It will be relative to the parent.
    UI_BoxFlag_FixedX       = (1 << 12),
    UI_BoxFlag_FixedY       = (1 << 13),

    UI_BoxFlag_FixedRect = (1 << 14),

    UI_BoxFlag_AnimateX = (1 << 15),
    UI_BoxFlag_AnimateY = (1 << 16),

    UI_BoxFlag_AnimateWidth  = (1 << 17),
    UI_BoxFlag_AnimateHeight = (1 << 18),

    UI_BoxFlag_AnimateScrollX = (1 << 19),
    UI_BoxFlag_AnimateScrollY = (1 << 20),

    UI_BoxFlag_Disabled        = (1 << 21),

    UI_BoxFlag_AnimateScroll = UI_BoxFlag_AnimateScrollX | UI_BoxFlag_AnimateScrollY,
    UI_BoxFlag_FixedPos      = UI_BoxFlag_FixedX | UI_BoxFlag_FixedY,
    UI_BoxFlag_AnimatePos    = UI_BoxFlag_AnimateX | UI_BoxFlag_AnimateY,
    UI_BoxFlag_AnimateDim    = UI_BoxFlag_AnimateWidth | UI_BoxFlag_AnimateHeight,
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

    Vec4F32    color[4];
    Vec4F32    border_color;
    F32        border_thickness;
    Vec4F32    radies;
    F32        softness;
    Gfx_Cursor hover_cursor;
    Render_TextureSlice  slice;
    Render_TextureFilter texture_filter;
};

typedef struct UI_TextStyle UI_TextStyle;
struct UI_TextStyle
{
    UI_TextStyle *stack_next;

    Vec4F32      color;
    UI_TextAlign align;
    Vec2F32      padding;
    U32          icon;
    U32          font_size;
    Str8         font;
};

typedef struct UI_LayoutStyle UI_LayoutStyle;
struct UI_LayoutStyle
{
    UI_LayoutStyle *stack_next;

    UI_Size     size[Axis2_COUNT];
    Vec2F32     relative_pos;
    Axis2       child_layout_axis;
    UI_BoxFlags box_flags;

    // TODO(hampus): This will make each UI_Box have a double fixed_rect
    RectF32     fixed_rect;
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

#define UI_CUSTOM_DRAW_PROC(name) Void name(UI_Box *root)
typedef UI_CUSTOM_DRAW_PROC(UI_CustomDrawProc);

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
    U64    first_frame_touched_index;
    U64    last_frame_touched_index;

    Vec2F32 rel_pos_animated;
    Vec2F32 rel_pos;

    Vec2F32 fixed_size_animated;
    Vec2F32 fixed_size;

    Vec2F32 scroll_animated;
    Vec2F32 scroll;

    RectF32 fixed_rect;

    UI_BoxFlags flags;
    Str8        string;
    Str8        debug_string;

    UI_RectStyle   rect_style;
    UI_TextStyle   text_style;
    UI_LayoutStyle layout_style;

    UI_CustomDrawProc *custom_draw;
    Void *data;

    F32 hot_t;
    F32 active_t;
};

typedef struct UI_Comm UI_Comm;
struct UI_Comm
{
    UI_Box *box;
    // NOTE(hampus): Relative to upper-left
    // corner of the box
    Vec2F32 rel_mouse;
    Vec2F32 drag_delta;
    Vec2F32 scroll;
    B8 clicked        : 1;
    B8 pressed        : 1;
    B8 released       : 1;
    B8 double_clicked : 1;
    B8 right_pressed  : 1;
    B8 right_released : 1;
    B8 dragging       : 1;
    B8 hovering       : 1;
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

typedef struct UI_Stats UI_Stats;
struct UI_Stats
{
    U64 rect_style_push_count;
    U64 text_style_push_count;
    U64 layout_style_push_count;
    U64 parent_push_count;
    U64 seed_push_count;
    U64 box_chain_count;
    U64 max_box_chain_count;
    U64 num_hashed_boxes;
    U64 num_transient_boxes;
};

typedef enum UI_TextActionFlag UI_TextActionFlag;
enum UI_TextActionFlag
{
    UI_TextActionFlag_WordScan                = (1 << 0),
    UI_TextActionFlag_KeepMark                = (1 << 1),
    UI_TextActionFlag_Delete                  = (1 << 2),
    UI_TextActionFlag_Copy                    = (1 << 3),
    UI_TextActionFlag_Paste                   = (1 << 4),
    UI_TextActionFlag_ZeroDeltaWithSelection  = (1 << 5),
    UI_TextActionFlag_DeltaPicksSelectionSide = (1 << 6),
    UI_TextActionFlag_SelectAll               = (1 << 7),
};

typedef struct UI_TextAction UI_TextAction;
struct UI_TextAction
{
    UI_TextActionFlag flags;
    S64 delta;
    U32 codepoint;
};

typedef struct UI_TextActionNode UI_TextActionNode;
struct UI_TextActionNode
{
    UI_TextActionNode *next;
    UI_TextActionNode *prev;
    UI_TextAction action;
};

typedef struct UI_TextActionList UI_TextActionList;
struct UI_TextActionList
{
    UI_TextActionNode *first;
    UI_TextActionNode *last;
};

typedef struct UI_TextOp UI_TextOp;
struct UI_TextOp
{
    S64 new_cursor;
    S64 new_mark;
    Vec2S64 range;
    Str8 replace_string;
    Str8 copy_string;
};

typedef struct UI_TextEditState UI_TextEditState;
struct UI_TextEditState
{
    S64 cursor;
    S64 mark;
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
    UI_Box *normal_root;
    UI_Box *tooltip_root;
    UI_Box *ctx_menu_root;

    UI_Key ctx_menu_key;
    UI_Key ctx_menu_anchor_key;
    Vec2F32 anchor_offset;

    UI_Key  next_ctx_menu_key;
    UI_Key  next_ctx_menu_anchor_key;
    Vec2F32 next_anchor_offset;

    UI_Key active_key;
    UI_Key hot_key;
    UI_Key focus_key;

    // NOTE(hampus): We only have this to for the
    // "clicked" member in UI_Comm
    UI_Key prev_active_key;

    Gfx_EventList *event_list;
    Render_Context *renderer;

    UI_Config config;

    UI_Stats stats[2];

    Vec2F32 mouse_pos;
    Vec2F32 prev_mouse_pos;

    B32 show_debug_lines;

    F64 dt;
    U64 frame_index;
};

////////////////////////////////
// hampus: Accessor functions

internal F64             ui_dt(Void);
internal Render_Context *ui_renderer(Void);
internal Gfx_EventList  *ui_events(Void);
internal UI_Stats       *ui_get_current_stats(Void);
internal UI_Stats       *ui_get_prev_stats(Void);
internal Vec2F32         ui_prev_mouse_pos(Void);
internal Vec2F32         ui_mouse_pos(Void);
internal Arena          *ui_permanent_arena(Void);
internal Arena          *ui_frame_arena(Void);
internal B32             ui_animations_enabled(Void);
internal F32             ui_animation_speed(Void);

////////////////////////////////
// hampus: Keying

internal UI_Key ui_key_null(Void);
internal B32    ui_key_is_null(UI_Key key);
internal B32    ui_key_match(UI_Key a, UI_Key b);
internal UI_Key ui_key_from_string(UI_Key seed, Str8 string);
internal UI_Key ui_key_from_stringf(UI_Key seed, CStr fmt, ...);
internal Str8   ui_get_hash_part_from_string(Str8 string);
internal Str8   ui_get_display_part_from_string(Str8 string);

////////////////////////////////
// hampus: Sizing

internal UI_Size ui_pixels(F32 value, F32 strictness);
internal UI_Size ui_text_content(F32 strictness);
internal UI_Size ui_pct(F32 value, F32 strictness);
internal UI_Size ui_children_sum(F32 strictness);
internal UI_Size ui_em(F32 value, F32 strictness);
internal UI_Size ui_fill(Void);

////////////////////////////////
// hampus: Text editing

internal UI_TextAction ui_text_action_from_event(Gfx_Event *event);

////////////////////////////////
// hampus: Box

internal UI_Comm ui_comm_from_box(UI_Box *box);
internal B32     ui_box_is_active(UI_Box *box);
internal B32     ui_box_is_hot(UI_Box *box);
internal B32     ui_box_is_focused(UI_Box *box);
internal UI_Box *ui_box_alloc(Void);
internal Void    ui_box_free(UI_Box *box);
internal B32     ui_box_has_flag(UI_Box *box, UI_BoxFlags flag);
internal UI_Box *ui_box_from_key(UI_Key key);
internal UI_Box *ui_box_make(UI_BoxFlags flags, Str8 string);
internal UI_Box *ui_box_makef(UI_BoxFlags flags, CStr fmt, ...);
internal Void    ui_box_equip_display_string(UI_Box *box, Str8 string);
internal Void    ui_box_equip_custom_draw_proc(UI_Box *box, UI_CustomDrawProc *proc);
internal B32     ui_box_was_created_this_frame(UI_Box *box);
internal B32     ui_mouse_is_inside_box(UI_Box *box);

////////////////////////////////
// hampus: Ctx menu

internal B32    ui_ctx_menu_begin(UI_Key key);
internal Void   ui_ctx_menu_end(Void);
internal Void   ui_ctx_menu_open(UI_Key anchor, Vec2F32 offset, UI_Key menu);
internal Void   ui_ctx_menu_close(Void);
internal B32    ui_ctx_menu_is_open(Void);
internal UI_Key ui_ctx_menu_key(Void);

////////////////////////////////
// hampus: Tooltip

internal Void ui_tooltip_begin(Void);
internal Void ui_tooltip_end(Void);

////////////////////////////////
// hampus: Init, begin end

internal UI_Context *ui_init(Void);
internal Void        ui_begin(UI_Context *ctx, Gfx_EventList *event_list, Render_Context *renderer, F64 dt);
internal Void        ui_end(Void);

////////////////////////////////
// hampus: Layout pass

internal Void ui_solve_independent_sizes(UI_Box *root, Axis2 axis);
internal Void ui_solve_upward_dependent_sizes(UI_Box *root, Axis2 axis);
internal Void ui_solve_downward_dependent_sizes(UI_Box *root, Axis2 axis);
internal Void ui_calculate_final_rect(UI_Box *root, Axis2 axis, F32 offset);
internal Void ui_layout(UI_Box *root);
internal Void ui_solve_size_violations(UI_Box *root, Axis2 axis);

////////////////////////////////
// hampus: Draw pass

internal Vec2F32 ui_align_text_in_rect(Render_Font *font, Str8 string, RectF32 rect, UI_TextAlign align, Vec2F32 padding);
internal Vec2F32 ui_align_character_in_rect(Render_Font *font, U32 codepoint, RectF32 rect, UI_TextAlign align);
internal Void    ui_draw(UI_Box *root);

////////////////////////////////
// hampus: Stack helpers

internal F32            ui_top_font_line_height(Void);
internal Render_FontKey ui_font_key_from_text_style(UI_TextStyle *text_style);
internal Render_FontKey ui_top_font_key(Void);

////////////////////////////////
// hampus: Stack managing

internal UI_Box         *ui_top_parent(Void);
internal UI_Box         *ui_push_parent(UI_Box *box);
internal UI_Box         *ui_pop_parent(Void);
internal UI_Key          ui_top_seed(Void);
internal UI_Key          ui_push_seed(UI_Key key);
internal UI_Key          ui_pop_seed(Void);
internal Void            ui_push_string(Str8 string);
internal Void            ui_pop_string(Void);
internal UI_RectStyle   *ui_top_rect_style(Void);
internal UI_RectStyle   *ui_push_rect_style(Void);
internal Void            ui_pop_rect_style(Void);
internal UI_RectStyle   *ui_get_auto_pop_rect_style(Void);
internal UI_TextStyle   *ui_top_text_style(Void);
internal UI_TextStyle   *ui_push_text_style(Void);
internal UI_TextStyle   *ui_pop_text_style(Void);
internal UI_TextStyle   *ui_get_auto_pop_text_style(Void);
internal UI_LayoutStyle *ui_top_layout_style(Void);
internal UI_LayoutStyle *ui_push_layout_style(Void);
internal Void            ui_pop_layout_style(Void);
internal UI_LayoutStyle *ui_get_auto_pop_layout_style(Void);

////////////////////////////////
// hampus: Defers

#define ui_parent(box)   defer_loop(ui_push_parent(box), ui_pop_parent())
#define ui_seed(string)  defer_loop(ui_push_string(string), ui_pop_string())
#define ui_tooltip()     defer_loop(ui_tooltip_begin(), ui_tooltip_end())
#define ui_ctx_menu(key) defer_loop_checked(ui_ctx_menu_begin(key), ui_ctx_menu_end())

////////////////////////////////
// hampus: Rect styling

#define ui_next_colors(c0, c1, c2, c3) memory_copy_array(ui_get_auto_pop_rect_style()->color, ((Vec4F32[4]){c0, c1, c2, c3}))

#define ui_push_colors(c0, c1, c2, c3) memory_copy_array(ui_push_rect_style()->color, ((Vec4F32[4]){c0, c1, c2, c3}))

#define ui_pop_colors()           ui_pop_rect_style()
#define ui_colors(c0, c1, c2, c3) defer_loop(ui_push_colors(c0, c1, c2, c3), ui_pop_colors())

#define ui_next_color(c) ui_next_colors(c, c, c, c)
#define ui_push_color(c) ui_push_colors(c, c, c, c)
#define ui_pop_color()   ui_pop_colors()
#define ui_color(c)      defer_loop(ui_push_color(c), ui_pop_color())

#define ui_next_vert_gradient(top, bottom) ui_next_colors(top, top, bottom, bottom)
#define ui_push_vert_gradient(top, bottom) ui_push_colors(top, top, bottom, bottom)
#define ui_pop_vert_gradient()             ui_pop_colors()
#define ui_vert_gradient(top, bottom)      defer_loop(ui_push_vert_gradient(top, bottom), ui_pop_vert_gradient())

#define ui_next_hori_gradient(left, right) ui_next_colors(left, right, left, right)
#define ui_push_hori_gradient(left, right) ui_push_colors(left, right, left, right)
#define ui_pop_hori_gradient(left, right)  ui_pop_colors()
#define ui_hori_gradient(left, right)      defer_loop(ui_push_hori_gradient(left, right), ui_pop_hori_gradient())

#define ui_next_border_color(color) ui_get_auto_pop_rect_style()->border_color = color
#define ui_push_border_color(color) ui_push_rect_style()->border_color = color
#define ui_pop_border_color()       ui_pop_rect_style()
#define ui_border_color(color)      defer_loop(ui_push_border_color(color), ui_pop_border_color())

#define ui_next_border_thickness(th) ui_get_auto_pop_rect_style()->border_thickness = th
#define ui_push_border_thickness(th) ui_push_rect_style()->border_thickness = th
#define ui_pop_border_thickness()    ui_pop_rect_style()
#define ui_border_thickness(th)      defer_loop(ui_push_border_thickness(th), ui_pop_border_thickness());

#define ui_next_corner_radies(r0, r1, r2, r3) memory_copy_array(ui_get_auto_pop_rect_style()->radies.v, ((F32[4]){r0, r1, r2, r3}))
#define ui_push_corner_radies(r0, r1, r2, r3) memory_copy_array(ui_push_rect_style()->radies.v, ((F32[4]){r0, r1, r2, r3}))
#define ui_pop_corner_radies()                ui_pop_rect_style()
#define ui_corner_radies(r0, r1, r2, r3)      defer_loop(ui_push_corner_radies(r0, r1, r2, r3), ui_pop_corner_radies())

#define ui_next_corner_radius(c) ui_next_corner_radies(c, c, c, c)
#define ui_push_corner_radius(c) ui_push_corner_radies(c, c, c, c)
#define ui_pop_corner_radius()   ui_pop_corner_radies()
#define ui_corner_radius(c)      defer_loop(ui_push_corner_radius(c), ui_pop_corner_radius())

#define ui_next_vert_corner_radius(top, bottom) ui_next_corner_radies(top, top, bottom, bottom)
#define ui_push_vert_corner_radius(top, bottom) ui_push_corner_radies(top, top, bottom, bottom)
#define ui_pop_vert_corner_radius(top, bottom)  ui_pop_corner_radies()
#define ui_vert_corner_radius(top, bottom)      defer_loop(ui_push_vert_corner_radius(top, bottom), ui_pop_vert_corner_radius())

#define ui_next_hori_corner_radius(left, right) ui_next_corner_radies(left, right, left, right)
#define ui_push_hori_corner_radius(left, right) ui_push_corner_radies(left, right, left, right)
#define ui_pop_hori_corner_radius(left, right)  ui_pop_corner_radies()
#define ui_hori_corner_radius(left, right)      defer_loop(ui_push_hori_corner_radius(left, right), ui_pop_hori_corner_radius());

#define ui_next_softness(s) ui_get_auto_pop_rect_style()->softness = s
#define ui_push_softness(s) ui_push_rect_style()->softness = s
#define ui_pop_softness()   ui_pop_rect_style()
#define ui_softness(s)      defer_loop(ui_push_softness(s), ui_pop_softness())

#define ui_next_slice(x) ui_get_auto_pop_rect_style()->slice = x
#define ui_push_slice(x) ui_push_rect_style()->slice = x
#define ui_pop_slice()   ui_pop_rect_style()
#define ui_slice(x)      defer_loop(ui_push_slice(x), ui_pop_slice())

#define ui_next_texture_filter(x) ui_get_auto_pop_rect_style()->texture_filter = x
#define ui_push_texture_filter(x) ui_push_rect_style()->texture_filter = x
#define ui_pop_texture_filter()   ui_pop_rect_style()
#define ui_texture_filter(x)      defer_loop(ui_push_texture_filter(x), ui_pop_texture_filter())

#define ui_next_hover_cursor(x) ui_get_auto_pop_rect_style()->hover_cursor = x
#define ui_push_hover_cursor(x) ui_push_rect_style()->hover_cursor = x
#define ui_pop_hover_cursor()   ui_pop_rect_style()
#define ui_hover_cursor(x)      defer_loop(ui_push_hover_cursor(x), ui_pop_hover_cursor())

////////////////////////////////
// hampus: Text styling

#define ui_next_text_color(c) ui_get_auto_pop_text_style()->color = c
#define ui_push_text_color(c) ui_push_text_style()->color = c
#define ui_pop_text_color()   ui_pop_text_style()
#define ui_text_color(c)      defer_loop(ui_push_text_color(c), ui_pop_text_color())

#define ui_next_text_align(x) ui_get_auto_pop_text_style()->align = x
#define ui_push_text_align(x) ui_push_text_style()->align = x
#define ui_pop_text_align()   ui_pop_text_style()
#define ui_text_align(x)      defer_loop(ui_push_text_align(x), ui_pop_text_align())

#define ui_next_text_padding(axis, x) ui_get_auto_pop_text_style()->padding.v[axis] = x
#define ui_push_text_padding(axis, x) ui_push_text_style()->padding.v[axis] = x
#define ui_pop_text_padding()         ui_pop_text_style()
#define ui_text_padding(axis, x)      defer_loop(ui_push_text_padding(axis, x), ui_pop_text_padding())

#define ui_next_font_size(x) ui_get_auto_pop_text_style()->font_size = x
#define ui_push_font_size(x) ui_push_text_style()->font_size = x
#define ui_pop_font_size()   ui_pop_text_style()
#define ui_font_size(x)      defer_loop(ui_push_font_size(x), ui_pop_font_size())

#define ui_next_icon(x) ui_get_auto_pop_text_style()->icon = x; ui_next_font(str8_lit(UI_ICON_FONT_PATH))
#define ui_push_icon(x) ui_push_text_style()->icon = x; ui_push_font(str8_lit(UI_ICON_FONT_PATH))
#define ui_pop_icon()   ui_pop_text_style(); ui_pop_font()
#define ui_icon(x)      defer_loop(ui_push_icon(x), ui_pop_icon())

#define ui_next_font(x) ui_get_auto_pop_text_style()->font = x
#define ui_push_font(x) ui_push_text_style()->font = x
#define ui_pop_font()   ui_pop_text_style()
#define ui_font(x)      defer_loop(ui_push_font(x), ui_pop_font())

////////////////////////////////
// hampus: Layout styling

#define ui_next_size(axis, sz) ui_get_auto_pop_layout_style()->size[axis] = sz
#define ui_push_size(axis, sz) ui_push_layout_style()->size[axis] = sz
#define ui_pop_size()          ui_pop_layout_style()
#define ui_size(axis, sz)      defer_loop(ui_push_size(axis, sz), ui_pop_size())

#define ui_next_width(sz) ui_next_size(Axis2_X, sz)
#define ui_push_width(sz) ui_push_size(Axis2_X, sz)
#define ui_pop_width()    ui_pop_size()
#define ui_width(sz)      defer_loop(ui_push_width(sz), ui_pop_width())

#define ui_next_height(sz) ui_next_size(Axis2_Y, sz)
#define ui_push_height(sz) ui_push_size(Axis2_Y, sz)
#define ui_pop_height()    ui_pop_size()
#define ui_height(sz)      defer_loop(ui_push_height(sz), ui_pop_height())

// NOTE(hampus): For these to work, the box has to have either
// UI_BoxFlag_FixedX and/or UI_BoxFlag_FixedY flag
#define ui_next_relative_pos(axis, p) ui_get_auto_pop_layout_style()->relative_pos.v[axis] = p
#define ui_push_relative_pos(axis, p) ui_push_layout_style()->relative_pos.v[axis] = p
#define ui_pop_relative_pos()         ui_pop_layout_style()
#define ui_relative_pos(axis, p)      defer_loop(ui_push_relative_pos(axis, p), ui_pop_relative_pos())

#define ui_next_child_layout_axis(axis) ui_get_auto_pop_layout_style()->child_layout_axis = axis
#define ui_push_child_layout_axis(axis) ui_push_layout_style()->child_layout_axis = axis
#define ui_pop_child_layout_axis()      ui_pop_layout_style()
#define ui_child_layout_axis(axis)      defer_loop(ui_push_child_layout_axis(axis), ui_pop_child_layout_axis());

#define ui_next_extra_box_flags(x) ui_get_auto_pop_layout_style()->box_flags |= (x)
#define ui_push_extra_box_flags(x) ui_push_layout_style()->box_flags |= (x)
#define ui_pop_extra_box_flags()   ui_pop_layout_style()
#define ui_extra_box_flags(x)      defer_loop(ui_push_extra_box_flags(x), ui_pop_extra_box_flags());

#define ui_next_fixed_rect(x) ui_get_auto_pop_layout_style()->fixed_rect = x
#define ui_push_fixed_rect(x) ui_push_layout_style()->fixed_rect = x
#define ui_pop_fixed_rect()   ui_pop_layout_style()
#define ui_fixed_rect(x)      defer_loop(ui_push_fixed_rect(x), ui_pop_fixed_rect());

#endif //UI_CORE_H
