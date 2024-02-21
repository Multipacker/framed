internal GLuint
opengl_create_shader(Str8 path, GLenum shader_type)
{
    GLuint shader = glCreateShader(shader_type);
    Arena_Temporary scratch = get_scratch(0, 0);

    Str8 source = { 0 };
    if (os_file_read(scratch.arena, path, &source))
    {
        const GLchar *source_data = (const GLchar *) source.data;
        GLint         source_size = (GLint) source.size;

        glShaderSource(shader, 1, &source_data, &source_size);

        glCompileShader(shader);

        GLint compile_status = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compile_status);
        if (!compile_status)
        {
            GLint log_length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);

            GLchar *raw_log = push_array(scratch.arena, GLchar, (U64) log_length);
            glGetShaderInfoLog(shader, log_length, 0, raw_log);

            Str8 log = str8((U8 *) raw_log, (U64) log_length);

            fprintf(stderr, "ERROR: Could not compile %.*s. Shader log:\n%.*s\n", str8_expand(path), str8_expand(log));

            glDeleteShader(shader);
            shader = 0;
        }
    }
    else
    {
        fprintf(stderr, "ERROR: Could not load file %.*s\n", str8_expand(path));

        glDeleteShader(shader);
        shader = 0;
    }

    release_scratch(scratch);
    return(shader);
}

internal GLuint
opengl_create_program(GLuint *shaders, U32 shader_count)
{
    GLuint program = glCreateProgram();

    for (U32 i = 0; i < shader_count; ++i)
    {
        glAttachShader(program, shaders[i]);
    }

    glLinkProgram(program);

    for (U32 i = 0; i < shader_count; ++i)
    {
        glDetachShader(program, shaders[i]);
        glDeleteShader(shaders[i]);
    }

    GLint link_status = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &link_status);
    if (!link_status)
    {
        Arena_Temporary scratch = get_scratch(0, 0);

        GLint log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

        GLchar *raw_log = push_array(scratch.arena, GLchar, (U64) log_length);
        glGetProgramInfoLog(program, log_length, 0, raw_log);

        Str8 log = str8((U8 *) raw_log, (U64) log_length);

        fprintf(stderr, "ERROR: Could not link program. Program log:\n%.*s\n", str8_expand(log));

        glDeleteProgram(program);
        program = 0;

        release_scratch(scratch);
    }

    return (program);
}

internal Void
opengl_vertex_array_instance_attribute(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset, GLuint bindingindex)
{
    glVertexArrayAttribFormat(vaobj,   attribindex, size, type, normalized, relativeoffset);
    glVertexArrayAttribBinding(vaobj,  attribindex, bindingindex);
    glVertexArrayBindingDivisor(vaobj, attribindex, 1);
    glEnableVertexArrayAttrib(vaobj,   attribindex);
}

internal Render_BackendContext *
render_backend_init(Render_Context *renderer)
{
    // NOTE(simon): The alignment is needed for atomic access within the struct.
    arena_align(renderer->permanent_arena, 8);
    renderer->backend = push_struct(renderer->permanent_arena, Render_BackendContext);
    Render_BackendContext *backend = renderer->backend;
    backend->texture_update_queue = push_array_zero(renderer->permanent_arena, OpenGL_TextureUpdate, OPENGL_TEXTURE_UPDATE_QUEUE_SIZE);

    glCreateBuffers(1, &backend->vbo);
    glNamedBufferData(backend->vbo, OPENGL_BATCH_SIZE * sizeof(Render_RectInstance), 0, GL_DYNAMIC_DRAW);

    glCreateVertexArrays(1, &backend->vao);

    opengl_vertex_array_instance_attribute(backend->vao, 0, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, min), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 1, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, max), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 2, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[0]), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 3, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[1]), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 4, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[2]), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 5, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[3]), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 6, 4, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, radies), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 7, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, softness), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 8, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, border_thickness), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 9, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, omit_texture), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 10, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, is_subpixel_text), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 11, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, use_nearest), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 12, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, min_uv), 0);
    opengl_vertex_array_instance_attribute(backend->vao, 13, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, max_uv), 0);

    glVertexArrayVertexBuffer(backend->vao, 0, backend->vbo, 0, sizeof(Render_RectInstance));

    GLuint shaders[] = {
        opengl_create_shader(str8_lit("src/render/opengl/shader.vert"), GL_VERTEX_SHADER),
        opengl_create_shader(str8_lit("src/render/opengl/shader.frag"), GL_FRAGMENT_SHADER),
    };
    backend->program = opengl_create_program(shaders, array_count(shaders));

    backend->uniform_projection_location = glGetUniformLocation(backend->program, "uniform_projection");
    backend->uniform_sampler_location    = glGetUniformLocation(backend->program, "uniform_sampler");

    // NOTE(simon): We only need to set these once as we don't change them anywhere
    Vec4F32 background = vec4f32_srgb_to_linear(v4f32(0.1f, 0.2f, 0.3f, 1.0f));
    glClearColor(background.r, background.g, background.b, background.a);
    glUseProgram(backend->program);
    glBindVertexArray(backend->vao);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return(backend);
}

internal Void
render_backend_begin(Render_Context *renderer)
{
    Render_BackendContext *backend = renderer->backend;
    backend->client_area = gfx_get_window_client_area(renderer->gfx);

    glViewport(0, 0, (GLsizei) backend->client_area.width, (GLsizei) backend->client_area.height);

    Mat4F32 projection = m4f32_ortho(0.0f, (F32) backend->client_area.width, (F32) backend->client_area.height, 0.0f, 1.0f, -1.0f);
    glProgramUniformMatrix4fv(backend->program, backend->uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);

    // NOTE(simon): Push a clip rect for the entire screen so that there is
    // always at least on clip rect in the stack.
    render_push_clip(renderer, v2f32(0.0f, 0.0f), v2f32((F32) backend->client_area.width, (F32) backend->client_area.height), false);
}

internal Void
render_backend_end(Render_Context *renderer)
{
    Render_BackendContext *backend = renderer->backend;

    // NOTE(simon): Perform texture updates.
    while (backend->texture_update_write_index - backend->texture_update_read_index != 0)
    {
        OpenGL_TextureUpdate *waiting_update = &backend->texture_update_queue[backend->texture_update_read_index & OPENGL_TEXTURE_UPDATE_QUEUE_MASK];

        while (!waiting_update->is_valid)
        {
            // NOTE(simon): Busy wait for the entry to become valid.
        }
        waiting_update->is_valid = false;
        OpenGL_TextureUpdate update = *waiting_update;
        memory_fence();
        ++backend->texture_update_read_index;

        glTextureSubImage2D(
            update.texture,
            0,
            update.x, update.y,
            update.width, update.height,
            GL_RGBA, GL_UNSIGNED_BYTE,
            update.data
        );
    }

    glDisable(GL_SCISSOR_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_SCISSOR_TEST);

    glProgramUniform1i(backend->program, backend->uniform_sampler_location, 0);

    for (OpenGL_Batch *batch = backend->batches.first; batch; batch = batch->next)
    {
        glNamedBufferSubData(backend->vbo, 0, batch->size * sizeof(Render_RectInstance), batch->rects);
        RectF32 clip_rect = batch->clip_node->rect;

        // NOTE(simon): OpenGL has its origin in the lower left corner, not the
        // top left like we have, hence the weirdness with the y-coordinate.
        glScissor(
            (GLint) clip_rect.min.x,
            (GLint) ((F32) backend->client_area.height - clip_rect.max.y),
            (GLsizei) (clip_rect.max.x - clip_rect.min.x),
            (GLsizei) (clip_rect.max.y - clip_rect.min.y)
        );

        if (batch->texture.u64[0])
        {
            GLuint opengl_texture = (GLuint) batch->texture.u64[0];
            glBindTextureUnit(0, opengl_texture);
        }

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->size);
    }

    // NOTE(simon): Update stats
    renderer->render_stats[0].rect_count  = backend->batches.rect_count;
    renderer->render_stats[0].batch_count = backend->batches.batch_count;

    backend->batches.first       = 0;
    backend->batches.last        = 0;
    backend->batches.rect_count  = 0;
    backend->batches.batch_count = 0;
    backend->clip_stack          = 0;

    swap(renderer->render_stats[0], renderer->render_stats[1], Render_RenderStats);
    memory_zero_struct(&renderer->render_stats[0]);
    gfx_swap_buffers(renderer->gfx);
}

internal OpenGL_Batch *
opengl_create_batch(Render_Context *renderer)
{
    Render_BackendContext *backend = renderer->backend;
    // NOTE(simon): No need to clear everything to zero, manually set the
    // parameters we care about.
    OpenGL_Batch *result = push_struct(renderer->frame_arena, OpenGL_Batch);

    result->size      = 0;
    result->clip_node = backend->clip_stack;
    result->texture   = (Render_Texture) { 0 };
    dll_push_back(backend->batches.first, backend->batches.last, result);
    ++backend->batches.batch_count;

    return(result);
}

// TODO(simon): Test performance without pruning batches and rectangles once we
// are rendering more complicated scenes.
internal Render_RectInstance *
render_rect_(Render_Context *renderer, Vec2F32 min, Vec2F32 max, Render_RectParams *params)
{
    Render_BackendContext *backend = renderer->backend;
    assert(backend->clip_stack);

    Render_RectInstance *result = &render_rect_instance_null;

    // NOTE(simon): Account for softness.
    RectF32 expanded_area = rectf32(
        v2f32_sub_f32(min, params->softness),
        v2f32_add_f32(max, params->softness)
    );

    // NOTE(simon): Is the rectangle completly outside of the current clip rect?
    if (!rectf32_overlaps(expanded_area, backend->clip_stack->rect))
    {
        return(result);
    }

    OpenGL_Batch *batch = backend->batches.last;

    if (!batch || batch->size >= OPENGL_BATCH_SIZE)
    {
        batch = opengl_create_batch(renderer);
    }

    B32 is_different_clip   = (batch->clip_node != backend->clip_stack);
    B32 inside_current_clip = rectf32_contains_rectf32(backend->clip_stack->rect, expanded_area);
    B32 inside_batch_clip   = rectf32_contains_rectf32(batch->clip_node->rect, expanded_area);
    if (is_different_clip && !(inside_current_clip && inside_batch_clip))
    {
        batch = opengl_create_batch(renderer);
    }

    if (
        batch->texture.u64[0] &&
        params->slice.texture.u64[0] &&
        batch->texture.u64[0] != params->slice.texture.u64[0]
    )
    {
        batch = opengl_create_batch(renderer);
    }

    // NOTE(simon): The batch either has the same texture, or none at all.
    if (params->slice.texture.u64[0])
    {
        batch->texture = params->slice.texture;
    }

    result = &batch->rects[batch->size++];
    result->min              = min;
    result->max              = max;
    result->min_uv           = params->slice.region.min;
    result->max_uv           = params->slice.region.max;
    result->colors[0]        = params->color;
    result->colors[1]        = params->color;
    result->colors[2]        = params->color;
    result->colors[3]        = params->color;
    result->radies[0]        = params->radius;
    result->radies[1]        = params->radius;
    result->radies[2]        = params->radius;
    result->radies[3]        = params->radius;
    result->softness         = params->softness;
    result->border_thickness = params->border_thickness;
    result->omit_texture     = (F32) (params->slice.texture.u64[0] == 0);
    result->is_subpixel_text = (F32) params->is_subpixel_text;
    result->use_nearest      = (F32) params->use_nearest;

    ++backend->batches.rect_count;

    return(result);
}

internal Void
render_push_clip(Render_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    Render_BackendContext *backend = renderer->backend;
    OpenGL_ClipNode *node = push_struct(renderer->frame_arena, OpenGL_ClipNode);

    if (clip_to_parent)
    {
        assert(backend->clip_stack);
        RectF32 parent = backend->clip_stack->rect;

        node->rect.min.x = f32_clamp(parent.min.x, min.x, parent.max.x);
        node->rect.min.y = f32_clamp(parent.min.y, min.y, parent.max.y);
        node->rect.max.x = f32_clamp(parent.min.x, max.x, parent.max.x);
        node->rect.max.y = f32_clamp(parent.min.y, max.y, parent.max.y);
    }
    else
    {
        node->rect = rectf32(min, max);
    }

    stack_push(backend->clip_stack, node);
}

internal Void
render_pop_clip(Render_Context *renderer)
{
    stack_pop(renderer->backend->clip_stack);
}

internal Render_Texture
render_create_texture(Render_Context *renderer, Str8 path)
{
    Render_Texture result = { 0 };

    Arena_Temporary scratch = get_scratch(0, 0);

    Str8 contents = { 0 };
    if (os_file_read(scratch.arena, path, &contents))
    {
        Image image = { 0 };
        if (image_load(scratch.arena, contents, &image))
        {
            result = render_create_texture_from_bitmap(
                renderer,
                image.pixels,
                image.width,
                image.height,
                image.color_space
            );
        }
        else
        {
            // TODO(simon): Could not load image data.
            log_error("Could not load image '%"PRISTR8"'", str8_expand(path));
        }
    }
    else
    {
        // TODO(simon): Could not read file.
        log_error("Could not load image '%"PRISTR8"'", str8_expand(path));
    }

    release_scratch(scratch);
    return(result);
}

internal Render_Texture
render_create_texture_from_bitmap(Render_Context *renderer, Void *data, U32 width, U32 height, Render_ColorSpace color_space)
{
    Render_Texture result = { 0 };

    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    GLenum internalformat = 0;
    switch (color_space)
    {
        case Render_ColorSpace_sRGB:   internalformat = GL_RGBA8;        break;
        case Render_ColorSpace_Linear: internalformat = GL_SRGB8_ALPHA8; break;
        invalid_case;
    }
    glTextureStorage2D(texture, 1, internalformat, (GLsizei) width, (GLsizei) height);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureSubImage2D(
        texture,
        0,
        0, 0,
        (GLsizei) width, (GLsizei) height,
        GL_RGBA, GL_UNSIGNED_BYTE,
        (const Void *) data
    );

    result.u64[0] = (U64) texture;
    result.u64[1] = (U64) width;
    result.u64[2] = (U64) height;

    return(result);
}

internal Void
render_destroy_texture(Render_Context *renderer, Render_Texture texture)
{
    if (texture.u64[0])
    {
        GLuint opengl_texture = (GLuint) texture.u64[0];
        glDeleteTextures(1, &opengl_texture);
    }
}

internal Void
render_update_texture(Render_Context *renderer, Render_Texture texture, Void *memory, U32 width, U32 height, U32 offset)
{
    if (texture.u64[0])
    {
        Render_BackendContext *backend = renderer->backend;

        U32 queue_index = u32_atomic_add(&backend->texture_update_write_index, 1);
        while (queue_index - backend->texture_update_read_index >= OPENGL_TEXTURE_UPDATE_QUEUE_SIZE)
        {
            // NOTE(simon): The queue is full, so busy wait. This should not be
            // that common.
        }

        OpenGL_TextureUpdate *update = &backend->texture_update_queue[queue_index & OPENGL_TEXTURE_UPDATE_QUEUE_MASK];

        update->texture = (GLuint) texture.u64[0];
        update->x       = (GLint) (offset % texture.u64[1]);
        update->y       = (GLint) (offset / texture.u64[1]);
        update->width   = (GLsizei) width;
        update->height  = (GLsizei) height;
        update->data    = memory;

        memory_fence();

        update->is_valid = true;
    }
}
