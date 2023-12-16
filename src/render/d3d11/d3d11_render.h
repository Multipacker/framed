#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

typedef struct Renderer Renderer;
struct Renderer
{
    Arena *perm_arena;

    Gfx_Context *gfx;

    ID3D11Device        *device;
    ID3D11DeviceContext *context;
    IDXGISwapChain1     *swap_chain;

    ID3D11Buffer        *vertex_buffer;
    ID3D11Buffer        *uniform_buffer;

    ID3D11InputLayout  *input_layout;
    ID3D11VertexShader *vertex_shader;
    ID3D11PixelShader  *pixel_shader;

    ID3D11BlendState        *blend_state;
    ID3D11SamplerState      *sampler;
    ID3D11RasterizerState   *rasterizer_state;
    ID3D11DepthStencilState *depth_state;

    ID3D11RenderTargetView  *render_target_view;
    ID3D11DepthStencilView  *depth_stencil_view;

    ID3D11ShaderResourceView *texture_view;

    DWORD current_width;
    DWORD current_height;
};

#endif //RENDER_D3D11_H
