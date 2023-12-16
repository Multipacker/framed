#if OS_LINUX
#    if defined(RENDERER_OPENGL)
#        include "render/opengl/opengl_render.c"
#    elif defined(RENDERER_VULKAN)
#        include "render/opengl/vulkan_render.c"
#    else
#        error No valid renderer backend selected
#    endif
#elif OS_WINDOWS
#    if defined(RENDERER_OPENGL)
#        include "render/opengl/opengl_render.c"
#    elif defined(RENDERER_VULKAN)
#        include "render/opengl/vulkan_render.c"
#    elif defined(RENDERER_D3D11)
#        include "render/d3d11/d3d11_render.c"
#    else
#        error No valid renderer backend selected
#    endif
#endif
