#ifndef RENDED3D11_H
#define RENDED3D11_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define D3D11_BATCH_SIZE 4096

typedef struct D3D11_ClipRect D3D11_ClipRect;
struct D3D11_ClipRect
{
	D3D11_ClipRect *next;
	D3D11_ClipRect *prev;
	RectF32 rect;
};

typedef struct D3D11_ClipRectStack D3D11_ClipRectStack;
struct D3D11_ClipRectStack
{
	D3D11_ClipRect *first;
	D3D11_ClipRect *last;
};

typedef struct D3D11_BatchParams D3D11_BatchParams;
struct D3D11_BatchParams
{
	R_Texture texture;
	D3D11_ClipRect *clip_rect;
};

typedef struct D3D11_Batch D3D11_Batch;
struct D3D11_Batch
{
	D3D11_Batch *next;
	D3D11_Batch *prev;
	R_RectInstance *instances;
	U64 instance_count;
	D3D11_BatchParams params;
};

typedef struct D3D11_BatchList D3D11_BatchList;
struct D3D11_BatchList
{
	D3D11_Batch *first;
	D3D11_Batch *last;
	U64 batch_count;
};

typedef struct R_Context R_Context;
struct R_Context
{
	Arena *arena;
	Arena *frame_arena;

	Gfx_Context *gfx;

	D3D11_BatchList batch_list;

	R_RenderStats render_stats[2];

	D3D11_ClipRectStack clip_rect_stack;

	R_Texture white_texture;

	R_FontAtlas *font_atlas;

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

	DWORD current_width;
	DWORD current_height;
};

#endif //RENDED3D11_H