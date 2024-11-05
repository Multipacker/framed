internal Render_TextureSlice
render_slice_from_texture(Render_Texture texture, RectF32 uv)
{
    Render_TextureSlice result = {uv, texture};
    return (result);
}

internal Render_TextureSlice
render_slice_from_texture_region(Render_Texture texture, RectU32 region)
{
#if 0
    RectF32 uv = rectf32_from_rectu32(region);
    Render_TextureSlice result = { region, texture };
    return(result);
#endif
    Render_TextureSlice result = {0};
    return (result);
}

internal Render_TextureSlice
render_create_texture_slice(Str8 path)
{
    Render_TextureSlice result;
    result.texture    = render_create_texture(path);
    result.region.min = v2f32(0, 0);
    result.region.max = v2f32(1, 1);
    return (result);
}

internal F32
f32_srgb_to_linear(F32 value)
{
    F32 result = 0.0f;
    if (value < 0.04045f)
    {
        result = value / 12.92f;
    }
    else
    {
        result = f32_pow((value + 0.055f) / 1.055f, 2.4f);
    }
    return (result);
}

internal Vec4F32
vec4f32_srgb_to_linear(Vec4F32 srgb)
{
    Vec4F32 result = v4f32(
        f32_srgb_to_linear(srgb.r),
        f32_srgb_to_linear(srgb.g),
        f32_srgb_to_linear(srgb.b),
        srgb.a
    );
    return (result);
}

internal F32
f32_linear_to_srgb(F32 value)
{
    F32 result = 0.0f;
    if (value < 0.0031308f)
    {
        result = value * 12.92f;
    }
    else
    {
        result = 1.055f * f32_pow(value, 1.0f / 2.4f) - 0.055f;
    }
    return (result);
}

internal Vec4F32
vec4f32_linear_to_srgb(Vec4F32 linear)
{
    Vec4F32 result = v4f32(
        f32_linear_to_srgb(linear.r),
        f32_linear_to_srgb(linear.g),
        f32_linear_to_srgb(linear.b),
        linear.a
    );
    return (result);
}

internal Render_RenderStats
render_get_stats(Render_Context *renderer)
{
    return (renderer->render_stats[1]);
}

internal Render_Context *
render_init(Void)
{
    Arena *arena              = arena_create("RenderPerm");
    Render_Context *renderer  = push_struct(arena, Render_Context);
    renderer->permanent_arena = arena;
    renderer->frame_arena     = arena_create("RenderFrame");
    render_backend_init(renderer);

    render_font_init();

    return renderer;
}

internal Void
render_begin(Render_Context *renderer)
{
    render_backend_begin(renderer);
}

internal Void
render_end(Render_Context *renderer)
{
    profile_begin_function();
    render_backend_end(renderer);
    renderer->frame_index++;
    render_font_end_frame();
    arena_pop_to(renderer->frame_arena, 0);
    profile_end_function();
}

internal Render_Texture
render_create_texture(Str8 path)
{
    Render_Texture result = {0};
    Str8 contents         = {0};

    Arena_Temporary scratch = get_scratch(0, 0);

    if (os_file_read(scratch.arena, path, &contents))
    {
        Image image = {0};
        if (image_load(scratch.arena, contents, &image))
        {
            result = render_create_texture_from_bitmap(image.pixels, image.width, image.height, image.color_space);
        }
        else
        {
            // TODO(simon): Could not load image data.
            log_error("Could not load image '%" PRISTR8 "'", str8_expand(path));
        }
    }
    else
    {
        // TODO(simon): Could not read file.
        log_error("Could not load image '%" PRISTR8 "'", str8_expand(path));
    }

    release_scratch(scratch);
    return (result);
}
