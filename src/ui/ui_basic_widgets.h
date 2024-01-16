#ifndef UI_BASIC_WIDGETS_H
#define UI_BASIC_WIDGETS_H

typedef struct UI_ScrollabelRegion UI_ScrollabelRegion;
struct UI_ScrollabelRegion
{
	UI_Box *view_region;
	UI_Box *container;
};

internal Void    ui_text(Str8 string);
internal Void    ui_textf(CStr fmt, ...);
internal UI_Comm ui_image(Render_TextureSlice slice, Str8 string);
internal UI_Comm ui_imagef(Render_TextureSlice slice, CStr fmt, ...);
internal UI_Comm ui_button(Str8 string);
internal UI_Comm ui_buttonf(CStr fmt, ...);
internal UI_Comm ui_check(B32 *value, Str8 string);
internal UI_Comm ui_checkf(B32 *value, CStr fmt, ...)
;
internal UI_ScrollabelRegion ui_push_scrollable_region(Str8 string);
internal Void                ui_pop_scrollable_region(Void);
internal Void                ui_scrollabel_region_set_scroll(UI_ScrollabelRegion region, F32 offset);

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

#define ui_named_row(string) defer_loop(ui_named_row_begin(string), ui_named_row_end())
#define ui_named_rowf(string, ...) defer_loop(ui_named_row_beginf(string, __VA_ARGS__), ui_named_row_end())
#define ui_row()             defer_loop(ui_row_begin(), ui_row_end())

#define ui_named_column(string) defer_loop(ui_named_column_begin(string), ui_named_column_end())
#define ui_named_columnf(string, ...) defer_loop(ui_named_column_beginf(string, __VA_ARGS__), ui_named_column_end())
#define ui_column()             defer_loop(ui_column_begin(), ui_column_end())

#define ui_scrollable_region(string) defer_loop(ui_push_scrollable_region(string), ui_pop_scrollable_region())

#endif //UI_BASIC_WIDGETS_H
