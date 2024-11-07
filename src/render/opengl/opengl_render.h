#ifndef OPENGL_RENDER_H
#define OPENGL_RENDER_H

#include "opengl.h"

#define OPENGL_BATCH_SIZE 1024

#define OPENGL_TEXTURE_UPDATE_QUEUE_SIZE (1 << 6)
#define OPENGL_TEXTURE_UPDATE_QUEUE_MASK (OPENGL_TEXTURE_UPDATE_QUEUE_SIZE - 1)

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

internal Void r_backend_init(Void);

#endif // OPENGL_RENDER_H
