// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2024 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once
#include "../ffx_api.h"
#include "../ffx_api_types.h"
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX11 0x0000002u
struct ffxCreateBackendDX11Desc
{
    ffxCreateContextDescHeader header;
    ID3D11Device              *device;  ///< Device on which the backend will run.
};

#define FFX_API_EFFECT_ID_FRAMEGENERATIONSWAPCHAIN_DX11 0x00030000u

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX11 0x30001u
struct ffxCreateContextDescFrameGenerationSwapChainWrapDX11
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain1** swapchain;        ///< Input swap chain to wrap, output frame interpolation swapchain.
    ID3D11DeviceContext* gameQueue;     ///< Input command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX11 0x30005u
struct ffxCreateContextDescFrameGenerationSwapChainNewDX11
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain1** swapchain;        ///< Output frame interpolation swapchain.
    DXGI_SWAP_CHAIN_DESC* desc;         ///< Swap chain creation parameters.
    IDXGIFactory* dxgiFactory;          ///< IDXGIFactory to use for DX11 swapchain creation.
    ID3D11DeviceContext* gameQueue;     ///< Input command queue to be used for presentation.
};

#define FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX11 0x30006u
struct ffxCreateContextDescFrameGenerationSwapChainForHwndDX11
{
    ffxCreateContextDescHeader header;
    IDXGISwapChain1** swapchain;                     ///< Output frame interpolation swapchain.
    HWND hwnd;                                       ///< HWND handle for the calling application;
    DXGI_SWAP_CHAIN_DESC1* desc;                     ///< Swap chain creation parameters.
    DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fullscreenDesc; ///< Fullscreen swap chain creation parameters.
    IDXGIFactory* dxgiFactory;                       ///< IDXGIFactory to use for DX11 swapchain creation.
    ID3D11DeviceContext* gameQueue;                  ///< Input command queue to be used for presentation.
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX11 0x30002u
struct ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX11
{
    ffxConfigureDescHeader header;
    struct FfxApiResource  uiResource;   ///< Resource containing user interface for composition. May be empty.
    uint32_t               flags;        ///< Zero or combination of values from FfxApiUiCompositionFlags.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX11 0x30003u
struct ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX11
{
    ffxQueryDescHeader header;
    void** pOutCommandList;             ///< Output command list (ID3D12GraphicsCommandList) to be used for frame generation dispatch.
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX11 0x30004u
struct ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX11
{
    ffxQueryDescHeader header;
    struct FfxApiResource *pOutTexture; ///< Output resource in which the frame interpolation result should be placed.
};

#define FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX11 0x30007u
struct ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX11
{
    ffxDispatchDescHeader header;
};

#define FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX11 0x30008u
struct ffxConfigureDescFrameGenerationSwapChainKeyValueDX11
{
    ffxConfigureDescHeader  header;
    uint64_t                key;        ///< Configuration key, member of the FfxApiConfigureFrameGenerationSwapChainKeyDX11 enumeration.
    uint64_t                u64;        ///< Integer value or enum value to set.
    void*                   ptr;        ///< Pointer to set or pointer to value to set.
};

//enum value matches enum FfxFrameInterpolationSwapchainConfigureKey
enum FfxApiConfigureFrameGenerationSwapChainKeyDX11
{
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_WAITCALLBACK = 0,                     ///< Sets FfxWaitCallbackFunc
    FFX_API_CONFIGURE_FG_SWAPCHAIN_KEY_FRAMEPACINGTUNING = 2,                ///< Sets FfxApiSwapchainFramePacingTuning
};

#define FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX11 0x00030009u
struct ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX11
{
    ffxQueryDescHeader header;
    struct FfxApiEffectMemoryUsage* gpuMemoryUsageFrameGenerationSwapchain;
};

#if defined(__cplusplus)

static inline uint32_t ffxApiGetSurfaceFormatDX11(DXGI_FORMAT format)
{
    switch (format)
    {
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        return FFX_API_SURFACE_FORMAT_R32G32B32A32_FLOAT;
    case DXGI_FORMAT_R32G32B32A32_UINT:
        return FFX_API_SURFACE_FORMAT_R32G32B32A32_UINT;
    //case DXGI_FORMAT_R32G32B32A32_SINT:
    //case DXGI_FORMAT_R32G32B32_TYPELESS:
    //case DXGI_FORMAT_R32G32B32_FLOAT:
    //case DXGI_FORMAT_R32G32B32_UINT:
    //case DXGI_FORMAT_R32G32B32_SINT:

    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R16G16B16A16_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        return FFX_API_SURFACE_FORMAT_R16G16B16A16_FLOAT;
    //case DXGI_FORMAT_R16G16B16A16_UNORM:
    //case DXGI_FORMAT_R16G16B16A16_UINT:
    //case DXGI_FORMAT_R16G16B16A16_SNORM:
    //case DXGI_FORMAT_R16G16B16A16_SINT:

    case DXGI_FORMAT_R32G32_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R32G32_TYPELESS;
    case DXGI_FORMAT_R32G32_FLOAT:
        return FFX_API_SURFACE_FORMAT_R32G32_FLOAT;
    //case DXGI_FORMAT_R32G32_FLOAT:
    //case DXGI_FORMAT_R32G32_UINT:
    //case DXGI_FORMAT_R32G32_SINT:

    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R32_FLOAT;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R32_UINT;

    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return FFX_API_SURFACE_FORMAT_R8_UINT;

    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_UNORM:
        return FFX_API_SURFACE_FORMAT_R10G10B10A2_UNORM;
    //case DXGI_FORMAT_R10G10B10A2_UINT:
    
    case DXGI_FORMAT_R11G11B10_FLOAT:
        return FFX_API_SURFACE_FORMAT_R11G11B10_FLOAT;

    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_R8G8B8A8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_SRGB;
    //case DXGI_FORMAT_R8G8B8A8_UINT:
    case DXGI_FORMAT_R8G8B8A8_SNORM:
        return FFX_API_SURFACE_FORMAT_R8G8B8A8_SNORM;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_UNORM:
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return FFX_API_SURFACE_FORMAT_B8G8R8A8_SRGB;

    case DXGI_FORMAT_R16G16_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R16G16_TYPELESS;
    case DXGI_FORMAT_R16G16_FLOAT:
        return FFX_API_SURFACE_FORMAT_R16G16_FLOAT;
    //case DXGI_FORMAT_R16G16_UNORM:
    case DXGI_FORMAT_R16G16_UINT:
        return FFX_API_SURFACE_FORMAT_R16G16_UINT;
    //case DXGI_FORMAT_R16G16_SNORM
    //case DXGI_FORMAT_R16G16_SINT 

    //case DXGI_FORMAT_R32_SINT:
    case DXGI_FORMAT_R32_UINT:
        return FFX_API_SURFACE_FORMAT_R32_UINT;
    case DXGI_FORMAT_R32_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R32_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return FFX_API_SURFACE_FORMAT_R32_FLOAT;

    case DXGI_FORMAT_R8G8_UINT:
        return FFX_API_SURFACE_FORMAT_R8G8_UINT;
    case DXGI_FORMAT_R8G8_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R8G8_TYPELESS;
    case DXGI_FORMAT_R8G8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8G8_UNORM;
    //case DXGI_FORMAT_R8G8_SNORM:
    //case DXGI_FORMAT_R8G8_SINT:

    case DXGI_FORMAT_R16_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R16_TYPELESS;
    case DXGI_FORMAT_R16_FLOAT:
        return FFX_API_SURFACE_FORMAT_R16_FLOAT;
    case DXGI_FORMAT_R16_UINT:
        return FFX_API_SURFACE_FORMAT_R16_UINT;
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return FFX_API_SURFACE_FORMAT_R16_UNORM;
    case DXGI_FORMAT_R16_SNORM:
        return FFX_API_SURFACE_FORMAT_R16_SNORM;
    //case DXGI_FORMAT_R16_SINT:

    case DXGI_FORMAT_R8_TYPELESS:
        return FFX_API_SURFACE_FORMAT_R8_TYPELESS;
    case DXGI_FORMAT_R8_UNORM:
    case DXGI_FORMAT_A8_UNORM:
        return FFX_API_SURFACE_FORMAT_R8_UNORM;
    case DXGI_FORMAT_R8_UINT:
        return FFX_API_SURFACE_FORMAT_R8_UINT;
    //case DXGI_FORMAT_R8_SNORM:
    //case DXGI_FORMAT_R8_SINT:
    //case DXGI_FORMAT_R1_UNORM:

    case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
        return FFX_API_SURFACE_FORMAT_R9G9B9E5_SHAREDEXP;

    case DXGI_FORMAT_UNKNOWN:
    default:
        return FFX_API_SURFACE_FORMAT_UNKNOWN;
    }
}

static inline FfxApiResource ffxApiGetResourceDX11(ID3D11Resource* pRes, uint32_t state = FFX_API_RESOURCE_STATE_COMPUTE_READ, uint32_t additionalUsages = 0)
{
    FfxApiResource res{};
    res.resource = pRes;
    res.state = state;
    if (!pRes) return res;

    DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
    D3D11_RESOURCE_DIMENSION dimension = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pRes->GetType(&dimension);

    if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
    {
        D3D11_BUFFER_DESC desc = {};
        reinterpret_cast<ID3D11Buffer*>(pRes)->GetDesc(&desc);

        res.description.flags = FFX_API_RESOURCE_FLAGS_NONE;
        res.description.usage = FFX_API_RESOURCE_USAGE_UAV;
        res.description.size = static_cast<uint32_t>(desc.ByteWidth);
        res.description.stride = static_cast<uint32_t>(desc.StructureByteStride);
        res.description.type = FFX_API_RESOURCE_TYPE_BUFFER;
    }
    else
    {
        UINT bindFlags = 0;

        res.description.flags = FFX_API_RESOURCE_FLAGS_NONE;
       
        switch (dimension)
        {
        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        {
            D3D11_TEXTURE1D_DESC desc = {};
            reinterpret_cast<ID3D11Texture1D*>(pRes)->GetDesc(&desc);

            format = desc.Format;
            bindFlags = desc.BindFlags;

            res.description.width = static_cast<uint32_t>(desc.Width);
            res.description.height = static_cast<uint32_t>(1);
            res.description.depth = static_cast<uint32_t>(1);
            res.description.mipCount = static_cast<uint32_t>(desc.MipLevels);
            res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE1D;
            break;
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        {
            D3D11_TEXTURE2D_DESC desc = {};
            reinterpret_cast<ID3D11Texture2D*>(pRes)->GetDesc(&desc);

            format = desc.Format;
            bindFlags = desc.BindFlags;

            res.description.width = static_cast<uint32_t>(desc.Width);
            res.description.height = static_cast<uint32_t>(desc.Height);
            res.description.depth = static_cast<uint32_t>(desc.ArraySize);
            res.description.mipCount = static_cast<uint32_t>(desc.MipLevels);
            if (desc.MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
                res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE_CUBE;
            else
                res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE2D;
            break;
        }
        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        {
            D3D11_TEXTURE3D_DESC desc = {};
            reinterpret_cast<ID3D11Texture3D*>(pRes)->GetDesc(&desc);

            format = desc.Format;
            bindFlags = desc.BindFlags;

            res.description.width = static_cast<uint32_t>(desc.Width);
            res.description.height = static_cast<uint32_t>(desc.Height);
            res.description.depth = static_cast<uint32_t>(desc.Depth);
            res.description.mipCount = static_cast<uint32_t>(desc.MipLevels);
            res.description.type = FFX_API_RESOURCE_TYPE_TEXTURE3D;
            break;
        }
        default:
            break;
        }

        if (format == DXGI_FORMAT_D16_UNORM || format == DXGI_FORMAT_D32_FLOAT)
        {
            res.description.usage = FFX_API_RESOURCE_USAGE_DEPTHTARGET;
        }
        else if (format == DXGI_FORMAT_D24_UNORM_S8_UINT || format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT)
        {
            res.description.usage = FFX_API_RESOURCE_USAGE_DEPTHTARGET | FFX_API_RESOURCE_USAGE_STENCILTARGET;
        }
        else
        {
            res.description.usage = FFX_API_RESOURCE_USAGE_READ_ONLY;
        }

        if (bindFlags & D3D11_BIND_UNORDERED_ACCESS)
            res.description.usage |= FFX_API_RESOURCE_USAGE_UAV;
    }

    res.description.format = ffxApiGetSurfaceFormatDX11(format);
    res.description.usage |= additionalUsages;
    return res;
}

#endif
