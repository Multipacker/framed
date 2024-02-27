#ifndef OPENGL_H
#define OPENGL_H

// Definitions taken from:
//   * https://github.com/KhronosGroup/OpenGL-Registry
//   * https://registry.khronos.org/OpenGL-Refpages/gl4/

#define GL_BLEND                0x0BE2
#define GL_CLAMP_TO_EDGE        0x812F
#define GL_COLOR_BUFFER_BIT     0x00004000
#define GL_COMPILE_STATUS       0x8B81
#define GL_DYNAMIC_DRAW         0x88E8
#define GL_FALSE                0
#define GL_FLOAT                0x1406
#define GL_FRAGMENT_SHADER      0x8B30
#define GL_INFO_LOG_LENGTH      0x8B84
#define GL_LINEAR               0x2601
#define GL_LINK_STATUS          0x8B82
#define GL_ONE_MINUS_SRC1_COLOR 0x88FA
#define GL_ONE_MINUS_SRC_ALPHA  0x0303
#define GL_RGBA                 0x1908
#define GL_RGBA8                0x8058
#define GL_SCISSOR_TEST         0x0C11
#define GL_SRC1_COLOR           0x88F9
#define GL_SRC_ALPHA            0x8589
#define GL_SRGB8_ALPHA8         0x8C43
#define GL_TEXTURE_2D           0x0DE1
#define GL_TEXTURE_MAG_FILTER   0x2800
#define GL_TEXTURE_MIN_FILTER   0x2801
#define GL_TEXTURE_WRAP_S       0x2802
#define GL_TEXTURE_WRAP_T       0x2803
#define GL_TRIANGLE_STRIP       0x0005
#define GL_UNSIGNED_BYTE        0x1401
#define GL_VERTEX_SHADER        0x8B31

typedef char         GLchar;
typedef float        GLfloat;
typedef int          GLint;
typedef intptr_t     GLintptr;
typedef int          GLsizei;
typedef unsigned int GLbitfield;
typedef unsigned int GLboolean;
typedef unsigned int GLenum;
typedef unsigned int GLuint;

#if OS_WINDOWS && ARCH_X64
typedef signed long long int GLsizeiptr;
#else
typedef signed long int GLsizeiptr;
#endif

#if !OS_WINDOWS
#    define APIENTRYP *
#endif

typedef Void   (APIENTRYP PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void   (APIENTRYP PFNGLBINDTEXTUREUNITPROC)(GLuint unit, GLuint texture);
typedef Void   (APIENTRYP PFNGLBINDVERTEXARRAYPROC)(GLuint array);
typedef Void   (APIENTRYP PFNGLBLENDFUNCSEPARATEPROC)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef Void   (APIENTRYP PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
typedef Void   (APIENTRYP PFNGLCLEARPROC)(GLbitfield mask);
typedef Void   (APIENTRYP PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef Void   (APIENTRYP PFNGLCREATEBUFFERSPROC)(GLsizei n, GLuint *buffers);
typedef GLuint (APIENTRYP PFNGLCREATEPROGRAMPROC)(Void);
typedef GLuint (APIENTRYP PFNGLCREATESHADERPROC)(GLenum shaderType);
typedef Void   (APIENTRYP PFNGLCREATETEXTURESPROC)(GLenum target, GLsizei n, GLuint *textures);
typedef Void   (APIENTRYP PFNGLCREATEVERTEXARRAYSPROC)(GLsizei n, GLuint *arrays);
typedef Void   (APIENTRYP PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef Void   (APIENTRYP PFNGLDELETESHADERPROC)(GLuint shader);
typedef Void   (APIENTRYP PFNGLDELETETEXTURESPROC)(GLsizei n, const GLuint *textures);
typedef Void   (APIENTRYP PFNGLDETACHSHADERPROC)(GLuint program, GLuint shader);
typedef Void   (APIENTRYP PFNGLDISABLEPROC)(GLenum cap);
typedef Void   (APIENTRYP PFNGLDRAWARRAYSINSTANCEDPROC)(GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
typedef Void   (APIENTRYP PFNGLENABLEPROC)(GLenum cap);
typedef Void   (APIENTRYP PFNGLENABLEVERTEXARRAYATTRIBPROC)(GLuint vaobj, GLuint index);
typedef Void   (APIENTRYP PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void   (APIENTRYP PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint *params);
typedef Void   (APIENTRYP PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
typedef Void   (APIENTRYP PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint *params);
typedef GLint  (APIENTRYP PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar *name);
typedef Void   (APIENTRYP PFNGLLINKPROGRAMPROC)(GLuint program);
typedef Void   (APIENTRYP PFNGLNAMEDBUFFERDATAPROC)(GLuint buffer, GLsizeiptr size, const Void *data, GLenum usage);
typedef Void   (APIENTRYP PFNGLNAMEDBUFFERSUBDATAPROC)(GLuint buffer, GLintptr offset, GLsizeiptr size, const Void *data);
typedef Void   (APIENTRYP PFNGLPROGRAMUNIFORM1IPROC)(GLuint program, GLint location, GLint v0);
typedef Void   (APIENTRYP PFNGLPROGRAMUNIFORMMATRIX4FVPROC)(GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
typedef Void   (APIENTRYP PFNGLSCISSORPROC)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef Void   (APIENTRYP PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar **string, const GLint *length);
typedef Void   (APIENTRYP PFNGLTEXTUREPARAMETERIPROC)(GLuint texture, GLenum pname, GLint param);
typedef Void   (APIENTRYP PFNGLTEXTURESTORAGE2DPROC)(GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
typedef Void   (APIENTRYP PFNGLTEXTURESUBIMAGE2DPROC)(GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const Void *pixels);
typedef Void   (APIENTRYP PFNGLUSEPROGRAMPROC)(GLuint program);
typedef Void   (APIENTRYP PFNGLVERTEXARRAYATTRIBBINDINGPROC)(GLuint vaobj, GLuint attribindex, GLuint bindingindex);
typedef Void   (APIENTRYP PFNGLVERTEXARRAYATTRIBFORMATPROC)(GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
typedef Void   (APIENTRYP PFNGLVERTEXARRAYBINDINGDIVISORPROC)(GLuint vaobj, GLuint bindingindex, GLuint divisor);
typedef Void   (APIENTRYP PFNGLVERTEXARRAYVERTEXBUFFERPROC)(GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
typedef Void   (APIENTRYP PFNGLVIEWPORTPROC)(GLint x, GLint y, GLsizei width, GLsizei height);

#define GL_FUNCTIONS(X)                                                \
    X(PFNGLATTACHSHADERPROC,              glAttachShader)              \
    X(PFNGLBINDTEXTUREUNITPROC,           glBindTextureUnit)           \
    X(PFNGLBINDVERTEXARRAYPROC,           glBindVertexArray)           \
    X(PFNGLBLENDFUNCSEPARATEPROC,         glBlendFuncSeparate)         \
    X(PFNGLCLEARCOLORPROC,                glClearColor)                \
    X(PFNGLCLEARPROC,                     glClear)                     \
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

#endif // OPENGL_H
