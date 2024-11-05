// NOTE(simon): Initialization

internal Void
draw_init(Void)
{
}

// NOTE(simon): Frame markers

internal Void
draw_begin_frame(Void)
{
    // NOTE(simon): Empty for now.
}

internal Void
draw_submit(Void)
{
    // NOTE(simon): Empty for now.
}

// NOTE(simon): Draw commands

internal Render_RectInstance *
draw_rect_(Vec2F32 min, Vec2F32 max, Draw_RectParams *parameters)
{
    Render_RectParams render_params = {
        .color            = parameters->color,
        .radius           = parameters->radius,
        .softness         = parameters->softness,
        .border_thickness = parameters->border_thickness,
        .slice            = parameters->slice,
        .is_subpixel_text = parameters->is_subpixel_text,
        .use_nearest      = parameters->use_nearest,
    };

    Render_RectInstance *result = render_rect_(min, max, &render_params);
    return result;
}

internal Void
draw_push_clip(Vec2F32 min, Vec2F32 max)
{
    render_push_clip(min, max, true);
}

internal Void
draw_pop_clip(Void)
{
    render_pop_clip();
}

internal Void
draw_character_internal(Vec2F32 min, U32 codepoint, Render_Font *font, Vec4F32 color)
{
    render_character_internal(min, codepoint, font, color);
}

internal Void
draw_text_internal(Vec2F32 min, Str8 text, Render_Font *font, Vec4F32 color)
{
    render_text_internal(min, text, font, color);
}

internal Vec2F32
draw_measure_character(Render_Font *font, U32 codepoint)
{
    return render_measure_character(font, codepoint);
}

internal Vec2F32
draw_measure_text(Render_Font *font, Str8 text)
{
    return render_measure_text(font, text);
}

internal Vec2F32
draw_measure_text_length(Render_Font *font, Str8 text, U64 length)
{
    return draw_measure_text(font, str8_prefix(text, length));
}
