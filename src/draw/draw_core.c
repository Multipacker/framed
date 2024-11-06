// NOTE(simon): Initialization

internal Void
d_init(Void)
{
}

// NOTE(simon): Frame markers

internal Void
d_begin_frame(Void)
{
    // NOTE(simon): Empty for now.
}

internal Void
d_submit(Void)
{
    // NOTE(simon): Empty for now.
}

// NOTE(simon): Draw commands

internal R_RectInstance *
d_rect_(Vec2F32 min, Vec2F32 max, D_RectParams *parameters)
{
    R_RectParams r_params = {
        .color            = parameters->color,
        .radius           = parameters->radius,
        .softness         = parameters->softness,
        .border_thickness = parameters->border_thickness,
        .slice            = parameters->slice,
        .is_subpixel_text = parameters->is_subpixel_text,
        .use_nearest      = parameters->use_nearest,
    };

    R_RectInstance *result = r_rect_(min, max, &r_params);
    return result;
}

internal Void
d_push_clip(Vec2F32 min, Vec2F32 max)
{
    r_push_clip(min, max, true);
}

internal Void
d_pop_clip(Void)
{
    r_pop_clip();
}

internal Void
d_character_internal(Vec2F32 min, U32 codepoint, R_Font *font, Vec4F32 color)
{
    r_character_internal(min, codepoint, font, color);
}

internal Void
d_text_internal(Vec2F32 min, Str8 text, R_Font *font, Vec4F32 color)
{
    r_text_internal(min, text, font, color);
}

internal Vec2F32
d_measure_character(R_Font *font, U32 codepoint)
{
    return r_measure_character(font, codepoint);
}

internal Vec2F32
d_measure_text(R_Font *font, Str8 text)
{
    return r_measure_text(font, text);
}

internal Vec2F32
d_measure_text_length(R_Font *font, Str8 text, U64 length)
{
    return d_measure_text(font, str8_prefix(text, length));
}
