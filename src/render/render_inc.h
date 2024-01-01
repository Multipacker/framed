#ifndef RENDER_INC_H
#define RENDER_INC_H

#include "render/render_icon_codepoints.h"
#include "render/render_core.h"
#include "render/render_font.h"

#if OS_LINUX
#    if defined(RENDERER_OPENGL)
#        include "render/opengl/opengl_render.h"
#    elif defined(RENDERER_VULKAN)
#        include "render/vulkan/vulkan_render.h"
#    else
#        error No valid renderer backend selected
#    endif
#elif OS_WINDOWS
#    if defined(RENDERER_OPENGL)
#        include "render/opengl/opengl_render.h"
#    elif defined(RENDERER_VULKAN)
#        include "render/vulkan/vulkan_render.h"
#    elif defined(RENDERER_D3D11)
#        include "render/d3d11/d3d11_render.h"
#    else
#        error No valid renderer backend selected
#    endif
#endif

#endif
