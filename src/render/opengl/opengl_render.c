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

internal R_BackendContext *
render_backend_init(R_Context *renderer)
{
    renderer->backend = push_struct(renderer->permanent_arena, R_BackendContext);
    R_BackendContext *backend = renderer->backend;
	glEnable(GL_FRAMEBUFFER_SRGB);

	glCreateBuffers(1, &backend->vbo);
	glNamedBufferData(backend->vbo, OPENGL_BATCH_SIZE * sizeof(R_RectInstance), 0, GL_DYNAMIC_DRAW);

	glCreateVertexArrays(1, &backend->vao);

	opengl_vertex_array_instance_attribute(backend->vao, 0,  2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, min), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 1,  2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, max), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 2,  4, GL_FLOAT, GL_FALSE, (GLuint)member_offset(R_RectInstance, colors[0]), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 3,  4, GL_FLOAT, GL_FALSE, (GLuint)member_offset(R_RectInstance, colors[1]), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 4,  4, GL_FLOAT, GL_FALSE, (GLuint)member_offset(R_RectInstance, colors[2]), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 5,  4, GL_FLOAT, GL_FALSE, (GLuint)member_offset(R_RectInstance, colors[3]), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 6,  4, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, radies), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 7,  1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, softness), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 8,  1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, border_thickness), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 9,  1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, omit_texture), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 10, 1, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, is_subpixel_text), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 11, 2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, min_uv), 0);
	opengl_vertex_array_instance_attribute(backend->vao, 12, 2, GL_FLOAT, GL_FALSE, member_offset(R_RectInstance, max_uv), 0);

	glVertexArrayVertexBuffer(backend->vao, 0, backend->vbo, 0, sizeof(R_RectInstance));

	GLuint shaders[] = {
		opengl_create_shader(str8_lit("src/render/opengl/shader.vert"), GL_VERTEX_SHADER),
		opengl_create_shader(str8_lit("src/render/opengl/shader.frag"), GL_FRAGMENT_SHADER),
	};
	backend->program = opengl_create_program(shaders, array_count(shaders));

	backend->uniform_projection_location = glGetUniformLocation(backend->program, "uniform_projection");
	backend->uniform_sampler_location    = glGetUniformLocation(backend->program, "uniform_sampler");

	// NOTE(simon): We only need to set these once as we don't change them anywhere
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glUseProgram(backend->program);
	glBindVertexArray(backend->vao);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC1_COLOR, GL_ONE_MINUS_SRC1_COLOR, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	return(backend);
}

internal Void
render_begin(R_Context *renderer)
{
    R_BackendContext *backend = renderer->backend;
	backend->client_area = gfx_get_window_client_area(renderer->gfx);

	glViewport(0, 0, (GLsizei) backend->client_area.width, (GLsizei) backend->client_area.height);

	Mat4F32 projection = m4f32_ortho(0.0f, (F32) backend->client_area.width, (F32) backend->client_area.height, 0.0f, 1.0f, -1.0f);
	glProgramUniformMatrix4fv(backend->program, backend->uniform_projection_location, 1, GL_FALSE, &projection.m[0][0]);

	// NOTE(simon): Push a clip rect for the entire screen so that there is
	// always at least on clip rect in the stack.
	render_push_clip(renderer, v2f32(0.0f, 0.0f), v2f32((F32) backend->client_area.width, (F32) backend->client_area.height), false);
}

internal Void
render_end(R_Context *renderer)
{
    R_BackendContext *backend = renderer->backend;
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	glEnable(GL_SCISSOR_TEST);

	glProgramUniform1i(backend->program, backend->uniform_sampler_location, 0);

	for (OpenGL_Batch *batch = backend->batches.first; batch; batch = batch->next)
	{
		glNamedBufferSubData(backend->vbo, 0, batch->size * sizeof(R_RectInstance), batch->rects);
		RectF32 clip_rect = batch->clip_node->rect;

		// NOTE(simon): OpenGL has its origin in the lower left corner, not the
		// top left like we have, hence the weirdness with the y-coordinate.
		glScissor(
			(GLint)   clip_rect.min.x,
			(GLint)   ((F32) backend->client_area.height - clip_rect.max.y),
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

	arena_pop_to(renderer->frame_arena, 0);
	swap(renderer->render_stats[0], renderer->render_stats[1], R_RenderStats);
	memory_zero_struct(&renderer->render_stats[0]);
	gfx_swap_buffers(renderer->gfx);
}

internal OpenGL_Batch *
opengl_create_batch(R_Context *renderer)
{
    R_BackendContext *backend = renderer->backend;
	// NOTE(simon): No need to clear everything to zero, manually set the
	// parameters we care about.
	OpenGL_Batch *result = push_struct(renderer->frame_arena, OpenGL_Batch);

	result->size      = 0;
	result->clip_node = backend->clip_stack;
	result->texture   = (R_Texture) { 0 };
	dll_push_back(backend->batches.first, backend->batches.last, result);
	++backend->batches.batch_count;

	return(result);
}

// TODO(simon): Test performance without pruning batches and rectangles once we
// are rendering more complicated scenes.
internal R_RectInstance *
render_rect_(R_Context *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params)
{
    R_BackendContext *backend = renderer->backend;
	assert(backend->clip_stack);

	R_RectInstance *result = &render_rect_instance_null;

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
	B32 inside_batch_clip   = rectf32_contains_rectf32(batch->clip_node->rect,     expanded_area);
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

	++backend->batches.rect_count;

	return(result);
}

internal Void
render_push_clip(R_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    R_BackendContext *backend = renderer->backend;
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
render_pop_clip(R_Context *renderer)
{
	stack_pop(renderer->backend->clip_stack);
}

internal R_Texture
render_create_texture(R_Context *renderer, Str8 path, R_ColorSpace color_space)
{
	R_Texture result = { 0 };

	Arena_Temporary scratch = arena_get_scratch(0, 0);

	Str8 contents = { 0 };
	if (os_file_read(scratch.arena, path, &contents))
	{
		int width         = 0;
		int height        = 0;
		int channel_count = 0;
		stbi_uc *pixels = stbi_load_from_memory(
			(stbi_uc const *) contents.data, (int) contents.size,
			&width, &height,
			&channel_count, 4
		);

		if (pixels)
		{
			// TODO(simon): Add support for greyscale and greyscale + alpha.
			GLenum byte_layout = 0;
			switch (channel_count)
			{
				case 1: assert(true); break;
				case 2: assert(true); break;
				case 3: byte_layout = GL_RGB;  break;
				case 4: byte_layout = GL_RGBA; break;
				invalid_case;
			}

			GLuint texture = 0;
			glCreateTextures(GL_TEXTURE_2D, 1, &texture);

			GLenum internalformat = 0;
			switch (color_space)
			{
				case R_ColorSpace_sRGB:   internalformat = GL_SRGB8_ALPHA8; break;
				case R_ColorSpace_Linear: internalformat = GL_RGBA8;        break;
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
				byte_layout, GL_UNSIGNED_BYTE,
				(const void *) pixels
			);

			result.u64[0] = (U64) texture;
			result.u64[1] = (U64) width;
			result.u64[2] = (U64) height;

			STBI_FREE(pixels);
		}
		else
		{
			// TODO(simon): Could not load image data.
		}
	}
	else
	{
		// TODO(simon): Could not read file.
	}

	arena_release_scratch(scratch);
	return(result);
}

internal R_Texture
render_create_texture_from_bitmap(R_Context *renderer, Void *data, U32 width, U32 height, R_ColorSpace color_space)
{
	R_Texture result = { 0 };

	GLuint texture = 0;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	GLenum internalformat = 0;
	switch (color_space)
	{
		case R_ColorSpace_sRGB:   internalformat = GL_SRGB8_ALPHA8; break;
		case R_ColorSpace_Linear: internalformat = GL_RGBA8;        break;
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
		(const void *) data
	);

	result.u64[0] = (U64) texture;
	result.u64[1] = width;
	result.u64[2] = height;

	return(result);
}

internal Void
render_destroy_texture(R_Context *renderer, R_Texture texture)
{
	if (texture.u64[0])
	{
		GLuint opengl_texture = (GLuint) texture.u64[0];
		glDeleteTextures(1, &opengl_texture);
	}
}

internal Void
render_update_texture(R_Context *renderer, R_Texture texture, Void *memory, U32 width, U32 height, U32 offset)
{
	if (texture.u64[0])
	{
		GLuint opengl_texture = (GLuint) texture.u64[0];
		U32 x = offset % texture.u64[1];
		U32 y = (U32)(offset / texture.u64[1]);
		glTextureSubImage2D(
			opengl_texture,
			0,
			(GLint) x, (GLint) y,
			(GLsizei) width, (GLsizei) height,
			GL_RGBA, GL_UNSIGNED_BYTE,
			(const void *) memory
		);
	}
}
