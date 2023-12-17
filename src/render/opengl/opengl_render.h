#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#define OPENGL_BATCH_SIZE 10

typedef struct OpenGL_Batch OpenGL_Batch;
struct OpenGL_Batch
{
	OpenGL_Batch *next;
	OpenGL_Batch *prev;
	U32 size;
	R_RectInstance rects[OPENGL_BATCH_SIZE];
};

typedef struct OpenGL_BatchList OpenGL_BatchList;
struct OpenGL_BatchList
{
	OpenGL_Batch *first;
	OpenGL_Batch *last;
	U64 rect_count;
	U64 batch_count;
};

struct Renderer
{
	Gfx_Context *gfx;

	Arena *frame_arena;

	OpenGL_BatchList batches;

	GLuint program;
	GLuint vbo;
	GLuint vao;
	GLint uniform_projection_location;
};

#endif // OPENGL_RENDER_H
