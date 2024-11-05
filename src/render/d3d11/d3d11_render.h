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
    Render_Texture texture;
    D3D11_ClipRect *clip_rect;
};

typedef struct D3D11_Batch D3D11_Batch;
struct D3D11_Batch
{
    D3D11_Batch *next;
    D3D11_Batch *prev;
    Render_RectInstance *instances;
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

internal Void render_backend_init(Void);

internal Void render_backend_begin(Void);
internal Void render_backend_end(Void);
internal Render_Texture render_create_texture(Str8 path);
internal Render_Texture render_create_texture_from_bitmap(Void *memory, U32 width, U32 height, Render_ColorSpace color_space);
internal Void render_destroy_texture(Render_Texture texture);
internal Void render_update_texture(Render_Texture texture, Void *memory, U32 width, U32 height, U32 offset);

internal Render_RectInstance *render_rect_(Vec2F32 min, Vec2F32 max, Render_RectParams *params);
internal Void render_push_clip(Vec2F32 min, Vec2F32 max, B32 clip_to_parent);
internal Void render_pop_clip(Void);

#endif // RENDED3D11_H
