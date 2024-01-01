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

internal UI_Box *ui_begin_named_row(Str8 string);
internal Void    ui_end_named_row(Void);
internal UI_Box *ui_begin_row(Void);
internal Void    ui_end_row(Void);

internal UI_Box *ui_begin_named_column(Str8 string);
internal Void    ui_end_named_column(Void);
internal UI_Box *ui_begin_column(Void);
internal Void    ui_end_column(Void);

#define ui_named_row(string) defer_loop(ui_begin_named_row(string), ui_end_named_row())
#define ui_row()             defer_loop(ui_begin_row(), ui_end_row())

#define ui_named_column(string) defer_loop(ui_begin_named_column(string), ui_end_named_column())
#define ui_column()             defer_loop(ui_begin_column(), ui_end_column())

#endif //UI_BASIC_WIDGETS_H
