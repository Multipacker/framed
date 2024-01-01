#ifndef RENDED3D11_H
#define RENDED3D11_H

#define COBJMACROS
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>

#define D3D11_BATCH_SIZE 4096

#define D3D11_TEXTURE_UPDATE_QUEUE_SIZE (1 << 6)
#define D3D11_TEXTURE_UPDATE_QUEUE_MASK (D3D11_TEXTURE_UPDATE_QUEUE_SIZE - 1)

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

typedef struct D3D11_TextureUpdate D3D11_TextureUpdate;
struct D3D11_TextureUpdate
{
	volatile B32 is_valid;

	ID3D11Resource *resource;

	U32 x;
	U32 y;
	U32 width;
	U32 height;

	Void *data;
};

typedef struct R_BackendContext R_BackendContext;
struct R_BackendContext
{
	D3D11_BatchList batch_list;

	D3D11_ClipRectStack clip_rect_stack;

	R_Texture white_texture;

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

	D3D11_TextureUpdate *texture_update_queue;
	U32 volatile texture_update_write_index;
	U32 volatile texture_update_read_index;
};

internal R_BackendContext *render_backend_init(R_Context *renderer);

#endif //RENDED3D11_H
