#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

internal Renderer
render_init(Gfx_Context *gfx)
{
    HRESULT hr;
    ID3D11Device* device;
    ID3D11DeviceContext* context;

    // NOTE(hampus): Create D3D11 device & context
    {
        UINT flags = 0;
#if !defined(BUILD_MODE_RELEASE)
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, levels, ARRAYSIZE(levels),
                               D3D11_SDK_VERSION, &device, 0, &context);
        assert(hr);
    }
}

internal Void
render_begin(Renderer *renderer)
{

}

internal Void
render_end(Renderer *renderer)
{
}

internal R_RectInstance *
render_rect_(Renderer *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params)
{

}

internal Void
render_push_clip(Renderer *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{

}

internal Void
render_pop_clip(Renderer *renderer)
{

}

internal R_RenderStats
render_get_stats(Renderer *renderer)
{

}
