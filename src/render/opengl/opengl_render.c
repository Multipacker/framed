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

internal Renderer
render_init(Gfx_Context *gfx)
{
	Renderer renderer = { 0 };
	Arena_Temporary scratch = arena_get_scratch(0, 0);

	renderer.gfx = gfx;

	glCreateVertexArrays(1, &renderer.vao);

	GLuint vertex_shader   = opengl_create_shader(str8_lit("src/render/opengl/shader.vert"), GL_VERTEX_SHADER);
	GLuint fragment_shader = opengl_create_shader(str8_lit("src/render/opengl/shader.frag"), GL_FRAGMENT_SHADER);

	renderer.program = glCreateProgram();
	glAttachShader(renderer.program, vertex_shader);
	glAttachShader(renderer.program, fragment_shader);
	glLinkProgram(renderer.program);
	glDetachShader(renderer.program, vertex_shader);
	glDetachShader(renderer.program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	GLint link_status = 0;
	glGetProgramiv(renderer.program, GL_LINK_STATUS, &link_status);
	if (!link_status)
	{
		GLint log_length = 0;
		glGetProgramiv(renderer.program, GL_INFO_LOG_LENGTH, &log_length);

		GLchar *raw_log = push_array(scratch.arena, GLchar, (U64) log_length);
		glGetProgramInfoLog(renderer.program, log_length, 0, raw_log);

		Str8 log = str8((U8 *) raw_log, (U64) log_length);

		fprintf(stderr, "ERROR: Could not link program. Program log:\n%.*s\n", str8_expand(log));

		glDeleteProgram(renderer.program);
		renderer.program = 0;
	}

	arena_release_scratch(scratch);
	return(renderer);
}

internal Void
render_begin(Renderer *renderer)
{
	Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);

	glViewport(0, 0, (GLsizei) client_area.x, (GLsizei) client_area.y);
	glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glUseProgram(renderer->program);
	glBindVertexArray(renderer->vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 3);
}

internal Void
render_end(Renderer *renderer)
{
	gfx_swap_buffers(renderer->gfx);
}

internal R_RectInstance *
render_rect_(Renderer *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params)
{
	return(0);
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
