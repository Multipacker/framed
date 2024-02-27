internal Void
linux_init_opengl(Void)
{
#define X(type, name) name = (type) SDL_GL_GetProcAddress(#name); assert(name);
    GL_FUNCTIONS(X)
#undef X
}
