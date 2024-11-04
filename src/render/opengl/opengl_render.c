#include "src/render/opengl/opengl_vert.glsl.embed"
#include "src/render/opengl/opengl_frag.glsl.embed"

typedef struct OpenGL_State OpenGL_State;
struct OpenGL_State
{
    OpenGL_BatchList batches;
    Vec2U32 client_area;

    GLuint program;
    GLuint vbo;
    GLuint vao;
    GLint uniform_projection_location;
    GLint uniform_sampler_location;

    OpenGL_ClipNode *clip_stack;

    OpenGL_TextureUpdate *texture_update_queue;
    U32 volatile texture_update_write_index;
    U32 volatile texture_update_read_index;
};

global OpenGL_State opengl_state;

internal Void
opengl_debug_output(GLenum source, GLenum type, U32 id, GLenum severity, GLsizei length, const char *message, const Void *userParam)
{
    // NOTE(hampus): We do not care about these warnings
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
    {
        return;
    }

    Str8 source_string = {0};
    switch (source)
    {
        case GL_DEBUG_SOURCE_API:
            source_string = str8_lit("Source: API");
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_string = str8_lit("Source: Window System");
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_string = str8_lit("Source: Shader Compiler");
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_string = str8_lit("Source: Third Party");
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_string = str8_lit("Source: Application");
            break;
        case GL_DEBUG_SOURCE_OTHER:
            source_string = str8_lit("Source: Other");
            break;
    }

    Str8 type_string = {0};
    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:
            type_string = str8_lit("Type: Error");
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_string = str8_lit("Type: Deprecated Behaviour");
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_string = str8_lit("Type: Undefined Behaviour");
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_string = str8_lit("Type: Portability");
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_string = str8_lit("Type: Performance");
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_string = str8_lit("Type: Marker");
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            type_string = str8_lit("Type: Push Group");
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            type_string = str8_lit("Type: Pop Group");
            break;
        case GL_DEBUG_TYPE_OTHER:
            type_string = str8_lit("Type: Other");
            break;
    }

    Str8 severity_string = {0};
    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:
            severity_string = str8_lit("Severity: high");
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_string = str8_lit("Severity: medium");
            break;
        case GL_DEBUG_SEVERITY_LOW:
            severity_string = str8_lit("Severity: low");
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_string = str8_lit("Severity: notification");
            break;
    }

    log_error("OpenGL: Debug message (%d): %s. %" PRISTR8 ", %" PRISTR8 ", %" PRISTR8, id, message, str8_expand(source_string), str8_expand(type_string), str8_expand(severity_string));

    if (severity == GL_DEBUG_SEVERITY_HIGH)
    {
        assert(false);
    }
}

internal GLuint
opengl_texture_id_from_handle(Render_Texture handle)
{
    GLuint result = handle.u64[0];
    return result;
}

internal Vec2S32
opengl_texture_size_from_handle(Render_Texture handle)
{
    Vec2S32 result = v2s32(
        handle.u64[1],
        handle.u64[2]
    );
    return result;
}

internal GLuint
opengl_create_shader(Str8 source, GLenum shader_type)
{
    GLuint shader           = glCreateShader(shader_type);
    Arena_Temporary scratch = get_scratch(0, 0);

    const GLchar *source_data = (const GLchar *) source.data;
    GLint source_size         = (GLint) source.size;

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

        fprintf(stderr, "ERROR: Could not compile shader. Shader log:\n%.*s\n", str8_expand(log));

        glDeleteShader(shader);
        shader = 0;
    }

    release_scratch(scratch);
    return (shader);
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
    glVertexArrayAttribFormat(vaobj, attribindex, size, type, normalized, relativeoffset);
    glVertexArrayAttribBinding(vaobj, attribindex, bindingindex);
    glVertexArrayBindingDivisor(vaobj, attribindex, 1);
    glEnableVertexArrayAttrib(vaobj, attribindex);
}

internal Void
render_backend_init(Render_Context *renderer)
{
#if !BUILD_MODE_RELEASE
    glDebugMessageCallback(&opengl_debug_output, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    opengl_state.texture_update_queue = push_array_zero(renderer->permanent_arena, OpenGL_TextureUpdate, OPENGL_TEXTURE_UPDATE_QUEUE_SIZE);

    glCreateBuffers(1, &opengl_state.vbo);
    glNamedBufferData(opengl_state.vbo, OPENGL_BATCH_SIZE * sizeof(Render_RectInstance), 0, GL_DYNAMIC_DRAW);

    glCreateVertexArrays(1, &opengl_state.vao);

    opengl_vertex_array_instance_attribute(opengl_state.vao, 0, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, min), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 1, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, max), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 2, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[0]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 3, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[1]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 4, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[2]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 5, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(Render_RectInstance, colors[3]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 6, 4, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, radies), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 7, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, softness), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 8, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, border_thickness), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 9, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, omit_texture), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 10, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, is_subpixel_text), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 11, 1, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, use_nearest), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 12, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, min_uv), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 13, 2, GL_FLOAT, GL_FALSE, member_offset(Render_RectInstance, max_uv), 0);

    glVertexArrayVertexBuffer(opengl_state.vao, 0, opengl_state.vbo, 0, sizeof(Render_RectInstance));

    arena_scratch(0, 0)
    {
        Str8 shader_vert = framed_embed_unpack(scratch, embed_opengl_vert_data, embed_opengl_vert_size);
        Str8 shader_frag = framed_embed_unpack(scratch, embed_opengl_frag_data, embed_opengl_frag_size);

        GLuint shaders[] = {
            opengl_create_shader(shader_vert, GL_VERTEX_SHADER),
            opengl_create_shader(shader_frag, GL_FRAGMENT_SHADER),
        };
        opengl_state.program = opengl_create_program(shaders, array_count(shaders));
    }

    opengl_state.uniform_projection_location = glGetUniformLocation(opengl_state.program, "uniform_projection");
    opengl_state.uniform_sampler_location    = glGetUniformLocation(opengl_state.program, "uniform_sampler");

    // NOTE(simon): We only need to set these once as we don't change them anywhere
    Vec4F32 background = vec4f32_srgb_to_linear(v4f32(0.1f, 0.2f, 0.3f, 1.0f));
    glClearColor(background.r, background.g, background.b, background.a);
    glUseProgram(opengl_state.program);
    glBindVertexArray(opengl_state.vao);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

internal Void
render_backend_begin(Render_Context *renderer)
{
    opengl_state.client_area = gfx_get_window_client_area();

    glViewport(0, 0, (GLsizei) opengl_state.client_area.width, (GLsizei) opengl_state.client_area.height);

    Mat4F32 projection = m4f32_ortho(0.0f, (F32) opengl_state.client_area.width, (F32) opengl_state.client_area.height, 0.0f, 1.0f, -1.0f);
    glProgramUniformMatrix4fv(opengl_state.program, opengl_state.uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);

    // NOTE(simon): Push a clip rect for the entire screen so that there is
    // always at least on clip rect in the stack.
    render_push_clip(renderer, v2f32(0.0f, 0.0f), v2f32((F32) opengl_state.client_area.width, (F32) opengl_state.client_area.height), false);
}

internal Void
render_backend_end(Render_Context *renderer)
{
    // NOTE(simon): Perform texture updates.
    while (opengl_state.texture_update_write_index - opengl_state.texture_update_read_index != 0)
    {
        OpenGL_TextureUpdate *waiting_update = &opengl_state.texture_update_queue[opengl_state.texture_update_read_index & OPENGL_TEXTURE_UPDATE_QUEUE_MASK];

        while (!waiting_update->is_valid)
        {
            // NOTE(simon): Busy wait for the entry to become valid.
        }
        waiting_update->is_valid    = false;
        OpenGL_TextureUpdate update = *waiting_update;
        memory_fence();
        ++opengl_state.texture_update_read_index;

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

    glProgramUniform1i(opengl_state.program, opengl_state.uniform_sampler_location, 0);

    for (OpenGL_Batch *batch = opengl_state.batches.first; batch; batch = batch->next)
    {
        glNamedBufferSubData(opengl_state.vbo, 0, batch->size * sizeof(Render_RectInstance), batch->rects);
        RectF32 clip_rect = batch->clip_node->rect;

        // NOTE(simon): OpenGL has its origin in the lower left corner, not the
        // top left like we have, hence the weirdness with the y-coordinate.
        glScissor(
            (GLint) clip_rect.min.x,
            (GLint) ((F32) opengl_state.client_area.height - clip_rect.max.y),
            (GLsizei) (clip_rect.max.x - clip_rect.min.x),
            (GLsizei) (clip_rect.max.y - clip_rect.min.y)
        );

        GLuint texture = opengl_texture_id_from_handle(batch->texture);
        if (texture)
        {
            glBindTextureUnit(0, texture);
        }

        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->size);
    }

    // NOTE(simon): Update stats
    renderer->render_stats[0].rect_count  = opengl_state.batches.rect_count;
    renderer->render_stats[0].batch_count = opengl_state.batches.batch_count;

    opengl_state.batches.first       = 0;
    opengl_state.batches.last        = 0;
    opengl_state.batches.rect_count  = 0;
    opengl_state.batches.batch_count = 0;
    opengl_state.clip_stack          = 0;

    swap(renderer->render_stats[0], renderer->render_stats[1], Render_RenderStats);
    memory_zero_struct(&renderer->render_stats[0]);
    gfx_swap_buffers();
}

internal OpenGL_Batch *
opengl_create_batch(Render_Context *renderer)
{
    // NOTE(simon): No need to clear everything to zero, manually set the
    // parameters we care about.
    OpenGL_Batch *result = push_struct(renderer->frame_arena, OpenGL_Batch);

    result->size      = 0;
    result->clip_node = opengl_state.clip_stack;
    result->texture   = (Render_Texture){0};
    dll_push_back(opengl_state.batches.first, opengl_state.batches.last, result);
    ++opengl_state.batches.batch_count;

    return (result);
}

// TODO(simon): Test performance without pruning batches and rectangles once we
// are rendering more complicated scenes.
internal Render_RectInstance *
render_rect_(Render_Context *renderer, Vec2F32 min, Vec2F32 max, Render_RectParams *params)
{
    assert(opengl_state.clip_stack);

    Render_RectInstance *result = &render_rect_instance_null;

    // NOTE(simon): Account for softness.
    RectF32 expanded_area = rectf32(
        v2f32_sub_f32(min, params->softness),
        v2f32_add_f32(max, params->softness)
    );

    // NOTE(simon): Is the rectangle completly outside of the current clip rect?
    if (!rectf32_overlaps(expanded_area, opengl_state.clip_stack->rect))
    {
        return (result);
    }

    OpenGL_Batch *batch = opengl_state.batches.last;

    if (!batch || batch->size >= OPENGL_BATCH_SIZE)
    {
        batch = opengl_create_batch(renderer);
    }

    B32 is_different_clip   = (batch->clip_node != opengl_state.clip_stack);
    B32 inside_current_clip = rectf32_contains_rectf32(opengl_state.clip_stack->rect, expanded_area);
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

    min.x = f32_round(min.x);
    min.y = f32_round(min.y);
    max.x = f32_round(max.x);
    max.y = f32_round(max.y);

    result                   = &batch->rects[batch->size++];
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

    ++opengl_state.batches.rect_count;

    return (result);
}

internal Void
render_push_clip(Render_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    OpenGL_ClipNode *node = push_struct(renderer->frame_arena, OpenGL_ClipNode);

    if (clip_to_parent)
    {
        assert(opengl_state.clip_stack);
        RectF32 parent = opengl_state.clip_stack->rect;

        node->rect.min.x = f32_clamp(parent.min.x, min.x, parent.max.x);
        node->rect.min.y = f32_clamp(parent.min.y, min.y, parent.max.y);
        node->rect.max.x = f32_clamp(parent.min.x, max.x, parent.max.x);
        node->rect.max.y = f32_clamp(parent.min.y, max.y, parent.max.y);
    }
    else
    {
        node->rect = rectf32(min, max);
    }

    stack_push(opengl_state.clip_stack, node);
}

internal Void
render_pop_clip(Render_Context *renderer)
{
    stack_pop(opengl_state.clip_stack);
}

internal Render_Texture
render_create_texture(Render_Context *renderer, Str8 path)
{
    Render_Texture result = {0};

    Arena_Temporary scratch = get_scratch(0, 0);

    Str8 contents = {0};
    if (os_file_read(scratch.arena, path, &contents))
    {
        Image image = {0};
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

internal Render_Texture
render_create_texture_from_bitmap(Render_Context *renderer, Void *data, U32 width, U32 height, Render_ColorSpace color_space)
{
    Render_Texture result = {0};

    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    GLenum internalformat = 0;
    switch (color_space)
    {
        case Render_ColorSpace_sRGB:
            internalformat = GL_RGBA8;
            break;
        case Render_ColorSpace_Linear:
            internalformat = GL_SRGB8_ALPHA8;
            break;
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

    return (result);
}

internal Void
render_destroy_texture(Render_Context *renderer, Render_Texture handle)
{
    GLuint texture = opengl_texture_id_from_handle(handle);
    if (texture)
    {
        glDeleteTextures(1, &texture);
    }
}

internal Void
render_update_texture(Render_Context *renderer, Render_Texture handle, Void *memory, U32 width, U32 height, U32 offset)
{
    GLuint texture = opengl_texture_id_from_handle(handle);
    if (texture)
    {
        U32 queue_index = u32_atomic_add(&opengl_state.texture_update_write_index, 1);
        while (queue_index - opengl_state.texture_update_read_index >= OPENGL_TEXTURE_UPDATE_QUEUE_SIZE)
        {
            // NOTE(simon): The queue is full, so busy wait. This should not be
            // that common.
        }

        OpenGL_TextureUpdate *update = &opengl_state.texture_update_queue[queue_index & OPENGL_TEXTURE_UPDATE_QUEUE_MASK];

        Vec2S32 size = opengl_texture_size_from_handle(handle);

        update->texture = texture;
        update->x       = (GLint) (offset % size.width);
        update->y       = (GLint) (offset / size.width);
        update->width   = (GLsizei) width;
        update->height  = (GLsizei) height;
        update->data    = memory;

        memory_fence();

        update->is_valid = true;
    }
}
