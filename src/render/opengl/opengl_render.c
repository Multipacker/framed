internal GLuint
opengl_create_shader(Str8 path, GLenum shader_type)
{
	GLuint shader = glCreateShader(shader_type);
	Arena_Temporary scratch = arena_get_scratch(0, 0);

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

	arena_release_scratch(scratch);
	return(shader);
}

internal GLuint
opengl_create_program(GLuint *shaders, U32 shader_count)
{
	GLuint program = glCreateProgram();

	for (U32 i = 0; i < shader_count; ++i) {
		glAttachShader(program, shaders[i]);
	}

	glLinkProgram(program);

	for (U32 i = 0; i < shader_count; ++i) {
		glDetachShader(program, shaders[i]);
		glDeleteShader(shaders[i]);
	}

	GLint link_status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &link_status);
	if (!link_status)
	{
		Arena_Temporary scratch = arena_get_scratch(0, 0);

		GLint log_length = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);

		GLchar *raw_log = push_array(scratch.arena, GLchar, (U64) log_length);
		glGetProgramInfoLog(program, log_length, 0, raw_log);

		Str8 log = str8((U8 *) raw_log, (U64) log_length);

		fprintf(stderr, "ERROR: Could not link program. Program log:\n%.*s\n", str8_expand(log));

		glDeleteProgram(program);
		program = 0;

		arena_release_scratch(scratch);
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

internal Renderer
render_init(Gfx_Context *gfx)
{
	Renderer renderer = { 0 };

	renderer.gfx = gfx;
	renderer.frame_arena = arena_create();

	glEnable(GL_FRAMEBUFFER_SRGB);

	glCreateBuffers(1, &renderer.vbo);
	glNamedBufferData(renderer.vbo, OPENGL_BATCH_SIZE * sizeof(R_RectInstance), 0, GL_DYNAMIC_DRAW);

	glCreateVertexArrays(1, &renderer.vao);

	opengl_vertex_array_instance_attribute(renderer.vao, 0, 2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, min), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 1, 2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, max), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 2, 4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, colors[0]), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 3, 4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, colors[1]), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 4, 4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, colors[2]), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 5, 4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, colors[3]), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 6, 4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, radies), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 7, 1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, softness), 0);
	opengl_vertex_array_instance_attribute(renderer.vao, 8, 1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, border_thickness), 0);

	glVertexArrayVertexBuffer(renderer.vao, 0, renderer.vbo, 0, sizeof(R_RectInstance));

	GLuint shaders[] = {
		opengl_create_shader(str8_lit("src/render/opengl/shader.vert"), GL_VERTEX_SHADER),
		opengl_create_shader(str8_lit("src/render/opengl/shader.frag"), GL_FRAGMENT_SHADER),
	};
	renderer.program = opengl_create_program(shaders, array_count(shaders));

	renderer.uniform_projection_location = glGetUniformLocation(renderer.program, "uniform_projection");

	// NOTE(simon): We only need to set these once as we don't change them anywhere
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glUseProgram(renderer.program);
	glBindVertexArray(renderer.vao);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return(renderer);
}

internal Void
render_begin(Renderer *renderer)
{
	Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);

	glViewport(0, 0, (GLsizei) client_area.width, (GLsizei) client_area.height);

	Mat4F32 projection = m4f32_ortho(0.0f, (F32) client_area.width, (F32) client_area.height, 0.0f, 1.0f, -1.0f);
	glProgramUniformMatrix4fv(renderer->program, renderer->uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);
}

internal Void
render_end(Renderer *renderer)
{
	glClear(GL_COLOR_BUFFER_BIT);

	for (OpenGL_Batch *batch = renderer->batches.first; batch; batch = batch->next)
	{
		glNamedBufferSubData(renderer->vbo, 0, batch->size * sizeof(R_RectInstance), batch->rects);
		glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, (GLsizei) batch->size);
	}

	arena_pop_to(renderer->frame_arena, 0);
	renderer->batches.first       = 0;
	renderer->batches.last        = 0;
	renderer->batches.rect_count  = 0;
	renderer->batches.batch_count = 0;

	gfx_swap_buffers(renderer->gfx);
}

internal R_RectInstance *
render_rect_(Renderer *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params)
{
	OpenGL_Batch *batch = renderer->batches.last;
	if (!batch || batch->size >= OPENGL_BATCH_SIZE)
	{
		batch = push_struct_zero(renderer->frame_arena, OpenGL_Batch);
		dll_push_back(renderer->batches.first, renderer->batches.last, batch);
		++renderer->batches.batch_count;
	}

	R_RectInstance *rect = &batch->rects[batch->size++];
	rect->min              = min;
	rect->max              = max;
	rect->colors[0]        = params->color;
	rect->colors[1]        = params->color;
	rect->colors[2]        = params->color;
	rect->colors[3]        = params->color;
	rect->radies[0]        = params->radius;
	rect->radies[1]        = params->radius;
	rect->radies[2]        = params->radius;
	rect->radies[3]        = params->radius;
	rect->softness         = params->softness;
	rect->border_thickness = params->border_thickness;

	++renderer->batches.rect_count;

	return(rect);
}

internal Void
render_push_clip(Renderer *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
}

internal Void
render_pop_clip(Renderer *renderer)
{
}

internal R_RenderStats
render_get_stats(Renderer *renderer)
{
	R_RenderStats result = { 0 };
	return(result);
}
