#ifndef UI_BASIC_WIDGETS_H
#define UI_BASIC_WIDGETS_H

typedef struct UI_ScrollabelRegion UI_ScrollabelRegion;
struct UI_ScrollabelRegion
{
    UI_Box *view_region;
    UI_Box *content;
};

typedef struct UI_ColorPickerData UI_ColorPickerData;
struct UI_ColorPickerData
{
    Vec4F32 *rgba;

    // NOTE(hampus): Stuff for the line edit
    UI_TextEditState text_edit_state[4];
    U8 *text_buffer[4];
    U64 text_buffer_size[4];
    U64 string_length[4];
};

internal Void    ui_text(Str8 string);
internal Void    ui_textf(CStr fmt, ...);
internal UI_Comm ui_image(Render_TextureSlice slice, Str8 string);
internal UI_Comm ui_imagef(Render_TextureSlice slice, CStr fmt, ...);
internal UI_Comm ui_button(Str8 string);
internal UI_Comm ui_buttonf(CStr fmt, ...);
internal UI_Comm ui_check(B32 *value, Str8 string);
internal UI_Comm ui_checkf(B32 *value, CStr fmt, ...);

internal Void ui_pop_scrollable_region_axis(Axis2 axis);
internal UI_ScrollabelRegion ui_push_scrollable_region_axis(Str8 string, Axis2 axis);


internal UI_ScrollabelRegion ui_push_scrollable_region(Str8 string);
internal Void                ui_pop_scrollable_region(Void);
internal Void                ui_scrollable_region_set_scroll(UI_ScrollabelRegion region, F32 offset);

internal Void    ui_spacer(UI_Size size);

internal UI_Box *ui_named_row_begin(Str8 string);
internal Void    ui_named_row_end(Void);
internal UI_Box *ui_named_row_beginfv(CStr fmt, va_list args);
internal UI_Box *ui_named_row_beginf(CStr fmt, ...);
internal UI_Box *ui_row_begin(Void);
internal Void    ui_row_end(Void);

internal UI_Box *ui_named_column_begin(Str8 string);
internal Void    ui_named_column_end(Void);
internal UI_Box *ui_named_column_beginfv(CStr fmt, va_list args);
internal UI_Box *ui_named_column_beginf(CStr fmt, ...);
internal UI_Box *ui_column_begin(Void);
internal Void    ui_column_end(Void);

internal UI_Comm ui_line_edit(UI_TextEditState *edit_state, U8 *buffer, U64 buffer_size, U64 *string_length, Str8 string);
internal UI_Comm ui_line_editf(UI_TextEditState *edit_state, U8 *buffer, U64 buffer_size, U64 *string_length, CStr format, ...);

internal Void ui_color_picker(UI_ColorPickerData *data);

typedef struct UI_ComboBoxParams UI_ComboBoxParams;
struct UI_ComboBoxParams
{
    UI_Size item_size;
};

internal B32 ui_combo_box_internal(Str8 name, U32 *selected_index, Str8 *item_names, U32 item_count, UI_ComboBoxParams *params);

#define ui_combo_box(name, selected_index, item_names, item_count, ...) ui_combo_box_internal(name, selected_index, item_names, item_count, &(UI_ComboBoxParams) { 0, __VA_ARGS__ });

#define ui_named_row(string) defer_loop(ui_named_row_begin(string), ui_named_row_end())
#define ui_named_rowf(string, ...) defer_loop(ui_named_row_beginf(string, __VA_ARGS__), ui_named_row_end())
#define ui_row()             defer_loop(ui_row_begin(), ui_row_end())

#define ui_named_column(string) defer_loop(ui_named_column_begin(string), ui_named_column_end())
#define ui_named_columnf(string, ...) defer_loop(ui_named_column_beginf(string, __VA_ARGS__), ui_named_column_end())
#define ui_column()             defer_loop(ui_column_begin(), ui_column_end())

#define ui_scrollable_region_axis(string, axis) defer_loop(ui_push_scrollable_region_axis(string, axis), ui_pop_scrollable_region_axis(axis))
#define ui_scrollable_region(string) defer_loop(ui_push_scrollable_region(string), ui_pop_scrollable_region())

#endif //UI_BASIC_WIDGETS_H
