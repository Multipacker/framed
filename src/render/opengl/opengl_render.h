#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#define OPENGL_BATCH_SIZE 1024

typedef struct OpenGL_ClipNode OpenGL_ClipNode;
struct OpenGL_ClipNode
{
	OpenGL_ClipNode *next;
	RectF32 rect;
};

typedef struct OpenGL_Batch OpenGL_Batch;
struct OpenGL_Batch
{
	OpenGL_Batch *next;
	OpenGL_Batch *prev;

	OpenGL_ClipNode *clip_node;
	R_Texture texture;

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

struct R_Context
{
	Gfx_Context *gfx;

	R_RenderStats stats;

	Arena *arena;
	Arena *frame_arena;

	OpenGL_BatchList batches;
	Vec2U32 client_area;

	GLuint program;
	GLuint vbo;
	GLuint vao;
	GLint uniform_projection_location;
	GLint uniform_sampler_location;

	OpenGL_ClipNode *clip_stack;
};

#endif // OPENGL_RENDER_H
