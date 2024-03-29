#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")

#define assert_hr(hr) assert(SUCCEEDED(hr))

#include <intrin.h>

internal D3D11_ClipRect *
d3d11_top_clip(Render_Context *renderer)
{
    return(renderer->backend->clip_rect_stack.first);
}

internal Render_RenderStats *
d3d11_get_current_stats(Render_Context *renderer)
{
    return(&renderer->render_stats[1]);
}

#pragma optimize( "", off )
internal Void
d3d11_load_shaders(Render_BackendContext *backend)
{
        D3D11_INPUT_ELEMENT_DESC desc[] =
        {
            { "MIN", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(Render_RectInstance, min), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "MAX", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(Render_RectInstance, max), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "MIN_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(Render_RectInstance, min_uv), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "MAX_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(Render_RectInstance, max_uv), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(Render_RectInstance, colors), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(Render_RectInstance, colors)+sizeof(Vec4F32), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(Render_RectInstance, colors)+sizeof(Vec4F32)*2, D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(Render_RectInstance, colors)+sizeof(Vec4F32)*3, D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "CORNER_RADIUS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(Render_RectInstance, radies), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "SOFTNESS", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(Render_RectInstance, softness), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "BORDER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(Render_RectInstance, border_thickness), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "OMIT_TEXTURE", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(Render_RectInstance, omit_texture), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "IS_SUBPIXEL_TEXT", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(Render_RectInstance, is_subpixel_text), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

            { "USE_NEAREST", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(Render_RectInstance, use_nearest), D3D11_INPUT_PER_INSTANCE_DATA, 1 },

        };

#include "render/d3d11/d3d11_vshader.h"
#include "render/d3d11/d3d11_pshader.h"

        ID3D11Device_CreateVertexShader(backend->device, d3d11_vshader, sizeof(d3d11_vshader), 0, &backend->vertex_shader);
        ID3D11Device_CreatePixelShader(backend->device, d3d11_pshader, sizeof(d3d11_pshader), 0, &backend->pixel_shader);
        ID3D11Device_CreateInputLayout(backend->device, desc, array_count(desc), d3d11_vshader, sizeof(d3d11_vshader), &backend->input_layout);

}

#pragma optimize( "", on )

internal Render_BackendContext *
render_backend_init(Render_Context *renderer)
{
    Gfx_Context *gfx = renderer->gfx;

    // NOTE(simon): The alignment is needed for atomic access within the struct.
    arena_align(renderer->permanent_arena, 8);
    renderer->backend = push_struct(renderer->permanent_arena, Render_BackendContext);
    Render_BackendContext *backend = renderer->backend;
    backend->texture_update_queue = push_array_zero(renderer->permanent_arena, D3D11_TextureUpdate, D3D11_TEXTURE_UPDATE_QUEUE_SIZE);

    HRESULT hr;

    IDXGIInfoQueue      *dxgi_info;
    IDXGIDevice         *dxgi_device;
    IDXGIAdapter        *dxgi_adapter;
    IDXGIFactory2       *factory;
    ID3D11InfoQueue     *info;
    // NOTE(hampus): Create D3D11 device & context
    {
        UINT flags = 0;
#if !BUILD_MODE_RELEASE
        flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0 };
        hr = D3D11CreateDevice(
            0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, levels, ARRAYSIZE(levels),
            D3D11_SDK_VERSION, &backend->device, 0, &backend->context);
        assert_hr(hr);
    }

    // NOTE(hampus): Enable useful debug break on errors
#if !BUILD_MODE_RELEASE
    {
        ID3D11Device_QueryInterface(backend->device, &IID_ID3D11InfoQueue, (Void**) &info);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(info);
    }

    {
        hr = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (Void**) &dxgi_info);
        IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        IDXGIInfoQueue_Release(dxgi_info);
    }
#endif

    // NOTE(hampus): Create swap chain
    {
        hr = ID3D11Device_QueryInterface(backend->device, &IID_IDXGIDevice, (Void**) &dxgi_device);
        assert_hr(hr);

        hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
        assert_hr(hr);

        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (Void**) &factory);
        assert_hr(hr);

        DXGI_SWAP_CHAIN_DESC1 desc =
        {
            //.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
            .BufferCount = 2,
            .Scaling = DXGI_SCALING_NONE,

            // NOTE(hampus): for Windows 8 compatibility use DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL
            // NOTE(hampus): for Windows 7 compatibility use DXGI_SWAP_EFFECT_DISCARD
            .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        };

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*) backend->device, gfx->hwnd, &desc, 0, 0, &backend->swap_chain);
        assert_hr(hr);

        IDXGIFactory_MakeWindowAssociation(factory, gfx->hwnd, DXGI_MWA_NO_ALT_ENTER);

        IDXGIFactory2_Release(factory);
        IDXGIAdapter_Release(dxgi_adapter);
        IDXGIDevice_Release(dxgi_device);
    }

    // NOTE(hampus): Create vertex buffer
    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = D3D11_BATCH_SIZE * sizeof(Render_RectInstance),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
        };

        ID3D11Device_CreateBuffer(backend->device, &desc, 0, &backend->vertex_buffer);
    }

    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = sizeof(Mat4F32),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        ID3D11Device_CreateBuffer(backend->device, &desc, 0, &backend->uniform_buffer);
    }

    d3d11_load_shaders(backend);

    // NOTE(hampus): White Texture
    {
        U32 white = 0xffffffff;

        UINT width = 1;
        UINT height = 1;

        D3D11_TEXTURE2D_DESC desc =
        {
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = { 1, 0 },
            .Usage = D3D11_USAGE_IMMUTABLE,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        D3D11_SUBRESOURCE_DATA data =
        {
            .pSysMem = &white,
            .SysMemPitch = width * sizeof(U32),
        };

        ID3D11Texture2D *texture;
        ID3D11ShaderResourceView *texture_view;
        ID3D11Device_CreateTexture2D(backend->device, &desc, &data, &texture);
        ID3D11Device_CreateShaderResourceView(backend->device, (ID3D11Resource*) texture, 0, &texture_view);
        ID3D11Texture2D_Release(texture);

        backend->white_texture.u64[0] = int_from_ptr(texture_view);
    }

    // NOTE(hampus): Sampler
    {
        D3D11_SAMPLER_DESC desc =
        {
            .Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR,
            .AddressU = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressV = D3D11_TEXTURE_ADDRESS_CLAMP,
            .AddressW = D3D11_TEXTURE_ADDRESS_CLAMP,
            .MaxAnisotropy = 1,
            .MinLOD = -F32_MAX,
            .MaxLOD = +F32_MAX,
        };

        ID3D11Device_CreateSamplerState(backend->device, &desc, &backend->sampler);
    }

    // NOTE(hampus): Blend state
    {
        // NOTE(hampus): Enable alpha blending
        D3D11_BLEND_DESC desc =
        {
            .RenderTarget[0] =
            {
                .BlendEnable = TRUE,

                // NOTE(hampus): For subpixel text rendering the rgb values are acting
                // as an alpha value for each component. Therefore we specify another
                // color source which will be these alpha values.

                // NOTE(hampus): dst.rgb = dst.rgb * (1 - src1.rgb) + src0.rgb * (src1.rgb)
            .SrcBlend = D3D11_BLEND_SRC1_COLOR,
            .DestBlend = D3D11_BLEND_INV_SRC1_COLOR,
            .BlendOp = D3D11_BLEND_OP_ADD,

            // NOTE(hampus): dst.a = dst.a * (1 - color.a) + src.a * (color.a)
            .SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA,
            .DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA,
            .BlendOpAlpha = D3D11_BLEND_OP_ADD,

            .RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
        },
        };
        ID3D11Device_CreateBlendState(backend->device, &desc, &backend->blend_state);
    }

    // NOTE(hampus): Rasterizer state
    {
        // NOTE(hampus): Disable culling
        D3D11_RASTERIZER_DESC desc =
        {
            .FillMode = D3D11_FILL_SOLID,
            .CullMode = D3D11_CULL_NONE,
            .ScissorEnable = TRUE,
        };
        ID3D11Device_CreateRasterizerState(backend->device, &desc, &backend->rasterizer_state);
    }

    // NOTE(hampus): Depth stencil state
    {
        // NOTE(hampus): Disable depth & stencil test
        D3D11_DEPTH_STENCIL_DESC desc =
        {
            .DepthEnable = FALSE,
            .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D11_COMPARISON_LESS,
            .StencilEnable = FALSE,
            .StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK,
            .StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK,
        };
        ID3D11Device_CreateDepthStencilState(backend->device, &desc, &backend->depth_state);
    }

    return(backend);
}

internal D3D11_Batch *
d3d11_push_batch(Render_Context *renderer)
{
    D3D11_Batch *result     = push_struct_zero(renderer->frame_arena, D3D11_Batch);
    result->instances = push_array(renderer->frame_arena, Render_RectInstance, D3D11_BATCH_SIZE);
    dll_push_back(renderer->backend->batch_list.first, renderer->backend->batch_list.last, result);
    result->params.clip_rect = d3d11_top_clip(renderer);
    renderer->backend->batch_list.batch_count++;
    Render_RenderStats *stats = d3d11_get_current_stats(renderer);
    stats->batch_count++;
    return(result);
}

internal Void
render_backend_begin(Render_Context *renderer)
{
    // NOTE(hampus): Push clip rect
    Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);
    Vec2F32 max_clip;
    max_clip.x = (F32) client_area.x;
    max_clip.y = (F32) client_area.y;
    render_push_clip(renderer, v2f32(0, 0), max_clip, false);

    // NOTE(hampus): First batch
    D3D11_Batch *first_batch = d3d11_push_batch(renderer);
    first_batch->params.texture = renderer->backend->white_texture;
}

internal Void
render_backend_end(Render_Context *renderer)
{
    Render_BackendContext *backend = renderer->backend;

    // NOTE(simon): Perform texture updates.
    while (backend->texture_update_write_index - backend->texture_update_read_index != 0)
    {
        D3D11_TextureUpdate *waiting_update = &backend->texture_update_queue[backend->texture_update_read_index & D3D11_TEXTURE_UPDATE_QUEUE_MASK];

        while (!waiting_update->is_valid)
        {
            // NOTE(simon): Busy wait for the entry to become valid.
        }
        waiting_update->is_valid = false;
        D3D11_TextureUpdate update = *waiting_update;
        memory_fence();
        ++backend->texture_update_read_index;

        // TODO(hampus): Benchmark these
#if 0
        D3D11_BOX box = { 0 };
        box.left      = update.x;
        box.top       = update.y;
        box.right     = update.x + update.width;
        box.bottom    = update.y + update.height;
        box.front     = 0;
        box.back      = 1;
        ID3D11DeviceContext_UpdateSubresource(
            backend->context,
            resource, 0,
            &box,
            update.data,
            (U32) update.width * 4, 0
        );
#else
        D3D11_MAPPED_SUBRESOURCE mapped;
        ID3D11DeviceContext_Map(backend->context, update.resource, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memory_copy_typed((U8 *) mapped.pData, (U8 *) update.data, (update.width * update.height * 4));
        ID3D11DeviceContext_Unmap(backend->context, update.resource, 0);
#endif
    }

    Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);
    HRESULT hr;
    if (backend->render_target_view == 0 || backend->current_width != client_area.width || backend->current_height != client_area.height)
    {
        if (backend->render_target_view)
        {
            ID3D11DeviceContext_ClearState(backend->context);
            ID3D11RenderTargetView_Release(backend->render_target_view);
            ID3D11DepthStencilView_Release(backend->depth_stencil_view);
            backend->render_target_view = 0;
        }

        if (client_area.width != 0 && client_area.height != 0)
        {
            hr = IDXGISwapChain1_ResizeBuffers(backend->swap_chain, 0, client_area.width, client_area.height, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr))
            {
                assert(!"Failed to resize swap chain!");
            }

            // NOTE(hampus): Create RenderTarget view for new backbuffer texture
            ID3D11Texture2D* backbuffer;
            IDXGISwapChain1_GetBuffer(backend->swap_chain, 0, &IID_ID3D11Texture2D, (Void**) &backbuffer);
            D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc = { .Format = DXGI_FORMAT_R8G8B8A8_UNORM, .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D };
            ID3D11Device_CreateRenderTargetView(backend->device, (ID3D11Resource*) backbuffer, &render_target_view_desc, &backend->render_target_view);
            ID3D11Texture2D_Release(backbuffer);

            D3D11_TEXTURE2D_DESC depthDesc =
            {
                .Width = client_area.width,
                .Height = client_area.height,
                .MipLevels = 1,
                .ArraySize = 1,
                .Format = DXGI_FORMAT_D32_FLOAT,
                .SampleDesc = { 1, 0 },
                .Usage = D3D11_USAGE_DEFAULT,
                .BindFlags = D3D11_BIND_DEPTH_STENCIL,
            };

            // NOTE(hampus): Create new depth stencil texture & DepthStencil view
            ID3D11Texture2D* depth;
            ID3D11Device_CreateTexture2D(backend->device, &depthDesc, 0, &depth);
            ID3D11Device_CreateDepthStencilView(backend->device, (ID3D11Resource*) depth, 0, &backend->depth_stencil_view);
            ID3D11Texture2D_Release(depth);
        }
        backend->current_width = client_area.width;
        backend->current_height = client_area.height;
    }

    F32 gamma = 2.2f;

    Vec4F32 white = v4f32(1, 1, 1, 1);

    if (backend->render_target_view)
    {
        D3D11_VIEWPORT viewport =
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT) client_area.width,
            .Height = (FLOAT) client_area.height,
            .MinDepth = 0,
            .MaxDepth = 1,
        };

        Render_RenderStats *stats = d3d11_get_current_stats(renderer);
        Vec4F32 clear_color = vec4f32_srgb_to_linear(v4f32(0, 0, 0, 1.f));

        FLOAT color[] = { clear_color.r, clear_color.g, clear_color.b, clear_color.a };
        ID3D11DeviceContext_ClearRenderTargetView(backend->context, backend->render_target_view, color);
        ID3D11DeviceContext_ClearDepthStencilView(backend->context, backend->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
        D3D11_BatchList *batch_list = &backend->batch_list;
        for (D3D11_Batch *batch = batch_list->first; batch != 0; batch = batch->next)
        {
            D3D11_BatchParams *params = &batch->params;

            // NOTE(hampus): Setup uniform buffer
            {
                Mat4F32 transform = m4f32_ortho(0, (F32) client_area.width, (F32) client_area.height, 0, -1.0f, 1.0f);

                D3D11_MAPPED_SUBRESOURCE mapped;
                ID3D11DeviceContext_Map(backend->context, (ID3D11Resource*) backend->uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memory_copy_typed((U8 *) mapped.pData, &transform.m, sizeof(transform));
                ID3D11DeviceContext_Unmap(backend->context, (ID3D11Resource*) backend->uniform_buffer, 0);
            }

            // NOTE(hampus): Setup vertex buffer
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                ID3D11DeviceContext_Map(backend->context, (ID3D11Resource*) backend->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memory_copy_typed((U8 *) mapped.pData, (U8 *) batch->instances, sizeof(Render_RectInstance) * batch->instance_count);
                ID3D11DeviceContext_Unmap(backend->context, (ID3D11Resource*) backend->vertex_buffer, 0);
            }

            // NOTE(hampus): Input Assembler
            ID3D11DeviceContext_IASetInputLayout(backend->context, backend->input_layout);
            ID3D11DeviceContext_IASetPrimitiveTopology(backend->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            UINT stride = sizeof(Render_RectInstance);
            UINT offset = 0;
            ID3D11DeviceContext_IASetVertexBuffers(backend->context, 0, 1, &backend->vertex_buffer, &stride, &offset);

            // NOTE(hampus): Vertex Shader
            ID3D11DeviceContext_VSSetConstantBuffers(backend->context, 0, 1, &backend->uniform_buffer);
            ID3D11DeviceContext_VSSetShader(backend->context, backend->vertex_shader, 0, 0);

            D3D11_RECT rect;
            rect.left   = (LONG) params->clip_rect->rect.x0;
            rect.top    = (LONG) params->clip_rect->rect.y0;
            rect.right  = (LONG) params->clip_rect->rect.x1;
            rect.bottom = (LONG) params->clip_rect->rect.y1;

            // NOTE(hampus): Rasterizer Stage
            ID3D11DeviceContext_RSSetViewports(backend->context, 1, &viewport);
            ID3D11DeviceContext_RSSetState(backend->context, backend->rasterizer_state);
            ID3D11DeviceContext_RSSetScissorRects(backend->context, 1, &rect);

            ID3D11ShaderResourceView *texture_view = ptr_from_int(batch->params.texture.u64[0]);

            // NOTE(hampus): Pixel Shader
            ID3D11DeviceContext_PSSetSamplers(backend->context, 0, 1, &backend->sampler);
            ID3D11DeviceContext_PSSetShaderResources(backend->context, 0, 1, &texture_view);
            ID3D11DeviceContext_PSSetShader(backend->context, backend->pixel_shader, 0, 0);

            // NOTE(hampus): Output Merger
            ID3D11DeviceContext_OMSetBlendState(backend->context, backend->blend_state, 0, ~0U);
            ID3D11DeviceContext_OMSetDepthStencilState(backend->context, backend->depth_state, 0);
            ID3D11DeviceContext_OMSetRenderTargets(backend->context, 1, &backend->render_target_view, backend->depth_stencil_view);

            // NOTE(hampus): Draw
            assert(batch->instance_count <= U32_MAX);
            ID3D11DeviceContext_DrawInstanced(backend->context, 4, (U32) batch->instance_count, 0, 0);
        }
    }

    BOOL vsync = TRUE;
    hr = IDXGISwapChain1_Present(backend->swap_chain, vsync ? 1 : 0, 0);
    if (hr == DXGI_STATUS_OCCLUDED)
    {
        // NOTE(hampus): Window is minimized, cannot vsync - instead sleep a bit
        if (vsync)
        {
            Sleep(10);
        }
    }
    else if (FAILED(hr))
    {
        assert(!"Failed to present swap chain! Device lost?");
    }

    // NOTE(hampus): Reset state
    render_pop_clip(renderer);
    swap(renderer->render_stats[0], renderer->render_stats[1], Render_RenderStats);
    memory_zero_struct(&renderer->render_stats[0]);
    backend->batch_list.first = 0;
    backend->batch_list.last  = 0;
    backend->batch_list.batch_count  = 0;
}

internal Render_RectInstance *
render_rect_(Render_Context *renderer, Vec2F32 min, Vec2F32 max, Render_RectParams *params)
{
    Render_BackendContext *backend = renderer->backend;
    if (params->slice.texture.u64[0] == 0)
    {
        params->slice.texture = backend->white_texture;
    }
    D3D11_BatchList *batch_list = &backend->batch_list;
    D3D11_Batch *batch = batch_list->last;

    Render_RectInstance *instance = &render_rect_instance_null;

    // NOTE(simon): Account for softness.
    RectF32 expanded_area = rectf32(
        v2f32_sub_f32(min, params->softness),
        v2f32_add_f32(max, params->softness)
    );

    if (!rectf32_overlaps(expanded_area, d3d11_top_clip(renderer)->rect))
    {
        return(instance);
    }

    B32 is_different_clip   = (batch->params.clip_rect != backend->clip_rect_stack.first);
    B32 inside_current_clip = rectf32_contains_rectf32(d3d11_top_clip(renderer)->rect, expanded_area);
    B32 inside_batch_clip   = rectf32_contains_rectf32(batch->params.clip_rect->rect, expanded_area);
    if ((is_different_clip && !(inside_current_clip && inside_batch_clip)) ||
            batch->instance_count >= D3D11_BATCH_SIZE)
    {
        batch = 0;
    }

    if (batch)
    {
        B32 batch_is_white  = batch->params.texture.u64[0] == backend->white_texture.u64[0];
        B32 params_is_white = params->slice.texture.u64[0] == backend->white_texture.u64[0];
        if (!params_is_white)
        {
            if (!batch_is_white)
            {
                if (batch->params.texture.u64[0] != params->slice.texture.u64[0])
                {
                    // NOTE(hampus): None of them are white and their
                    // texture is not the same.
                    batch = 0;
                }
            }
            else
            {
                // NOTE(hampus): The batch is just white. Overwrite it with
                // the new texture instead
                batch->params.texture = params->slice.texture;
            }
        }
    }

    if (!batch)
    {
        batch = d3d11_push_batch(renderer);
        batch->params.texture = params->slice.texture;
    }

    instance = batch->instances + batch->instance_count;

    instance->min              = min;
    instance->max              = max;
    instance->min_uv           = params->slice.region.min;
    instance->max_uv           = params->slice.region.max;
    instance->colors[0]        = params->color;
    instance->colors[1]        = params->color;
    instance->colors[2]        = params->color;
    instance->colors[3]        = params->color;
    instance->radies[0]        = params->radius;
    instance->radies[1]        = params->radius;
    instance->radies[2]        = params->radius;
    instance->radies[3]        = params->radius;
    instance->softness         = params->softness;
    instance->border_thickness = params->border_thickness;
    instance->omit_texture     = (F32) (params->slice.texture.u64[0] == backend->white_texture.u64[0]);
    instance->is_subpixel_text = (F32) params->is_subpixel_text;
    instance->use_nearest = (F32) params->use_nearest;

    batch->instance_count++;

    Render_RenderStats *stats = d3d11_get_current_stats(renderer);
    stats->rect_count++;

    return(instance);
}

internal Void
render_push_clip(Render_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    RectF32 rect = { min, max };
    if (clip_to_parent)
    {
        RectF32 top_clip_rect = d3d11_top_clip(renderer)->rect;
        rect.x0 = f32_clamp(top_clip_rect.x0, rect.x0, top_clip_rect.x1);
        rect.y0 = f32_clamp(top_clip_rect.y0, rect.y0, top_clip_rect.y1);
        rect.x1 = f32_clamp(top_clip_rect.x0, rect.x1, top_clip_rect.x1);
        rect.y1 = f32_clamp(top_clip_rect.y0, rect.y1, top_clip_rect.y1);
    }
    D3D11_ClipRect *node = push_struct(renderer->frame_arena, D3D11_ClipRect);
    node->rect = rect;
    stack_push(renderer->backend->clip_rect_stack.first, node);
}

internal Void
render_pop_clip(Render_Context *renderer)
{
    stack_pop(renderer->backend->clip_rect_stack.first);
}

typedef struct Render_D3D11_Texture Render_D3D11_Texture;
struct Render_D3D11_Texture
{
    ID3D11ShaderResourceView *texture_view;
    U64 width;
    U64 height;
    ID3D11Texture2D *texture;
};

internal Render_Texture
render_create_texture_from_bitmap(Render_Context *renderer, Void *memory, U32 width, U32 height, Render_ColorSpace color_space)
{
    Render_Texture result = { 0 };
    DXGI_FORMAT d3d11_format = { 0 };
    switch (color_space)
    {
        case Render_ColorSpace_sRGB:   d3d11_format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case Render_ColorSpace_Linear: d3d11_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
            invalid_case;
    }

    D3D11_TEXTURE2D_DESC desc =
    {
        .Width = width,
        .Height = height,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format = d3d11_format,
        .SampleDesc = { 1, 0 },
        .Usage = D3D11_USAGE_DYNAMIC,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
    };

    D3D11_SUBRESOURCE_DATA data =
    {
        .pSysMem = memory,
        .SysMemPitch = width * sizeof(U32),
    };

    ID3D11Texture2D *texture;
    ID3D11ShaderResourceView *texture_view;
    ID3D11Device_CreateTexture2D(renderer->backend->device, &desc, &data, &texture);
    ID3D11Device_CreateShaderResourceView(renderer->backend->device, (ID3D11Resource*) texture, 0, &texture_view);

    result.u64[0] = int_from_ptr(texture_view);
    result.u64[1] = (U64) width;
    result.u64[2] = (U64) height;
    result.u64[3] = int_from_ptr(texture);
    return(result);
}

internal Render_Texture
render_create_texture(Render_Context *renderer, Str8 path)
{
    Render_Texture result = { 0 };
    Str8 file  = { 0 };
    Arena_Temporary scratch = get_scratch(0, 0);
    if (os_file_read(scratch.arena, path, &file))
    {
        Image image = { 0 };
        if (image_load(scratch.arena, file, &image))
        {
            result = render_create_texture_from_bitmap(renderer, image.pixels, image.width, image.height, image.color_space);
        }
        else
        {
            // TODO(hampus): Logging
        }
    }
    else
    {
        // TODO(hampus): Logging
    }
    release_scratch(scratch);
    return(result);
}

internal Void
render_destroy_texture(Render_Context *renderer, Render_Texture texture)
{
    Render_D3D11_Texture *d3d11_texture = (Render_D3D11_Texture *) texture.u64;
    // TODO(hampus): How to release texture view?
}

internal Void
render_update_texture(Render_Context *renderer, Render_Texture texture, Void *memory, U32 width, U32 height, U32 offset)
{
    Render_BackendContext *backend = renderer->backend;

    U32 queue_index = u32_atomic_add(&backend->texture_update_write_index, 1);
    while (queue_index - backend->texture_update_read_index >= D3D11_TEXTURE_UPDATE_QUEUE_SIZE)
    {
        // NOTE(simon): The queue is full, so busy wait. This should not be
        // that common.
    }

    D3D11_TextureUpdate *update = &backend->texture_update_queue[queue_index & D3D11_TEXTURE_UPDATE_QUEUE_MASK];

    Render_D3D11_Texture *d3d11_texture = (Render_D3D11_Texture *) texture.u64;
    ID3D11Resource *resource = (ID3D11Resource *) d3d11_texture->texture;

    update->resource = resource;
    update->x        = (U32) (offset % d3d11_texture->width);
    update->y        = (U32) (offset / d3d11_texture->width);
    update->width    = width;
    update->height   = height;
    update->data     = memory;

    memory_fence();

    update->is_valid = true;
}
