#ifndef GFX_LINUX_OPENGL_H
#define GFX_LINUX_OPENGL_H

#include <GL/glcorearb.h>

#define GL_FUNCTIONS(X)                                                \
    X(PFNGLCLEARPROC,                     glClear)                     \
    X(PFNGLCLEARCOLORPROC,                glClearColor)                \
    X(PFNGLATTACHSHADERPROC,              glAttachShader)              \
    X(PFNGLBINDTEXTUREUNITPROC,           glBindTextureUnit)           \
    X(PFNGLBINDVERTEXARRAYPROC,           glBindVertexArray)           \
    X(PFNGLBLENDFUNCSEPARATEPROC,         glBlendFuncSeparate)         \
    X(PFNGLCOMPILESHADERPROC,             glCompileShader)             \
    X(PFNGLCREATEBUFFERSPROC,             glCreateBuffers)             \
    X(PFNGLCREATEPROGRAMPROC,             glCreateProgram)             \
    X(PFNGLCREATESHADERPROC,              glCreateShader)              \
    X(PFNGLCREATETEXTURESPROC,            glCreateTextures)            \
    X(PFNGLCREATEVERTEXARRAYSPROC,        glCreateVertexArrays)        \
    X(PFNGLDELETEPROGRAMPROC,             glDeleteProgram)             \
    X(PFNGLDELETESHADERPROC,              glDeleteShader)              \
    X(PFNGLDELETETEXTURESPROC,            glDeleteTextures)            \
    X(PFNGLDETACHSHADERPROC,              glDetachShader)              \
    X(PFNGLDISABLEPROC,                   glDisable)                   \
    X(PFNGLDRAWARRAYSINSTANCEDPROC,       glDrawArraysInstanced)       \
    X(PFNGLENABLEPROC,                    glEnable)                    \
    X(PFNGLENABLEVERTEXARRAYATTRIBPROC,   glEnableVertexArrayAttrib)   \
    X(PFNGLGETPROGRAMINFOLOGPROC,         glGetProgramInfoLog)         \
    X(PFNGLGETPROGRAMIVPROC,              glGetProgramiv)              \
    X(PFNGLGETSHADERINFOLOGPROC,          glGetShaderInfoLog)          \
    X(PFNGLGETSHADERIVPROC,               glGetShaderiv)               \
    X(PFNGLGETUNIFORMLOCATIONPROC,        glGetUniformLocation)        \
    X(PFNGLLINKPROGRAMPROC,               glLinkProgram)               \
    X(PFNGLNAMEDBUFFERDATAPROC,           glNamedBufferData)           \
    X(PFNGLNAMEDBUFFERSUBDATAPROC,        glNamedBufferSubData)        \
    X(PFNGLPROGRAMUNIFORM1IPROC,          glProgramUniform1i)          \
    X(PFNGLPROGRAMUNIFORMMATRIX4FVPROC,   glProgramUniformMatrix4fv)   \
    X(PFNGLSCISSORPROC,                   glScissor)                   \
    X(PFNGLSHADERSOURCEPROC,              glShaderSource)              \
    X(PFNGLTEXTUREPARAMETERIPROC,         glTextureParameteri)         \
    X(PFNGLTEXTURESTORAGE2DPROC,          glTextureStorage2D)          \
    X(PFNGLTEXTURESUBIMAGE2DPROC,         glTextureSubImage2D)         \
    X(PFNGLUSEPROGRAMPROC,                glUseProgram)                \
    X(PFNGLVERTEXARRAYATTRIBBINDINGPROC,  glVertexArrayAttribBinding)  \
    X(PFNGLVERTEXARRAYATTRIBFORMATPROC,   glVertexArrayAttribFormat)   \
    X(PFNGLVERTEXARRAYBINDINGDIVISORPROC, glVertexArrayBindingDivisor) \
    X(PFNGLVERTEXARRAYVERTEXBUFFERPROC,   glVertexArrayVertexBuffer)   \
    X(PFNGLVIEWPORTPROC,                  glViewport)                  \

#define X(type, name) global type name;
GL_FUNCTIONS(X)
#undef X

internal Void linux_init_opengl(Void);

#endif // GFX_LINUX_OPENGL_H
