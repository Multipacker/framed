#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#define assert_hr(hr) assert(SUCCEEDED(hr))

#include <intrin.h>

internal F32
f32_pow_intrin(F32 a, F32 b)
{
    F32 result = _mm_cvtss_f32(_mm_pow_ps(_mm_set1_ps(a), _mm_set1_ps(b)));
    return(result);
}

internal Vec4F32
linear_from_srgb(Vec4F32 c)
{
    F32 gamma = 2.2f;
    Vec4F32 result = c;
    result.r = f32_pow_intrin(c.r, gamma);
    result.g = f32_pow_intrin(c.g, gamma);
    result.b = f32_pow_intrin(c.b, gamma);
    result.a = c.a;
    return(result);
}

internal D3D11_ClipRect *
d3d11_top_clip(R_Context *renderer)
{
    return(renderer->clip_rect_stack.first);
}

internal R_RenderStats *
d3d11_get_current_stats(R_Context *renderer)
{
    return(&renderer->render_stats[0]);
}

internal R_Context *
render_init(Gfx_Context *gfx)
{
    Arena *arena = arena_create();
    R_Context *result = push_struct(arena, R_Context);
    result->perm_arena = arena;
    result->frame_arena = arena_create();
    result->gfx = gfx;

    F32 test = 0xff;

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
        hr = D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, levels, ARRAYSIZE(levels),
                               D3D11_SDK_VERSION, &result->device, 0, &result->context);
        assert_hr(hr);
    }

    // NOTE(hampus): Enable useful debug break on errors
#if !BUILD_MODE_RELEASE
    {
        ID3D11Device_QueryInterface(result->device, &IID_ID3D11InfoQueue, (Void**)&info);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        ID3D11InfoQueue_SetBreakOnSeverity(info, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
        ID3D11InfoQueue_Release(info);
    }

    {
        hr = DXGIGetDebugInterface1(0, &IID_IDXGIInfoQueue, (Void**)&dxgi_info);
        IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        IDXGIInfoQueue_SetBreakOnSeverity(dxgi_info, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
        IDXGIInfoQueue_Release(dxgi_info);
    }
#endif

    // NOTE(hampus): Create swap chain
    {
        hr = ID3D11Device_QueryInterface(result->device, &IID_IDXGIDevice, (Void**)&dxgi_device);
        assert_hr(hr);

        hr = IDXGIDevice_GetAdapter(dxgi_device, &dxgi_adapter);
        assert_hr(hr);

        hr = IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, (Void**)&factory);
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

        hr = IDXGIFactory2_CreateSwapChainForHwnd(factory, (IUnknown*)result->device, gfx->hwnd, &desc, 0, 0, &result->swap_chain);
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
            .ByteWidth = D3D11_BATCH_SIZE * sizeof(R_RectInstance),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE
        };

        ID3D11Device_CreateBuffer(result->device, &desc, 0, &result->vertex_buffer);
    }

    // NOTE(hampus): Create shaders
    {
        D3D11_INPUT_ELEMENT_DESC desc[] =
        {
            { "MIN", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(R_RectInstance, min), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            { "MAX", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(R_RectInstance, max), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            { "MIN_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(R_RectInstance, min_uv), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            { "MAX_UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
                member_offset(R_RectInstance, max_uv), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(R_RectInstance, colors), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"COLOR", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(R_RectInstance, colors)+sizeof(Vec4F32), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"COLOR", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(R_RectInstance, colors)+sizeof(Vec4F32)*2, D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"COLOR", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(R_RectInstance, colors)+sizeof(Vec4F32)*3, D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"CORNER_RADIUS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
                member_offset(R_RectInstance, radies), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"SOFTNESS", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(R_RectInstance, softness), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"BORDER_THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(R_RectInstance, border_thickness), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"EMIT_TEXTURE", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(R_RectInstance, omit_texture), D3D11_INPUT_PER_INSTANCE_DATA, 1},

            {"IS_SUBPIXEL_TEXT", 0, DXGI_FORMAT_R32_FLOAT, 0,
                member_offset(R_RectInstance, is_subpixel_text), D3D11_INPUT_PER_INSTANCE_DATA, 1},

        };

        UINT flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR | D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_WARNINGS_ARE_ERRORS;
#if !BUILD_MODE_RELEASE
        flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

        Arena_Temporary scratch = arena_get_scratch(0, 0);

        Str8 hlsl = {0};
        B32 hlsl_file_read_result = os_file_read(scratch.arena, str8_lit("src/render/d3d11/d3d11_shader.hlsl"), &hlsl);
        assert(hlsl_file_read_result);

        ID3DBlob* error;
        ID3DBlob* vblob;
        hr = D3DCompile(hlsl.data, hlsl.size, 0, 0, 0, "vs", "vs_5_0", flags, 0, &vblob, &error);
        if (FAILED(hr))
        {
            CStr message = ID3D10Blob_GetBufferPointer(error);
            OutputDebugStringA(message);
            assert(!"Failed to compile vertex shader!");
        }

        ID3DBlob* pblob;
        hr = D3DCompile(hlsl.data, hlsl.size, 0, 0, 0, "ps", "ps_5_0", flags, 0, &pblob, &error);
        if (FAILED(hr))
        {
            CStr message = ID3D10Blob_GetBufferPointer(error);
            OutputDebugStringA(message);
            assert(!"Failed to compile pixel shader!");
        }

        ID3D11Device_CreateVertexShader(result->device, ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), 0, &result->vertex_shader);
        ID3D11Device_CreatePixelShader(result->device, ID3D10Blob_GetBufferPointer(pblob), ID3D10Blob_GetBufferSize(pblob), 0, &result->pixel_shader);
        ID3D11Device_CreateInputLayout(result->device, desc, ARRAYSIZE(desc), ID3D10Blob_GetBufferPointer(vblob), ID3D10Blob_GetBufferSize(vblob), &result->input_layout);
        ID3D10Blob_Release(pblob);
        ID3D10Blob_Release(vblob);

        arena_release_scratch(scratch);
    }

    {
        D3D11_BUFFER_DESC desc =
        {
            .ByteWidth = sizeof(Mat4F32),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        ID3D11Device_CreateBuffer(result->device, &desc, 0, &result->uniform_buffer);
    }

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
        ID3D11Device_CreateTexture2D(result->device, &desc, &data, &texture);
        ID3D11Device_CreateShaderResourceView(result->device, (ID3D11Resource*)texture, 0, &texture_view);
        ID3D11Texture2D_Release(texture);

        result->white_texture.u64[0] = int_from_ptr(texture_view);
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

        ID3D11Device_CreateSamplerState(result->device, &desc, &result->sampler);
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
        ID3D11Device_CreateBlendState(result->device, &desc, &result->blend_state);
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
        ID3D11Device_CreateRasterizerState(result->device, &desc, &result->rasterizer_state);
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
        ID3D11Device_CreateDepthStencilState(result->device, &desc, &result->depth_state);
    }

    return(result);
}

internal D3D11_Batch *
d3d11_push_batch(R_Context *renderer)
{
    D3D11_Batch *result     = push_struct_zero(renderer->frame_arena, D3D11_Batch);
    result->instances = push_array(renderer->frame_arena, R_RectInstance, D3D11_BATCH_SIZE);
    dll_push_back(renderer->batch_list.first, renderer->batch_list.last, result);
    result->params.clip_rect = d3d11_top_clip(renderer);
    renderer->batch_list.batch_count++;
    R_RenderStats *stats = d3d11_get_current_stats(renderer);
    stats->batch_count++;
    return(result);
}

internal Void
render_begin(R_Context *renderer)
{
    // NOTE(hampus): Push clip rect
    Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);
    Vec2F32 max_clip;
    max_clip.x = (F32)client_area.x;
    max_clip.y = (F32)client_area.y;
    render_push_clip(renderer, v2f32(0, 0), max_clip, false);

    // NOTE(hampus): First batch
    D3D11_Batch *first_batch = d3d11_push_batch(renderer);
    first_batch->params.texture = renderer->white_texture;
}

internal Void
render_end(R_Context *renderer)
{
    Vec2U32 client_area = gfx_get_window_client_area(renderer->gfx);
    HRESULT hr;
    if (renderer->render_target_view == 0 || renderer->current_width != client_area.width || renderer->current_height != client_area.height)
    {
        if (renderer->render_target_view)
        {
            ID3D11DeviceContext_ClearState(renderer->context);
            ID3D11RenderTargetView_Release(renderer->render_target_view);
            ID3D11DepthStencilView_Release(renderer->depth_stencil_view);
            renderer->render_target_view = 0;
        }

        if (client_area.width != 0 && client_area.height != 0)
        {
            hr = IDXGISwapChain1_ResizeBuffers(renderer->swap_chain, 0, client_area.width, client_area.height, DXGI_FORMAT_UNKNOWN, 0);
            if (FAILED(hr))
            {
                assert(!"Failed to resize swap chain!");
            }

            // NOTE(hampus): Create RenderTarget view for new backbuffer texture
            ID3D11Texture2D* backbuffer;
            IDXGISwapChain1_GetBuffer(renderer->swap_chain, 0, &IID_ID3D11Texture2D, (Void**)&backbuffer);
            D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc = {.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, .ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D};
            ID3D11Device_CreateRenderTargetView(renderer->device, (ID3D11Resource*)backbuffer, &render_target_view_desc, &renderer->render_target_view);
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
            ID3D11Device_CreateTexture2D(renderer->device, &depthDesc, 0, &depth);
            ID3D11Device_CreateDepthStencilView(renderer->device, (ID3D11Resource*)depth, 0, &renderer->depth_stencil_view);
            ID3D11Texture2D_Release(depth);
        }
        renderer->current_width = client_area.width;
        renderer->current_height = client_area.height;
    }

    F32 gamma = 2.2f;

    Vec4F32 white = v4f32(1, 1, 1, 1);

    if (renderer->render_target_view)
    {
        D3D11_VIEWPORT viewport =
        {
            .TopLeftX = 0,
            .TopLeftY = 0,
            .Width = (FLOAT)client_area.width,
            .Height = (FLOAT)client_area.height,
            .MinDepth = 0,
            .MaxDepth = 1,
        };

            R_RenderStats *stats = d3d11_get_current_stats(renderer);
        Vec4F32 bg_color = linear_from_srgb(v4f32(0.5f, 0.5f, 0.5f, 1.f));

        FLOAT color[] = { bg_color.r, bg_color.g, bg_color.b, bg_color.a };
        ID3D11DeviceContext_ClearRenderTargetView(renderer->context, renderer->render_target_view, color);
        ID3D11DeviceContext_ClearDepthStencilView(renderer->context, renderer->depth_stencil_view, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);
        D3D11_BatchList *batch_list = &renderer->batch_list;
        for (D3D11_Batch *batch = batch_list->first;
             batch != 0;
             batch = batch->next)
        {
            D3D11_BatchParams *params = &batch->params;

            // NOTE(hampus): Setup uniform buffer
            {
                Mat4F32 transform = m4f32_ortho(0, (F32)client_area.width, (F32)client_area.height, 0, -1.0f, 1.0f);

                D3D11_MAPPED_SUBRESOURCE mapped;
                ID3D11DeviceContext_Map(renderer->context, (ID3D11Resource*)renderer->uniform_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memory_copy_typed((U8 *)mapped.pData, &transform.m, sizeof(transform));
                ID3D11DeviceContext_Unmap(renderer->context, (ID3D11Resource*)renderer->uniform_buffer, 0);
            }

            // NOTE(hampus): Setup vertex buffer
            {
                D3D11_MAPPED_SUBRESOURCE mapped;
                ID3D11DeviceContext_Map(renderer->context, (ID3D11Resource*)renderer->vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
                memory_copy_typed((U8 *)mapped.pData, (U8 *)batch->instances, sizeof(R_RectInstance) * batch->instance_count);
                ID3D11DeviceContext_Unmap(renderer->context, (ID3D11Resource*)renderer->vertex_buffer, 0);
            }

            // NOTE(hampus): Input Assembler
            ID3D11DeviceContext_IASetInputLayout(renderer->context, renderer->input_layout);
            ID3D11DeviceContext_IASetPrimitiveTopology(renderer->context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            UINT stride = sizeof(R_RectInstance);
            UINT offset = 0;
            ID3D11DeviceContext_IASetVertexBuffers(renderer->context, 0, 1, &renderer->vertex_buffer, &stride, &offset);

            // NOTE(hampus): Vertex Shader
            ID3D11DeviceContext_VSSetConstantBuffers(renderer->context, 0, 1, &renderer->uniform_buffer);
            ID3D11DeviceContext_VSSetShader(renderer->context, renderer->vertex_shader, 0, 0);

            D3D11_RECT rect;
            rect.left   = (LONG)params->clip_rect->rect.x0;
            rect.top    = (LONG)params->clip_rect->rect.y0;
            rect.right  = (LONG)params->clip_rect->rect.x1;
            rect.bottom = (LONG)params->clip_rect->rect.y1;

            // NOTE(hampus): Rasterizer Stage
            ID3D11DeviceContext_RSSetViewports(renderer->context, 1, &viewport);
            ID3D11DeviceContext_RSSetState(renderer->context, renderer->rasterizer_state);
            ID3D11DeviceContext_RSSetScissorRects(renderer->context, 1, &rect);

            ID3D11ShaderResourceView *texture_view = ptr_from_int(batch->params.texture.u64[0]);

            // NOTE(hampus): Pixel Shader
            ID3D11DeviceContext_PSSetSamplers(renderer->context, 0, 1, &renderer->sampler);
            ID3D11DeviceContext_PSSetShaderResources(renderer->context, 0, 1, &texture_view);
            ID3D11DeviceContext_PSSetShader(renderer->context, renderer->pixel_shader, 0, 0);

            // NOTE(hampus): Output Merger
            ID3D11DeviceContext_OMSetBlendState(renderer->context, renderer->blend_state, 0, ~0U);
            ID3D11DeviceContext_OMSetDepthStencilState(renderer->context, renderer->depth_state, 0);
            ID3D11DeviceContext_OMSetRenderTargets(renderer->context, 1, &renderer->render_target_view, renderer->depth_stencil_view);

            // NOTE(hampus): Draw
            assert(batch->instance_count <= U32_MAX);
            ID3D11DeviceContext_DrawInstanced(renderer->context, 4, (U32)batch->instance_count, 0, 0);
        }
    }

    BOOL vsync = TRUE;
    hr = IDXGISwapChain1_Present(renderer->swap_chain, vsync ? 1 : 0, 0);
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
    arena_pop_to(renderer->frame_arena, 0);
    swap(renderer->render_stats[0], renderer->render_stats[1], R_RenderStats);
    memory_zero_struct(&renderer->render_stats[0]);
    renderer->batch_list.first = 0;
    renderer->batch_list.last  = 0;
    renderer->batch_list.batch_count  = 0;
}

internal R_RectInstance *
render_rect_(R_Context *renderer, Vec2F32 min, Vec2F32 max, R_RectParams *params)
{
    if (params->slice.texture.u64[0] == 0)
    {
        params->slice.texture = renderer->white_texture;
    }
    D3D11_BatchList *batch_list = &renderer->batch_list;
    D3D11_Batch *batch = batch_list->last;

    R_RectInstance *instance = &render_rect_instance_null;

	// NOTE(simon): Account for softness.
	RectF32 expanded_area = rectf32(
                                    v2f32_sub_f32(min, params->softness),
                                    v2f32_add_f32(max, params->softness)
                                    );

	if (!rectf32_overlaps(expanded_area, d3d11_top_clip(renderer)->rect))
	{
		return(instance);
	}

	B32 is_different_clip   = (batch->params.clip_rect != renderer->clip_rect_stack.first);
	B32 inside_current_clip = rectf32_contains_rectf32(d3d11_top_clip(renderer)->rect, expanded_area);
	B32 inside_batch_clip   = rectf32_contains_rectf32(d3d11_top_clip(renderer)->rect, expanded_area);
	if ((is_different_clip && !(inside_current_clip && inside_batch_clip)) ||
        batch->instance_count >= D3D11_BATCH_SIZE)
	{
          batch = 0;
	}

    if (batch)
    {
        B32 batch_is_white  = batch->params.texture.u64[0] == renderer->white_texture.u64[0];
            B32 params_is_white = params->slice.texture.u64[0] == renderer->white_texture.u64[0];
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
    instance->omit_texture     = (F32)(params->slice.texture.u64[0] == renderer->white_texture.u64[0]);
    instance->is_subpixel_text = (F32)params->is_subpixel_text;

    batch->instance_count++;

    R_RenderStats *stats = d3d11_get_current_stats(renderer);
    stats->rect_count++;

    return(instance);
}

internal Void
render_push_clip(R_Context *renderer, Vec2F32 min, Vec2F32 max, B32 clip_to_parent)
{
    RectF32 rect = {min, max};
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
    stack_push(renderer->clip_rect_stack.first, node);
}

internal Void
render_pop_clip(R_Context *renderer)
{
    stack_pop(renderer->clip_rect_stack.first);
}

internal R_RenderStats
render_get_stats(R_Context *renderer)
{
    return(renderer->render_stats[1]);
}

internal R_Texture
render_create_texture_from_bitmap(R_Context *renderer, Void *memory, S32 width, S32 height, R_ColorSpace color_space)
{
    R_Texture result = {0};
    DXGI_FORMAT d3d11_format = {0};
    switch (color_space)
    {
        case R_ColorSpace_sRGB:   d3d11_format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; break;
        case R_ColorSpace_Linear: d3d11_format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
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
        .Usage = D3D11_USAGE_IMMUTABLE,
        .BindFlags = D3D11_BIND_SHADER_RESOURCE,
    };

    D3D11_SUBRESOURCE_DATA data =
    {
        .pSysMem = memory,
        .SysMemPitch = width * sizeof(U32),
    };

    ID3D11Texture2D *texture;
    ID3D11ShaderResourceView *texture_view;
    ID3D11Device_CreateTexture2D(renderer->device, &desc, &data, &texture);
    ID3D11Device_CreateShaderResourceView(renderer->device, (ID3D11Resource*)texture, 0, &texture_view);
    ID3D11Texture2D_Release(texture);

    result.u64[0] = int_from_ptr(texture_view);
    return(result);
}

internal R_Texture
render_create_texture(R_Context *renderer, Str8 path, R_ColorSpace color_space)
{
    R_Texture result = {0};
    Str8 file  = {0};
    Arena_Temporary scratch = arena_get_scratch(0, 0);
    if (os_file_read(scratch.arena, path, &file))
    {
        S32 width, height, channels;
        // NOTE(hampus): We only want images with 4 components, RGBA
        S32 req_components = 4;
        assert(file.size <= U32_MAX);
        Void *memory = stbi_load_from_memory(file.data, (U32)file.size, &width, &height, &channels, req_components);
        if (memory)
        {
            if (width && height)
            {
                result = render_create_texture_from_bitmap(renderer, memory, width, height, color_space);
                stbi_image_free(memory);
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
    }
    else
    {
        // TODO(hampus): Logging
    }
                         arena_release_scratch(scratch);
    return(result);
}

internal Void
render_destroy_texture(R_Context *renderer, R_Texture texture)
{
    // TODO(hampus): How to release texture view?
}
