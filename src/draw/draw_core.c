typedef struct Draw_State Draw_State;
struct Draw_State
{
    Render_Context *renderer;
};

global Draw_State draw_state;

// NOTE(simon): Initialization

internal Void
draw_init(Render_Context *renderer)
{
    draw_state.renderer = renderer;
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

    Render_RectInstance *result = render_rect_(draw_state.renderer, min, max, &render_params);
    return result;
}

internal Void
draw_push_clip(Vec2F32 min, Vec2F32 max)
{
    render_push_clip(draw_state.renderer, min, max, true);
}

internal Void
draw_pop_clip(Void)
{
    render_pop_clip(draw_state.renderer);
}
