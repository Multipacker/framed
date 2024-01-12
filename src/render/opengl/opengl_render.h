#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#define OPENGL_BATCH_SIZE 1024

#define OPENGL_TEXTURE_UPDATE_QUEUE_SIZE (1 << 6)
#define OPENGL_TEXTURE_UPDATE_QUEUE_MASK (OPENGL_TEXTURE_UPDATE_QUEUE_SIZE - 1)

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
	Render_Texture texture;

	U32 size;
	Render_RectInstance rects[OPENGL_BATCH_SIZE];
};

typedef struct OpenGL_BatchList OpenGL_BatchList;
struct OpenGL_BatchList
{
	OpenGL_Batch *first;
	OpenGL_Batch *last;
	U64 rect_count;
	U64 batch_count;
};

typedef struct OpenGL_TextureUpdate OpenGL_TextureUpdate;
struct OpenGL_TextureUpdate
{
	volatile B32 is_valid;

	GLuint texture;

	GLint x;
	GLint y;
	GLsizei width;
	GLsizei height;

	Void *data;
};

struct Render_BackendContext
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

internal Render_BackendContext *render_backend_init(Render_Context *renderer);

#endif // OPENGL_RENDER_H
