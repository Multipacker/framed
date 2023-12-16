#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

struct Renderer
{
	Gfx_Context *gfx;
	GLuint program;
	GLuint vao;
};

#endif // OPENGL_RENDER_H
