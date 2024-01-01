#ifndef UI_BASIC_WIDGETS_H
#define UI_BASIC_WIDGETS_H

internal Void    ui_text(Str8 string);
internal Void    ui_textf(CStr fmt, ...);
internal UI_Comm ui_image(R_TextureSlice slice, Str8 string);
internal UI_Comm ui_imagef(R_TextureSlice slice, CStr fmt, ...);
internal UI_Comm ui_button(Str8 string);
internal UI_Comm ui_buttonf(CStr fmt, ...);
internal UI_Comm ui_check(B32 *value, Str8 string);
internal UI_Comm ui_checkf(B32 *value, CStr fmt, ...);

internal Void    ui_spacer(UI_Size size);

internal UI_Box *ui_named_row_begin(Str8 string);
internal Void    ui_named_row_end(Void);
internal UI_Box *ui_row_begin(Void);
internal Void    ui_row_end(Void);

internal UI_Box *ui_named_column_begin(Str8 string);
internal Void    ui_named_column_end(Void);
internal UI_Box *ui_column_begin(Void);
internal Void    ui_column_end(Void);

#define ui_named_row(string) defer_loop(ui_named_row_begin(string), ui_named_row_end())
#define ui_row()             defer_loop(ui_row_begin(), ui_row_end())

#define ui_named_column(string) defer_loop(ui_named_column_begin(string), ui_named_column_end())
#define ui_column()             defer_loop(ui_column_begin(), ui_column_end())

#endif //UI_BASIC_WIDGETS_H
