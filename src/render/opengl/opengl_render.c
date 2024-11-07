#include "src/render/opengl/opengl_vert.glsl.embed"
#include "src/render/opengl/opengl_frag.glsl.embed"

typedef struct OpenGL_State OpenGL_State;
struct OpenGL_State
{
    Arena *permanent_arena;

    R_RenderStats stats;

    Vec2U32 client_area;

    GLuint program;
    GLuint vao;
    GLint uniform_projection_location;
    GLint uniform_sampler_location;

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

internal R_Texture
r_texture_zero(Void)
{
    R_Texture result = { 0 };
    return result;
}

internal GLuint
opengl_texture_id_from_handle(R_Texture handle)
{
    GLuint result = (GLuint) handle.u64[0];
    return result;
}

internal Vec2U32
opengl_texture_size_from_handle(R_Texture handle)
{
    Vec2U32 result = v2u32(
        (U32) handle.u64[1],
        (U32) handle.u64[2]
    );
    return result;
}

internal B32
r_texture_equal(R_Texture a, R_Texture b)
{
    GLuint a_texture = opengl_texture_id_from_handle(a);
    GLuint b_texture = opengl_texture_id_from_handle(b);
    B32 result = a_texture == b_texture;
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
r_init(Void)
{
#if !BUILD_MODE_RELEASE
    glDebugMessageCallback(&opengl_debug_output, NULL);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

    opengl_state.permanent_arena = arena_create("OpenGLPerm");

    opengl_state.texture_update_queue = push_array_zero(opengl_state.permanent_arena, OpenGL_TextureUpdate, OPENGL_TEXTURE_UPDATE_QUEUE_SIZE);

    glCreateVertexArrays(1, &opengl_state.vao);

    opengl_vertex_array_instance_attribute(opengl_state.vao, 0, 2, GL_FLOAT, GL_FALSE, member_offset(R_Shape, min), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 1, 2, GL_FLOAT, GL_FALSE, member_offset(R_Shape, max), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 2, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(R_Shape, colors[0]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 3, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(R_Shape, colors[1]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 4, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(R_Shape, colors[2]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 5, 4, GL_FLOAT, GL_FALSE, (GLuint) member_offset(R_Shape, colors[3]), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 6, 4, GL_FLOAT, GL_FALSE, member_offset(R_Shape, radies), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 7, 1, GL_FLOAT, GL_FALSE, member_offset(R_Shape, softness), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 8, 1, GL_FLOAT, GL_FALSE, member_offset(R_Shape, border_thickness), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 9, 1, GL_FLOAT, GL_FALSE, member_offset(R_Shape, omit_texture), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 10, 1, GL_FLOAT, GL_FALSE, member_offset(R_Shape, is_subpixel_text), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 11, 1, GL_FLOAT, GL_FALSE, member_offset(R_Shape, use_nearest), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 12, 2, GL_FLOAT, GL_FALSE, member_offset(R_Shape, min_uv), 0);
    opengl_vertex_array_instance_attribute(opengl_state.vao, 13, 2, GL_FLOAT, GL_FALSE, member_offset(R_Shape, max_uv), 0);

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
r_begin(Void)
{
    opengl_state.client_area = gfx_get_window_client_area();

    glViewport(0, 0, (GLsizei) opengl_state.client_area.width, (GLsizei) opengl_state.client_area.height);

    Mat4F32 projection = m4f32_ortho(0.0f, (F32) opengl_state.client_area.width, (F32) opengl_state.client_area.height, 0.0f, 1.0f, -1.0f);
    glProgramUniformMatrix4fv(opengl_state.program, opengl_state.uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);
}

internal Void
r_submit(R_BatchList batches) {
    profile_begin_function();

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

    for (R_Batch *batch = batches.first; batch; batch = batch->next)
    {
        GLsizei width  = (GLsizei) (batch->clip_rect.max.x - batch->clip_rect.min.x);
        GLsizei height = (GLsizei) (batch->clip_rect.max.y - batch->clip_rect.min.y);
        if (width > 0 && height > 0)
        {
            U64 byte_size = batch->shapes.shape_count * sizeof(R_Shape);

            // NOTE(simon): Create buffer for geometry.
            GLuint vbo = 0;
            glCreateBuffers(1, &vbo);
            glNamedBufferData(vbo, (GLsizeiptr) byte_size, 0, GL_STREAM_DRAW);

            // NOTE(simon): Fill with shape data.
            {
                U8 *mapped_buffer = glMapNamedBuffer(vbo, GL_WRITE_ONLY);
                U8 *ptr = mapped_buffer;
                for (R_ShapeChunk *chunk = batch->shapes.first; chunk; chunk = chunk->next)
                {
                    memory_copy(ptr, chunk->shapes, chunk->count * sizeof(*chunk->shapes));
                    ptr += chunk->count * sizeof(*chunk->shapes);
                }
                glUnmapNamedBuffer(vbo);
            }

            // NOTE(simon): Connect buffer to the VAO.
            glVertexArrayVertexBuffer(opengl_state.vao, 0, vbo, 0, sizeof(R_Shape));

            // NOTE(simon): OpenGL has its origin in the lower left corner, not the
            // top left like we have, hence the weirdness with the y-coordinate.
            glScissor(
                (GLint) batch->clip_rect.min.x,
                (GLint) ((F32) opengl_state.client_area.height - batch->clip_rect.max.y),
                width,
                height
            );

            // NOTE(simon): Set active texture or deactivate it if we have none.
            GLuint texture = opengl_texture_id_from_handle(batch->texture);
            glBindTextureUnit(0, texture);

            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->shapes.shape_count);

            // NOTE(simon): Cleanup.
            glDeleteBuffers(1, &vbo);
        }
    }

    profile_end_function();
}

internal Void
r_end(Void)
{
    profile_begin_function();

    gfx_swap_buffers();

    profile_end_function();
}

internal R_Texture
r_create_texture_from_bitmap(Void *data, U32 width, U32 height, R_ColorSpace color_space)
{
    R_Texture result = {0};

    GLuint texture = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    GLenum internalformat = 0;
    switch (color_space)
    {
        case R_ColorSpace_sRGB:
            internalformat = GL_RGBA8;
            break;
        case R_ColorSpace_Linear:
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
r_destroy_texture(R_Texture handle)
{
    GLuint texture = opengl_texture_id_from_handle(handle);
    if (texture)
    {
        glDeleteTextures(1, &texture);
    }
}

internal Void
r_update_texture(R_Texture handle, Void *memory, U32 width, U32 height, U32 offset)
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

        Vec2U32 size = opengl_texture_size_from_handle(handle);

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

internal R_RenderStats
r_get_stats(Void)
{
    return opengl_state.stats;
}
