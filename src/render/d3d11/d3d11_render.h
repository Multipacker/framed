#ifndef RENDER_D3D11_H
#define RENDER_D3D11_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define D3D11_BATCH_SIZE 4096

typedef struct R_D3D11_BatchParams R_D3D11_BatchParams;
struct R_D3D11_BatchParams
{
    ID3D11ShaderResourceView *texture;
    RectF32 clip_rect;
};

typedef struct R_D3D11_Batch R_D3D11_Batch;
struct R_D3D11_Batch
{
    R_RectInstance *instances;
    U64 instance_count;
    R_D3D11_BatchParams params;
};

typedef struct R_D3D11_BatchNode R_D3D11_BatchNode;
struct R_D3D11_BatchNode
{
    R_D3D11_BatchNode *next;
    R_D3D11_BatchNode *prev;
    R_D3D11_Batch *batch;
};

typedef struct R_D3D11_BatchList R_D3D11_BatchList;
struct R_D3D11_BatchList
{
    R_D3D11_BatchNode *first;
    R_D3D11_BatchNode *last;
    U64 count;
};

typedef struct R_D3D11_ClipRectNode R_D3D11_ClipRectNode;
struct R_D3D11_ClipRectNode
{
    R_D3D11_ClipRectNode *next;
    R_D3D11_ClipRectNode *prev;
    RectF32 rect;
};

typedef struct R_D3D11_ClipRectStack R_D3D11_ClipRectStack;
struct R_D3D11_ClipRectStack
{
    R_D3D11_ClipRectNode *first;
    R_D3D11_ClipRectNode *last;
};

typedef struct R_Context R_Context;
struct R_Context
{
    Arena *perm_arena;
    Arena *frame_arena;

    Gfx_Context *gfx;

    R_D3D11_BatchList *batch_list;

    R_RenderStats render_stats[2];

    R_D3D11_ClipRectStack clip_rect_stack;

    ID3D11ShaderResourceView *white_texture;

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