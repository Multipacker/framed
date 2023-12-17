#pragma comment(lib, "opengl32.lib")

global PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = 0;
global PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
global PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 0;

 internal Void APIENTRY
win32_debug_output(GLenum source,
                GLenum type,
                 U32 id,
                GLenum severity,
                GLsizei length,
                  const char *message,
                 const Void *userParam)
{
	if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

	assert(false);

	printf("---------------\n");
	printf("Debug message (%d): %s", id, message);

	switch (source)
	{
		case GL_DEBUG_SOURCE_API:             printf("Source: API"); break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   printf("Source: Window System"); break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER: printf("Source: Shader Compiler"); break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:     printf("Source: Third Party"); break;
		case GL_DEBUG_SOURCE_APPLICATION:     printf("Source: Application"); break;
		case GL_DEBUG_SOURCE_OTHER:           printf("Source: Other"); break;
	}

	printf("\n");

	switch (type)
	{
		case GL_DEBUG_TYPE_ERROR:               printf("Type: Error"); break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: printf("Type: Deprecated Behaviour"); break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  printf("Type: Undefined Behaviour"); break;
		case GL_DEBUG_TYPE_PORTABILITY:         printf("Type: Portability"); break;
		case GL_DEBUG_TYPE_PERFORMANCE:         printf("Type: Performance"); break;
		case GL_DEBUG_TYPE_MARKER:              printf("Type: Marker"); break;
		case GL_DEBUG_TYPE_PUSH_GROUP:          printf("Type: Push Group"); break;
		case GL_DEBUG_TYPE_POP_GROUP:           printf("Type: Pop Group"); break;
		case GL_DEBUG_TYPE_OTHER:               printf("Type: Other"); break;
	}

	printf("\n");

	switch (severity)
	{
		case GL_DEBUG_SEVERITY_HIGH:         printf("Severity: high"); break;
		case GL_DEBUG_SEVERITY_MEDIUM:       printf("Severity: medium"); break;
		case GL_DEBUG_SEVERITY_LOW:          printf("Severity: low"); break;
		case GL_DEBUG_SEVERITY_NOTIFICATION: printf("Severity: notification"); break;
	}
	printf("\n");
}

internal Void
win32_get_wgl_functions(Void)
{
	 Arena_Temporary scratch = arena_get_scratch(0, 0);

	HWND dummy = CreateWindowEx(0,
                                (LPCWSTR)cstr16_from_str8(scratch.arena, str8_lit("STATIC")).data,
                                (LPCWSTR)cstr16_from_str8(scratch.arena, str8_lit("DummyWindow")).data,
                                WS_OVERLAPPED,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                NULL, NULL, NULL, NULL);

	assert(dummy && "Failed to create dummy window");

	HDC dc = GetDC(dummy);
	assert(dc && "Failed to get device context for dummy window");

	PIXELFORMATDESCRIPTOR desc =
	{
		.nSize = sizeof(desc),
		.nVersion = 1,
		.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		.iPixelType = PFD_TYPE_RGBA,
		.cColorBits = 24,
	};

	S32 format = ChoosePixelFormat(dc, &desc);
	if (!format)
	{
		assert(!"Cannot choose OpenGL pixel format for dummy window!");
	}

	S32 ok = DescribePixelFormat(dc, format, sizeof(desc), &desc);
	assert(ok && "Failed to describe OpenGL pixel format");

	if (!SetPixelFormat(dc, format, &desc))
	{
		assert(!"Cannot set OpenGL pixel format for dummy window!");
	}

	HGLRC rc = wglCreateContext(dc);
	assert(rc && "Failed to create OpenGL context for dummy window");

	ok = wglMakeCurrent(dc, rc);
	assert(ok && "Failed to make current OpenGL context for dummy window");

	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
    (Void*)wglGetProcAddress("wglGetExtensionsStringARB");
	if (!wglGetExtensionsStringARB)
	{
		assert(!"OpenGL does not support WGL_ARB_extensions_string extension!");
	}

	Str8 extensions = str8_cstr((CStr)wglGetExtensionsStringARB(dc));
	assert(extensions.data && "Failed to get OpenGL WGL extension string");

    Str8List extension_list = str8_split_by_codepoints(scratch.arena, extensions, str8_lit(" "));

    for (Str8Node *node = extension_list.first;
         node != 0;
         node = node->next)
    {
        Str8 string = node->string;

		if (str8_equal(string, str8_lit("WGL_ARB_pixel_format")))
		{
			wglChoosePixelFormatARB = (Void *)wglGetProcAddress("wglChoosePixelFormatARB");
		}
		else if (str8_equal(string, str8_lit("WGL_ARB_create_context")))
		{
			wglCreateContextAttribsARB = (Void *)wglGetProcAddress("wglCreateContextAttribsARB");
		}
		else if (str8_equal(string, str8_lit("WGL_EXT_swap_control")))
		{
			wglSwapIntervalEXT = (Void *)wglGetProcAddress("wglSwapIntervalEXT");
		}
    }

	if (!wglChoosePixelFormatARB || !wglCreateContextAttribsARB || !wglSwapIntervalEXT)
	{
		assert(!"OpenGL does not support required WGL extensions for modern context!");
	}

	arena_release_scratch(scratch);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(rc);
	ReleaseDC(dummy, dc);
	DestroyWindow(dummy);
}

 internal Void
win32_init_opengl(Gfx_Context *gfx)
{
	win32_get_wgl_functions();
	{
		S32 attrib[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
			WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
			WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_COLOR_BITS_ARB, 24,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
			0,
		};

		S32 format;
		UINT formats;
		if (!wglChoosePixelFormatARB(gfx->hdc, attrib, 0, 1, &format, &formats) || formats == 0)
		{
			assert(!"OpenGL does not support required pixel format!");
		}

		PIXELFORMATDESCRIPTOR desc ={ .nSize = sizeof(desc) };
		S32 ok = DescribePixelFormat(gfx->hdc, format, sizeof(desc), &desc);
		assert(ok && "Failed to describe OpenGL pixel format");

		if (!SetPixelFormat(gfx->hdc, format, &desc))
		{
			assert(!"Cannot set OpenGL selected pixel format!");
		}
	}

	{
		 S32 attrib[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 5,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#if !BUILD_MODE_RELEASE
			WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
			0,
		};

		HGLRC rc = wglCreateContextAttribsARB(gfx->hdc, NULL, attrib);
		if (!rc)
		{
			assert(!"Cannot create modern OpenGL context! OpenGL version 4.5 not supported?");
		}

		BOOL ok = wglMakeCurrent(gfx->hdc, rc);
		assert(ok && "Failed to make current OpenGL context");

#define X(type, name) name = (type)wglGetProcAddress(#name); assert(name);
		GL_FUNCTIONS(X)
#undef X

#if !BUILD_MODE_RELEASE
        glDebugMessageCallback(&win32_debug_output, NULL);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
	}
}

 internal Void
gfx_swap_buffers(Gfx_Context *gfx)
{
	SwapBuffers(gfx->hdc);
}