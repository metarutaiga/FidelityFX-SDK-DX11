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

#include "../ffx_api.hpp"
#include "ffx_api_dx11.h"

// Helper types for header initialization. Api definition is in .h file.

namespace ffx
{

template<>
struct struct_type<ffxCreateBackendDX11Desc> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_BACKEND_DX11> {};

struct CreateBackendDX11Desc : public InitHelper<ffxCreateBackendDX11Desc> {};

template<>
struct struct_type<ffxCreateContextDescFrameGenerationSwapChainWrapDX11> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX11> {};

struct CreateContextDescFrameGenerationSwapChainWrapDX11 : public InitHelper<ffxCreateContextDescFrameGenerationSwapChainWrapDX11> {};

template<>
struct struct_type<ffxCreateContextDescFrameGenerationSwapChainNewDX11> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_NEW_DX11> {};

struct CreateContextDescFrameGenerationSwapChainNewDX11 : public InitHelper<ffxCreateContextDescFrameGenerationSwapChainNewDX11> {};

template<>
struct struct_type<ffxCreateContextDescFrameGenerationSwapChainForHwndDX11> : std::integral_constant<uint64_t, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_FOR_HWND_DX11> {};

struct CreateContextDescFrameGenerationSwapChainForHwndDX11 : public InitHelper<ffxCreateContextDescFrameGenerationSwapChainForHwndDX11> {};

template<>
struct struct_type<ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX11> : std::integral_constant<uint64_t, FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_REGISTERUIRESOURCE_DX11> {};

struct ConfigureDescFrameGenerationSwapChainRegisterUiResourceDX11 : public InitHelper<ffxConfigureDescFrameGenerationSwapChainRegisterUiResourceDX11> {};

template<>
struct struct_type<ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX11> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONCOMMANDLIST_DX11> {};

struct QueryDescFrameGenerationSwapChainInterpolationCommandListDX11 : public InitHelper<ffxQueryDescFrameGenerationSwapChainInterpolationCommandListDX11> {};

template<>
struct struct_type<ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX11> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_INTERPOLATIONTEXTURE_DX11> {};

struct QueryDescFrameGenerationSwapChainInterpolationTextureDX11 : public InitHelper<ffxQueryDescFrameGenerationSwapChainInterpolationTextureDX11> {};

template<>
struct struct_type<ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX11> : std::integral_constant<uint64_t, FFX_API_DISPATCH_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WAIT_FOR_PRESENTS_DX11> {};

struct DispatchDescFrameGenerationSwapChainWaitForPresentsDX11 : public InitHelper<ffxDispatchDescFrameGenerationSwapChainWaitForPresentsDX11> {};

template<>
struct struct_type<ffxConfigureDescFrameGenerationSwapChainKeyValueDX11> : std::integral_constant<uint64_t, FFX_API_CONFIGURE_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_KEYVALUE_DX11> {};

struct ConfigureDescFrameGenerationSwapChainKeyValueDX11 : public InitHelper<ffxConfigureDescFrameGenerationSwapChainKeyValueDX11> {};

template<>
struct struct_type<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX11> : std::integral_constant<uint64_t, FFX_API_QUERY_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_GPU_MEMORY_USAGE_DX11> {};

struct QueryFrameGenerationSwapChainGetGPUMemoryUsageDX11 : public InitHelper<ffxQueryFrameGenerationSwapChainGetGPUMemoryUsageDX11> {};

}
