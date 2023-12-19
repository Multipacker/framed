#ifndef WIN32_GFX_OPENGL_H
#define WIN32_GFX_OPENGL_H

#include <GL/gl.h>

#include "glcorearb.h"
#include "wglext.h"

#define GL_FUNCTIONS(X) \
X(PFNGLCREATEBUFFERSPROC,               glCreateBuffers             ) \
X(PFNGLNAMEDBUFFERSTORAGEPROC,          glNamedBufferStorage        ) \
X(PFNGLBINDVERTEXARRAYPROC,             glBindVertexArray           ) \
X(PFNGLCREATEVERTEXARRAYSPROC,          glCreateVertexArrays        ) \
X(PFNGLVERTEXARRAYATTRIBBINDINGPROC,    glVertexArrayAttribBinding  ) \
X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,     glVertexArrayVertexBuffer   ) \
X(PFNGLVERTEXARRAYATTRIBFORMATPROC,     glVertexArrayAttribFormat   ) \
X(PFNGLENABLEVERTEXARRAYATTRIBPROC,     glEnableVertexArrayAttrib   ) \
X(PFNGLCREATESHADERPROGRAMVPROC,        glCreateShaderProgramv      ) \
X(PFNGLGETPROGRAMIVPROC,                glGetProgramiv              ) \
X(PFNGLGETPROGRAMINFOLOGPROC,           glGetProgramInfoLog         ) \
X(PFNGLGENPROGRAMPIPELINESPROC,         glGenProgramPipelines       ) \
X(PFNGLUSEPROGRAMSTAGESPROC,            glUseProgramStages          ) \
X(PFNGLBINDPROGRAMPIPELINEPROC,         glBindProgramPipeline       ) \
X(PFNGLPROGRAMUNIFORMMATRIX2FVPROC,     glProgramUniformMatrix2fv   ) \
X(PFNGLPROGRAMUNIFORMMATRIX4FVPROC,     glProgramUniformMatrix4fv   ) \
X(PFNGLBINDTEXTUREUNITPROC,             glBindTextureUnit           ) \
X(PFNGLCREATETEXTURESPROC,              glCreateTextures            ) \
X(PFNGLTEXTUREPARAMETERIPROC,           glTextureParameteri         ) \
X(PFNGLTEXTURESTORAGE2DPROC,            glTextureStorage2D          ) \
X(PFNGLTEXTURESUBIMAGE2DPROC,           glTextureSubImage2D         ) \
X(PFNGLDEBUGMESSAGECALLBACKPROC,        glDebugMessageCallback      ) \
X(PFNGLNAMEDBUFFERSUBDATAPROC,     	 glNamedBufferSubData        ) \
X(PFNGLDRAWARRAYSINSTANCEDPROC,         glDrawArraysInstanced       ) \
X(PFNGLSCISSORINDEXEDPROC,              glScissorIndexed            ) \
X(PFNGLVERTEXARRAYBINDINGDIVISORPROC,   glVertexArrayBindingDivisor ) \
X(PFNGLCREATESHADERPROC,                glCreateShader              ) \
X(PFNGLSHADERSOURCEPROC,                glShaderSource              ) \
X(PFNGLCOMPILESHADERPROC,               glCompileShader             ) \
X(PFNGLGETSHADERIVPROC,                 glGetShaderiv               ) \
X(PFNGLGETSHADERINFOLOGPROC,            glGetShaderInfoLog          ) \
X(PFNGLDELETESHADERPROC,                glDeleteShader              ) \
X(PFNGLCREATEPROGRAMPROC,               glCreateProgram             ) \
X(PFNGLATTACHSHADERPROC,                glAttachShader              ) \
X(PFNGLLINKPROGRAMPROC,                 glLinkProgram               ) \
X(PFNGLDETACHSHADERPROC,                glDetachShader              ) \
X(PFNGLDELETEPROGRAMPROC,               glDeleteProgram             ) \
X(PFNGLNAMEDBUFFERDATAPROC,             glNamedBufferData           ) \
X(PFNGLGETUNIFORMLOCATIONPROC,          glGetUniformLocation        ) \
X(PFNGLUSEPROGRAMPROC,                  glUseProgram                ) \
X(PFNGLPROGRAMUNIFORM1IPROC,            glProgramUniform1i          ) \

#define X(type, name) global type name;
GL_FUNCTIONS(X)
#undef X

internal Void win32_init_opengl(Gfx_Context *gfx);

#endif
