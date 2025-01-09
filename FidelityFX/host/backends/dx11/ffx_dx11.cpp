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

#define INITGUID

#include <host/ffx_interface.h>
#include <host/ffx_util.h>
#include <host/ffx_assert.h>
#include <host/backends/dx11/ffx_dx11.h>
#include <host/backends/ffx_shader_blobs.h>
#include <codecvt>  // convert string to wstring
#include <mutex>

extern "C" void CalculateDXBCChecksum(const DWORD* pData, DWORD dwSize, DWORD dwHash[4]);

// DX11 prototypes for functions in the backend interface
FfxUInt32 GetSDKVersionDX11(FfxInterface* backendInterface);
FfxErrorCode GetEffectGpuMemoryUsageDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage);
FfxErrorCode CreateBackendContextDX11(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId);
FfxErrorCode GetDeviceCapabilitiesDX11(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode DestroyBackendContextDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode CreateResourceDX11(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode DestroyResourceDX11(FfxInterface* backendInterface, FfxResourceInternal resource, FfxUInt32 effectContextId);
FfxErrorCode RegisterResourceDX11(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource GetResourceDX11(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode UnregisterResourcesDX11(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptorDX11(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode StageConstantBufferDataDX11(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer);
FfxErrorCode CreatePipelineDX11(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription*  desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode DestroyPipelineDX11(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode ScheduleGpuJobDX11(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode ExecuteGpuJobsDX11(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);

#define FFX_MAX_RESOURCE_IDENTIFIER_COUNT   (128)

typedef struct BackendContext_DX11 {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        wchar_t                     resourceName[64] = {};
#endif
        ID3D11Resource*             resourcePtr;
        FfxResourceDescription      resourceDescription;
        ID3D11ShaderResourceView*   srvPtr[16];
        ID3D11UnorderedAccessView*  uavPtr[16];
    } Resource;

    uint32_t refCount;
    uint32_t maxEffectContexts;

    ID3D11Device*           device = nullptr;
    ID3D11DeviceContext*    deviceContext = nullptr;
    ID3D11DeviceContext1*   deviceContext1 = nullptr;

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount;

    ID3D11ShaderResourceView* srvs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    ID3D11UnorderedAccessView* uavs[D3D11_1_UAV_SLOT_COUNT];

    uint8_t*                pStagingRingBuffer;
    uint32_t                stagingRingBufferBase;

    typedef struct alignas(32) EffectContext {

        // Resource allocation
        uint32_t            nextStaticResource;
        uint32_t            nextDynamicResource;

        // Usage
        bool                active;

        // VRAM usage
        FfxEffectMemoryUsage vramUsage;

    } EffectContext;

    // Resource holder
    Resource*               pResources;
    EffectContext*          pEffectContexts;

    void*                   constantBufferMem[FFX_MAX_NUM_CONST_BUFFERS];
    ID3D11Buffer*           constantBufferResource[FFX_MAX_NUM_CONST_BUFFERS];
    uint32_t                constantBufferSize[FFX_MAX_NUM_CONST_BUFFERS];
    uint32_t                constantBufferOffset[FFX_MAX_NUM_CONST_BUFFERS];
    std::mutex              constantBufferMutex;

} BackendContext_DX11;

FFX_API size_t ffxGetScratchMemorySizeDX11(size_t maxContexts)
{
    uint32_t resourceArraySize          = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX11::Resource), sizeof(uint64_t));
    uint32_t contextArraySize           = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_DX11::EffectContext), sizeof(uint32_t));
    uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(maxContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
    uint32_t gpuJobDescArraySize        = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));

    return FFX_ALIGN_UP(sizeof(BackendContext_DX11) + resourceArraySize + contextArraySize + stagingRingBufferArraySize + gpuJobDescArraySize, sizeof(uint64_t));
}

// Create a FfxDevice from a ID3D11Device*
FfxDevice ffxGetDeviceDX11(ID3D11Device* dx11Device)
{
    FFX_ASSERT(NULL != dx11Device);
    return reinterpret_cast<FfxDevice>(dx11Device);
}

// populate interface with DX11 pointers.
FfxErrorCode ffxGetInterfaceDX11(
    FfxInterface* backendInterface,
    FfxDevice device,
    void* scratchBuffer,
    size_t scratchBufferSize,
    uint32_t maxContexts) {

    FFX_RETURN_ON_ERROR(
        backendInterface,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBuffer,
        FFX_ERROR_INVALID_POINTER);
    FFX_RETURN_ON_ERROR(
        scratchBufferSize >= ffxGetScratchMemorySizeDX11(maxContexts),
        FFX_ERROR_INSUFFICIENT_MEMORY);

    backendInterface->fpGetSDKVersion = GetSDKVersionDX11;
    backendInterface->fpGetEffectGpuMemoryUsage = GetEffectGpuMemoryUsageDX11;
    backendInterface->fpCreateBackendContext = CreateBackendContextDX11;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesDX11;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextDX11;
    backendInterface->fpCreateResource = CreateResourceDX11;
    backendInterface->fpDestroyResource = DestroyResourceDX11;
    backendInterface->fpMapResource;
    backendInterface->fpUnmapResource;
    backendInterface->fpGetResource = GetResourceDX11;
    backendInterface->fpRegisterResource = RegisterResourceDX11;
    backendInterface->fpUnregisterResources = UnregisterResourcesDX11;
    backendInterface->fpRegisterStaticResource;
    backendInterface->fpGetResourceDescription = GetResourceDescriptorDX11;
    backendInterface->fpStageConstantBufferDataFunc = StageConstantBufferDataDX11;
    backendInterface->fpCreatePipeline = CreatePipelineDX11;
    backendInterface->fpGetPermutationBlobByIndex = ffxGetPermutationBlobByIndex;
    backendInterface->fpDestroyPipeline = DestroyPipelineDX11;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobDX11;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsDX11;
    backendInterface->fpBreadcrumbsAllocBlock;
    backendInterface->fpBreadcrumbsFreeBlock;
    backendInterface->fpBreadcrumbsWrite;
    backendInterface->fpBreadcrumbsPrintDeviceInfo;
    backendInterface->fpSwapChainConfigureFrameGeneration = [](FfxFrameGenerationConfig const*) -> FfxErrorCode { return FFX_OK; };
    backendInterface->fpRegisterConstantBufferAllocator;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FFX_RETURN_ON_ERROR(
        !backendContext->refCount,
        FFX_ERROR_BACKEND_API_ERROR);

    // Clear everything out
    memset(backendContext, 0, sizeof(*backendContext));

    // Set the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    backendContext->maxEffectContexts = maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListDX11(ID3D11DeviceContext* deviceContext)
{
    FFX_ASSERT(NULL != deviceContext);
    return reinterpret_cast<FfxCommandList>(deviceContext);
}

// register a DX11 resource to the backend
FfxResource ffxGetResourceDX11(ID3D11Resource* dx11Resource,
    FfxResourceDescription                     ffxResDescription,
    wchar_t const*                             ffxResName,
    FfxResourceStates                          state /*=FFX_RESOURCE_STATE_COMPUTE_READ*/)
{
    FfxResource resource = {};
    resource.resource    = reinterpret_cast<void*>(const_cast<ID3D11Resource*>(dx11Resource));
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName) {
        wcscpy_s(resource.name, ffxResName);
    }
#endif

    return resource;
}

static void SetNameDX11(ID3D11DeviceChild* resource, wchar_t const* name)
{
    if (resource)
    {
        resource->SetPrivateData(WKPDID_D3DDebugObjectNameW, static_cast<UINT>(wcslen(name)), name);
    }
}

static void TIF(HRESULT result)
{
    if (FAILED(result)) {

        wchar_t errorMessage[256];
        memset(errorMessage, 0, 256);
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, result, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), errorMessage, 255, NULL);
        char errA[256];
        size_t returnSize;
        wcstombs_s(&returnSize, errA, 255, errorMessage, 255);
#ifdef _DEBUG
        int32_t msgboxID = MessageBoxW(NULL, errorMessage, L"Error", MB_OK);
#endif
        throw 1;
    }
}

// fix up format in case resource passed for UAV cannot be mapped
static DXGI_FORMAT convertFormatUav(DXGI_FORMAT format)
{
    switch (format) {
        // Handle Depth
        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
        case DXGI_FORMAT_D32_FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
        case DXGI_FORMAT_D16_UNORM:
            return DXGI_FORMAT_R16_UNORM;

        // Handle color: assume FLOAT for 16 and 32 bit channels, else UNORM
        case DXGI_FORMAT_R32G32B32A32_TYPELESS:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32_TYPELESS:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case DXGI_FORMAT_R32G32_TYPELESS:
            return DXGI_FORMAT_R32G32_FLOAT;
        case DXGI_FORMAT_R16G16_TYPELESS:
            return DXGI_FORMAT_R16G16_FLOAT;
        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case DXGI_FORMAT_B8G8R8A8_TYPELESS:
            return DXGI_FORMAT_B8G8R8A8_UNORM;
        case DXGI_FORMAT_B8G8R8X8_TYPELESS:
            return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
        case DXGI_FORMAT_R32_TYPELESS:
            return DXGI_FORMAT_R32_FLOAT;
        case DXGI_FORMAT_R8G8_TYPELESS:
            return DXGI_FORMAT_R8G8_UNORM;
        case DXGI_FORMAT_R16_TYPELESS:
            return DXGI_FORMAT_R16_FLOAT;
        case DXGI_FORMAT_R8_TYPELESS:
            return DXGI_FORMAT_R8_UNORM;
        default:
            return format;
    }
}

// fix up format in case resource passed for SRV cannot be mapped
static DXGI_FORMAT convertFormatSrv(DXGI_FORMAT format)
{
    switch (format) {
        // Handle Depth
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_UNORM;

        // Handle Color
    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
        return DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

        // Colors can map as is
    default:
        return format;
    }
}

DXGI_FORMAT ffxGetDX11FormatFromSurfaceFormat(FfxSurfaceFormat surfaceFormat)
{
    switch (surfaceFormat) {

        case (FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case (FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case (FFX_SURFACE_FORMAT_R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case (FFX_SURFACE_FORMAT_R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case(FFX_SURFACE_FORMAT_R10G10B10A2_UNORM):
            return DXGI_FORMAT_R10G10B10A2_UNORM;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_SRGB):
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case (FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case (FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case (FFX_SURFACE_FORMAT_R16G16_SINT):
            return DXGI_FORMAT_R16G16_SINT;
        case (FFX_SURFACE_FORMAT_R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case (FFX_SURFACE_FORMAT_R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case (FFX_SURFACE_FORMAT_R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case (FFX_SURFACE_FORMAT_R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case (FFX_SURFACE_FORMAT_R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case (FFX_SURFACE_FORMAT_R8_UINT):
            return DXGI_FORMAT_R8_UINT;
        case (FFX_SURFACE_FORMAT_R8G8_UINT):
            return DXGI_FORMAT_R8G8_UINT;
        case (FFX_SURFACE_FORMAT_R8G8_UNORM):
            return DXGI_FORMAT_R8G8_UNORM;
        case (FFX_SURFACE_FORMAT_R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
        case (FFX_SURFACE_FORMAT_UNKNOWN):
            return DXGI_FORMAT_UNKNOWN;

        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT patchDxgiFormatWithFfxUsage(DXGI_FORMAT dxResFmt, FfxSurfaceFormat ffxFmt)
{
    DXGI_FORMAT fromFfx = ffxGetDX11FormatFromSurfaceFormat(ffxFmt);
    DXGI_FORMAT fmt = dxResFmt;

    switch (fmt)
    {
    // fixup typeless formats with what is passed in the ffxSurfaceFormat
    case DXGI_FORMAT_UNKNOWN:
    case DXGI_FORMAT_R32G32B32A32_TYPELESS:
    case DXGI_FORMAT_R32G32B32_TYPELESS:
    case DXGI_FORMAT_R16G16B16A16_TYPELESS:
    case DXGI_FORMAT_R32G32_TYPELESS:
    case DXGI_FORMAT_R10G10B10A2_TYPELESS:
    case DXGI_FORMAT_R16G16_TYPELESS:
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R8G8_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_R8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
        return fromFfx;

    // fixup RGBA8 with SRGB flag passed in the ffxSurfaceFormat
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return fromFfx;
    
    // fixup depth formats as ffxGetDX12FormatFromSurfaceFormat will result in wrong format
    case DXGI_FORMAT_D32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

    case DXGI_FORMAT_D16_UNORM:
        return DXGI_FORMAT_R16_UNORM;

    default:
        break;
    }
    return fmt;
}

D3D11_BIND_FLAG ffxGetDX11BindFlags(FfxResourceUsage flags)
{
    int dx11ResourceFlags = D3D11_BIND_SHADER_RESOURCE;
    if (flags & FFX_RESOURCE_USAGE_RENDERTARGET) dx11ResourceFlags |= D3D11_BIND_RENDER_TARGET;
    if (flags & FFX_RESOURCE_USAGE_UAV) dx11ResourceFlags |= D3D11_BIND_UNORDERED_ACCESS;
    return D3D11_BIND_FLAG(dx11ResourceFlags);
}

FfxSurfaceFormat ffxGetSurfaceFormatDX11(DXGI_FORMAT format)
{
    switch (format) {

        case(DXGI_FORMAT_R32G32B32A32_TYPELESS):
            return FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS;
        case(DXGI_FORMAT_R32G32B32A32_FLOAT):
            return FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT;
        case DXGI_FORMAT_R32G32B32A32_UINT:
            return FFX_SURFACE_FORMAT_R32G32B32A32_UINT;
        //case DXGI_FORMAT_R32G32B32A32_SINT:
        //case DXGI_FORMAT_R32G32B32_TYPELESS:
        //case DXGI_FORMAT_R32G32B32_FLOAT:
        //case DXGI_FORMAT_R32G32B32_UINT:
        //case DXGI_FORMAT_R32G32B32_SINT:

        case DXGI_FORMAT_R16G16B16A16_TYPELESS:
        case(DXGI_FORMAT_R16G16B16A16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        //case DXGI_FORMAT_R16G16B16A16_UNORM:
        //case DXGI_FORMAT_R16G16B16A16_UINT:
        //case DXGI_FORMAT_R16G16B16A16_SNORM:
        //case DXGI_FORMAT_R16G16B16A16_SINT:

        case DXGI_FORMAT_R32G32_TYPELESS:
        case DXGI_FORMAT_R32G32_FLOAT:
            return FFX_SURFACE_FORMAT_R32G32_FLOAT;
        //case DXGI_FORMAT_R32G32_FLOAT:
        //case DXGI_FORMAT_R32G32_UINT:
        //case DXGI_FORMAT_R32G32_SINT:

        case DXGI_FORMAT_R32G8X24_TYPELESS:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
            return FFX_SURFACE_FORMAT_R32_FLOAT;

        case DXGI_FORMAT_R24G8_TYPELESS:
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
            return FFX_SURFACE_FORMAT_R32_UINT;

        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
            return FFX_SURFACE_FORMAT_R8_UINT;

        case DXGI_FORMAT_R10G10B10A2_TYPELESS:
        case DXGI_FORMAT_R10G10B10A2_UNORM:
            return FFX_SURFACE_FORMAT_R10G10B10A2_UNORM;
        //case DXGI_FORMAT_R10G10B10A2_UINT:
        
        case (DXGI_FORMAT_R11G11B10_FLOAT):
            return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;

        case (DXGI_FORMAT_R8G8B8A8_TYPELESS):
            return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
        case (DXGI_FORMAT_R8G8B8A8_UNORM):
            return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case (DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
            return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
        //case DXGI_FORMAT_R8G8B8A8_UINT:
        case DXGI_FORMAT_R8G8B8A8_SNORM:
            return FFX_SURFACE_FORMAT_R8G8B8A8_SNORM;

        case DXGI_FORMAT_R16G16_TYPELESS:
        case (DXGI_FORMAT_R16G16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16_FLOAT;
        //case DXGI_FORMAT_R16G16_UNORM:
        case (DXGI_FORMAT_R16G16_UINT):
            return FFX_SURFACE_FORMAT_R16G16_UINT;
        //case DXGI_FORMAT_R16G16_SNORM
        //case DXGI_FORMAT_R16G16_SINT 

        //case DXGI_FORMAT_R32_SINT:
        case DXGI_FORMAT_R32_UINT:
            return FFX_SURFACE_FORMAT_R32_UINT;
        case DXGI_FORMAT_R32_TYPELESS:
        case(DXGI_FORMAT_D32_FLOAT):
        case(DXGI_FORMAT_R32_FLOAT):
            return FFX_SURFACE_FORMAT_R32_FLOAT;

        case DXGI_FORMAT_R8G8_TYPELESS:
        case (DXGI_FORMAT_R8G8_UINT):
            return FFX_SURFACE_FORMAT_R8G8_UINT;
        //case DXGI_FORMAT_R8G8_UNORM:
        //case DXGI_FORMAT_R8G8_SNORM:
        //case DXGI_FORMAT_R8G8_SINT:

        case DXGI_FORMAT_R16_TYPELESS:
        case (DXGI_FORMAT_R16_FLOAT):
            return FFX_SURFACE_FORMAT_R16_FLOAT;
        case (DXGI_FORMAT_R16_UINT):
            return FFX_SURFACE_FORMAT_R16_UINT;
        case DXGI_FORMAT_D16_UNORM:
        case (DXGI_FORMAT_R16_UNORM):
            return FFX_SURFACE_FORMAT_R16_UNORM;
        case (DXGI_FORMAT_R16_SNORM):
            return FFX_SURFACE_FORMAT_R16_SNORM;
        //case DXGI_FORMAT_R16_SINT:

        case DXGI_FORMAT_R8_TYPELESS:
        case DXGI_FORMAT_R8_UNORM:
        case DXGI_FORMAT_A8_UNORM:
            return FFX_SURFACE_FORMAT_R8_UNORM;
        case DXGI_FORMAT_R8_UINT:
            return FFX_SURFACE_FORMAT_R8_UINT;
        //case DXGI_FORMAT_R8_SNORM:
        //case DXGI_FORMAT_R8_SINT:
        //case DXGI_FORMAT_R1_UNORM:

        case(DXGI_FORMAT_UNKNOWN):
            return FFX_SURFACE_FORMAT_UNKNOWN;
        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return FFX_SURFACE_FORMAT_UNKNOWN;
    }
}

bool IsDepthDX11(DXGI_FORMAT format)
{
    return (format == DXGI_FORMAT_D16_UNORM) || 
           (format == DXGI_FORMAT_D32_FLOAT) || 
           (format == DXGI_FORMAT_D24_UNORM_S8_UINT) ||
           (format == DXGI_FORMAT_D32_FLOAT_S8X24_UINT);
}

FfxResourceDescription GetFfxResourceDescriptionDX11(ID3D11Resource* pResource)
{
    FfxResourceDescription resourceDescription = {};

    // This is valid
    if (!pResource)
        return resourceDescription;

    if (pResource)
    {
        D3D11_RESOURCE_DIMENSION dimension = {};
        pResource->GetType(&dimension);
        
        if (dimension == D3D11_RESOURCE_DIMENSION_BUFFER)
        {
            D3D11_BUFFER_DESC desc = {};
            reinterpret_cast<ID3D11Buffer*>(pResource)->GetDesc(&desc);

            resourceDescription.flags  = FFX_RESOURCE_FLAGS_NONE;
            resourceDescription.usage  = FFX_RESOURCE_USAGE_UAV;
            resourceDescription.width  = desc.ByteWidth;
            resourceDescription.height = 1;
            resourceDescription.format = ffxGetSurfaceFormatDX11(DXGI_FORMAT_UNKNOWN);

            // What should we initialize this to?? No case for this yet
            resourceDescription.depth    = 0;
            resourceDescription.mipCount = 0;

            // Set the type
            resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
        }
        else
        {
            // Set flags properly for resource registration
            resourceDescription.flags = FFX_RESOURCE_FLAGS_NONE;

            switch (dimension)
            {
            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            {
                D3D11_TEXTURE1D_DESC desc = {};
                reinterpret_cast<ID3D11Texture1D*>(pResource)->GetDesc(&desc);

                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
                resourceDescription.usage = IsDepthDX11(desc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
                if ((desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == D3D11_BIND_UNORDERED_ACCESS)
                    resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

                resourceDescription.width = desc.Width;
                resourceDescription.height = 1;
                resourceDescription.depth = 1;
                resourceDescription.mipCount = desc.MipLevels;
                resourceDescription.format = ffxGetSurfaceFormatDX11(desc.Format);
                break;
            }
            case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            {
                D3D11_TEXTURE2D_DESC desc = {};
                reinterpret_cast<ID3D11Texture2D*>(pResource)->GetDesc(&desc);

                if (desc.ArraySize == 1)
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                else if (desc.ArraySize == 6)
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE_CUBE;
                else
                    resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
                resourceDescription.usage = IsDepthDX11(desc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
                if ((desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == D3D11_BIND_UNORDERED_ACCESS)
                    resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

                resourceDescription.width = desc.Width;
                resourceDescription.height = desc.Height;
                resourceDescription.depth = desc.ArraySize;
                resourceDescription.mipCount = desc.MipLevels;
                resourceDescription.format = ffxGetSurfaceFormatDX11(desc.Format);
                break;
            }
            case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            {
                D3D11_TEXTURE3D_DESC desc = {};
                reinterpret_cast<ID3D11Texture3D*>(pResource)->GetDesc(&desc);

                resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
                resourceDescription.usage = IsDepthDX11(desc.Format) ? FFX_RESOURCE_USAGE_DEPTHTARGET : FFX_RESOURCE_USAGE_READ_ONLY;
                if ((desc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) == D3D11_BIND_UNORDERED_ACCESS)
                    resourceDescription.usage = (FfxResourceUsage)(resourceDescription.usage | FFX_RESOURCE_USAGE_UAV);

                resourceDescription.width = desc.Width;
                resourceDescription.height = desc.Height;
                resourceDescription.depth = desc.Depth;
                resourceDescription.mipCount = desc.MipLevels;
                resourceDescription.format = ffxGetSurfaceFormatDX11(desc.Format);
                break;
            }
            default:
                FFX_ASSERT_MESSAGE(false, "FFXInterface: Cauldron: Unsupported texture dimension requested. Please implement.");
                break;
            }
        }
    }

    return resourceDescription;
}

ID3D11Resource* getDX11ResourcePtr(BackendContext_DX11* backendContext, int32_t resourceIndex)
{
    FFX_ASSERT(NULL != backendContext);
    return reinterpret_cast<ID3D11Resource*>(backendContext->pResources[resourceIndex].resourcePtr);
}

//////////////////////////////////////////////////////////////////////////
// DX11 back end implementation

FfxUInt32 GetSDKVersionDX11(FfxInterface* backendInterface)
{
    return FFX_SDK_MAKE_VERSION(FFX_SDK_VERSION_MAJOR, FFX_SDK_VERSION_MINOR, FFX_SDK_VERSION_PATCH);
}

uint64_t GetResourceGpuMemorySizeDX11(ID3D11Resource* resource)
{
    uint64_t      size = 0;

    return size;
}

FfxErrorCode GetEffectGpuMemoryUsageDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId, FfxEffectMemoryUsage* outVramUsage)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != outVramUsage);

    BackendContext_DX11*                backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    BackendContext_DX11::EffectContext& effectContext  = backendContext->pEffectContexts[effectContextId];

    *outVramUsage = effectContext.vramUsage;

    return FFX_OK;
}

// initialize the DX11 backend
FfxErrorCode CreateBackendContextDX11(FfxInterface* backendInterface, FfxEffect effect, FfxEffectBindlessConfig* bindlessConfig, FfxUInt32* effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != backendInterface->device);

    HRESULT result = S_OK;
    ID3D11Device* dx11Device = reinterpret_cast<ID3D11Device*>(backendInterface->device);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!backendContext->refCount) {

        new (&backendContext->constantBufferMutex) std::mutex();

        if (dx11Device != NULL) {

            dx11Device->AddRef();
            backendContext->device = dx11Device;

            dx11Device->GetImmediateContext(&backendContext->deviceContext);
            backendContext->deviceContext->QueryInterface(IID_PPV_ARGS(&backendContext->deviceContext1));
        }

        // Map all of our pointers
        uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX11::Resource), sizeof(uint64_t));
        uint32_t stagingRingBufferArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE, sizeof(uint32_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(backendContext->maxEffectContexts * sizeof(BackendContext_DX11::EffectContext), sizeof(uint32_t));

        uint8_t* pMem = (uint8_t*)((BackendContext_DX11*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resources
        backendContext->pResources = (BackendContext_DX11::Resource*)(pMem);
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Map the staging buffer
        backendContext->pStagingRingBuffer = (uint8_t*)(pMem);
        memset(backendContext->pStagingRingBuffer, 0, stagingRingBufferArraySize);
        pMem += stagingRingBufferArraySize;

        // Map the effect contexts
        backendContext->pEffectContexts = reinterpret_cast<BackendContext_DX11::EffectContext*>(pMem);
        memset(backendContext->pEffectContexts, 0, contextArraySize);
    }

    // Direct3D 11.1
    if (!backendContext->refCount && backendContext->deviceContext1 != NULL) {

        std::lock_guard<std::mutex> cbLock{ backendContext->constantBufferMutex };

        // create dynamic ring buffer for constant uploads
        backendContext->constantBufferSize[0] = FFX_ALIGN_UP(256/*FFX_MAX_CONST_SIZE*/, 256) *
            backendContext->maxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES; // Size aligned to 256

        D3D11_BUFFER_DESC constDesc = {};
        constDesc.ByteWidth = backendContext->constantBufferSize[0];
        constDesc.Usage = D3D11_USAGE_DYNAMIC;
        constDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        TIF(dx11Device->CreateBuffer(&constDesc, nullptr, &backendContext->constantBufferResource[0]));
        SetNameDX11(backendContext->constantBufferResource[0], L"FFX_DX11_DynamicRingBuffer");

        // map it
        D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
        TIF(backendContext->deviceContext->Map(backendContext->constantBufferResource[0], 0,
            D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSubresource));
        backendContext->constantBufferMem[0] = mappedSubresource.pData;
        backendContext->constantBufferOffset[0] = 0;
    }

    // Increment the ref count
    ++backendContext->refCount;

    // Get an available context id
    for (uint32_t i = 0; i < backendContext->maxEffectContexts; ++i) {
        if (!backendContext->pEffectContexts[i].active) {
            *effectContextId = i;

            // Reset everything accordingly
            BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[i];
            effectContext.active = true;
            effectContext.nextStaticResource = (i * FFX_MAX_RESOURCE_COUNT) + 1;
            effectContext.nextDynamicResource = (i * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;
            break;
        }
    }

    return FFX_OK;
}

// query device capabilities to select the optimal shader permutation
FfxErrorCode GetDeviceCapabilitiesDX11(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != backendInterface->device);
    FFX_ASSERT(NULL != deviceCapabilities);
    ID3D11Device* dx11Device = reinterpret_cast<ID3D11Device*>(backendInterface->device);

    // Check if we have shader model 6.6
    D3D_FEATURE_LEVEL shaderModel = dx11Device->GetFeatureLevel();
    switch (shaderModel) {

    case D3D_FEATURE_LEVEL_10_0:
    case D3D_FEATURE_LEVEL_10_1:
    case D3D_FEATURE_LEVEL_11_0:
    case D3D_FEATURE_LEVEL_11_1:
        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
        break;

    case D3D_FEATURE_LEVEL_12_0:
        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
        break;

    case D3D_FEATURE_LEVEL_12_1:
        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_3;
        break;

    case D3D_FEATURE_LEVEL_12_2:
        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_5;
        break;

    default:
        deviceCapabilities->maximumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
        break;
    }

    // check if we have 16bit floating point.
    D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT d3d11Options = {};
    if (SUCCEEDED(dx11Device->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &d3d11Options, sizeof(d3d11Options)))) {

        deviceCapabilities->fp16Supported = (d3d11Options.AllOtherShaderStagesMinPrecision != 0);
    }

    return FFX_OK;
}

// deinitialize the DX11 backend
FfxErrorCode DestroyBackendContextDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    FFX_ASSERT(backendContext->refCount > 0);

    // Delete any resources allocated by this context
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (uint32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex) {
        if (backendContext->pResources[currentStaticResourceIndex].resourcePtr) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: DX11: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { static_cast<int32_t>(currentStaticResourceIndex) };
            DestroyResourceDX11(backendInterface, internalResource, effectContextId);
        }
    }
    for (uint32_t currentResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentResourceIndex < effectContextId * FFX_MAX_RESOURCE_COUNT + FFX_MAX_RESOURCE_COUNT; ++currentResourceIndex) {
        if (backendContext->pResources[currentResourceIndex].resourcePtr) {
            FfxResourceInternal internalResource = { static_cast<int32_t>(currentResourceIndex) };
            DestroyResourceDX11(backendInterface, internalResource, effectContextId);
        }
    }

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --backendContext->refCount;

    if (!backendContext->refCount) {

        // release constant buffer pool
        for (size_t i = 0; i < FFX_MAX_NUM_CONST_BUFFERS; ++i) {

            if (backendContext->constantBufferResource[i] != NULL) {
                backendContext->deviceContext->Unmap(backendContext->constantBufferResource[i], 0);
                backendContext->constantBufferResource[i]->Release();
                backendContext->constantBufferResource[i] = NULL;
                backendContext->constantBufferMem[i] = nullptr;
                backendContext->constantBufferOffset[i] = 0;
                backendContext->constantBufferSize[i] = 0;
            }
        }
        backendContext->gpuJobCount = 0;

        if (backendContext->deviceContext1 != NULL) {
            backendContext->deviceContext1->Release();
            backendContext->deviceContext1 = NULL;
        }
        if (backendContext->deviceContext != NULL) {
            backendContext->deviceContext->Release();
            backendContext->deviceContext = NULL;
        }
        if (backendContext->device != NULL) {
            backendContext->device->Release();
            backendContext->device = NULL;
        }
    }

    return FFX_OK;
}

// create a internal resource that will stay alive until effect gets shut down
FfxErrorCode CreateResourceDX11(
    FfxInterface* backendInterface,
    const FfxCreateResourceDescription* createResourceDescription,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outTexture
)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != createResourceDescription);
    FFX_ASSERT(NULL != outTexture);
    FFX_ASSERT_MESSAGE(createResourceDescription->initData.type != FFX_RESOURCE_INIT_DATA_TYPE_INVALID,
                       "InitData type cannot be FFX_RESOURCE_INIT_DATA_TYPE_INVALID. Please explicitly specify the resource initialization type.");

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    ID3D11Device* dx11Device = backendContext->device;

    uint64_t resourceSize = 0;
    FFX_ASSERT(NULL != dx11Device);

    FFX_ASSERT(effectContext.nextStaticResource + 1 < effectContext.nextDynamicResource);

    outTexture->internalIndex = effectContext.nextStaticResource++;
    BackendContext_DX11::Resource* backendResource = &backendContext->pResources[outTexture->internalIndex];
    backendResource->resourceDescription = createResourceDescription->resourceDescription;

    D3D11_BUFFER_DESC dx11BufferDescription = {};
    D3D11_TEXTURE1D_DESC dx11Texture1DDescription = {};
    D3D11_TEXTURE2D_DESC dx11Texture2DDescription = {};
    D3D11_TEXTURE3D_DESC dx11Texture3DDescription = {};

    switch (createResourceDescription->resourceDescription.type) {

    case FFX_RESOURCE_TYPE_BUFFER:
        dx11BufferDescription.ByteWidth = createResourceDescription->resourceDescription.width;
        dx11BufferDescription.Usage = D3D11_USAGE_DEFAULT;
        dx11BufferDescription.BindFlags = ffxGetDX11BindFlags(backendResource->resourceDescription.usage);
        if (createResourceDescription->resourceDescription.format == FFX_SURFACE_FORMAT_UNKNOWN) {
            dx11BufferDescription.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            dx11BufferDescription.StructureByteStride = createResourceDescription->resourceDescription.stride;
        }
        break;

    case FFX_RESOURCE_TYPE_TEXTURE1D:
        dx11Texture1DDescription.Format = ffxGetDX11FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx11Texture1DDescription.Width = createResourceDescription->resourceDescription.width;
        dx11Texture1DDescription.ArraySize = createResourceDescription->resourceDescription.depth;
        dx11Texture1DDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
        dx11Texture1DDescription.Usage = D3D11_USAGE_DEFAULT;
        dx11Texture1DDescription.BindFlags = ffxGetDX11BindFlags(backendResource->resourceDescription.usage);
        break;

    case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
        dx11Texture2DDescription.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
    case FFX_RESOURCE_TYPE_TEXTURE2D:
        dx11Texture2DDescription.Format = ffxGetDX11FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx11Texture2DDescription.Width = createResourceDescription->resourceDescription.width;
        dx11Texture2DDescription.Height = createResourceDescription->resourceDescription.height;
        dx11Texture2DDescription.ArraySize = createResourceDescription->resourceDescription.depth;
        dx11Texture2DDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
        dx11Texture2DDescription.Usage = D3D11_USAGE_DEFAULT;
        dx11Texture2DDescription.BindFlags = ffxGetDX11BindFlags(backendResource->resourceDescription.usage);
        dx11Texture2DDescription.SampleDesc.Count = 1;
        break;

    case FFX_RESOURCE_TYPE_TEXTURE3D:
        dx11Texture3DDescription.Format = ffxGetDX11FormatFromSurfaceFormat(createResourceDescription->resourceDescription.format);
        dx11Texture3DDescription.Width = createResourceDescription->resourceDescription.width;
        dx11Texture3DDescription.Height = createResourceDescription->resourceDescription.height;
        dx11Texture3DDescription.Depth = createResourceDescription->resourceDescription.depth;
        dx11Texture3DDescription.MipLevels = createResourceDescription->resourceDescription.mipCount;
        dx11Texture3DDescription.Usage = D3D11_USAGE_DEFAULT;
        dx11Texture3DDescription.BindFlags = ffxGetDX11BindFlags(backendResource->resourceDescription.usage);
        break;

    default:
        break;
    }

    ID3D11Resource* dx11Resource = nullptr;
    if (createResourceDescription->heapType == FFX_HEAP_TYPE_UPLOAD) {

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif
        return FFX_OK;

    }
    else {

        D3D11_SUBRESOURCE_DATA dx11SubResourceData = {};
        D3D11_SUBRESOURCE_DATA* pSubResourceData = nullptr;
        if (createResourceDescription->initData.buffer) {
            pSubResourceData = &dx11SubResourceData;
            pSubResourceData->pSysMem = createResourceDescription->initData.buffer;
            pSubResourceData->SysMemPitch = static_cast<UINT>(createResourceDescription->initData.size);
            pSubResourceData->SysMemSlicePitch = static_cast<UINT>(createResourceDescription->initData.size);
        }

        switch (createResourceDescription->resourceDescription.type) {

        case FFX_RESOURCE_TYPE_BUFFER:
            TIF(dx11Device->CreateBuffer(&dx11BufferDescription, pSubResourceData, (ID3D11Buffer**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE1D:
            TIF(dx11Device->CreateTexture1D(&dx11Texture1DDescription, pSubResourceData, (ID3D11Texture1D**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            dx11SubResourceData.SysMemPitch /= dx11Texture2DDescription.Height;
            TIF(dx11Device->CreateTexture2D(&dx11Texture2DDescription, pSubResourceData, (ID3D11Texture2D**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE3D:
            dx11SubResourceData.SysMemPitch /= dx11Texture3DDescription.Height;
            dx11SubResourceData.SysMemPitch /= dx11Texture3DDescription.Depth;
            dx11SubResourceData.SysMemSlicePitch /= dx11Texture3DDescription.Depth;
            TIF(dx11Device->CreateTexture3D(&dx11Texture3DDescription, pSubResourceData, (ID3D11Texture3D**)&dx11Resource));
            break;

        default:
            break;
        }

        resourceSize = GetResourceGpuMemorySizeDX11(dx11Resource);

        SetNameDX11(dx11Resource, createResourceDescription->name);
        backendResource->resourcePtr = dx11Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif

        // Create SRVs and UAVs
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC dx11UavDescription = {};
            D3D11_SHADER_RESOURCE_VIEW_DESC dx11SrvDescription = {};

            // we still want to respect the format provided in the description for SRGB or TYPELESS resources
            DXGI_FORMAT descFormat = {};

            bool requestArrayView = FFX_CONTAINS_FLAG(backendResource->resourceDescription.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

            D3D11_RESOURCE_DIMENSION resourceDimension = D3D11_RESOURCE_DIMENSION(0);
            dx11Resource->GetType(&resourceDimension);

            D3D11_BUFFER_DESC dx11BufferDesc = {};
            D3D11_TEXTURE1D_DESC dx11Texture1DDesc = {};
            D3D11_TEXTURE2D_DESC dx11Texture2DDesc = {};
            D3D11_TEXTURE3D_DESC dx11Texture3DDesc = {};

            switch (resourceDimension) {

            case D3D11_RESOURCE_DIMENSION_BUFFER:
                reinterpret_cast<ID3D11Buffer*>(dx11Resource)->GetDesc(&dx11BufferDesc);
                descFormat = patchDxgiFormatWithFfxUsage(DXGI_FORMAT_UNKNOWN, backendResource->resourceDescription.format);
                dx11UavDescription.Format = convertFormatUav(descFormat);
                dx11SrvDescription.Format = convertFormatUav(descFormat);
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                reinterpret_cast<ID3D11Texture1D*>(dx11Resource)->GetDesc(&dx11Texture1DDescription);
                descFormat = patchDxgiFormatWithFfxUsage(dx11Texture1DDescription.Format, backendResource->resourceDescription.format);
                dx11UavDescription.Format = convertFormatUav(descFormat);
                dx11SrvDescription.Format = convertFormatUav(descFormat);
                if (dx11Texture1DDescription.ArraySize > 1 || requestArrayView)
                {
                    dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                    dx11UavDescription.Texture1DArray.ArraySize = dx11Texture1DDescription.ArraySize;
                    dx11UavDescription.Texture1DArray.FirstArraySlice = 0;
                    dx11UavDescription.Texture1DArray.MipSlice = 0;

                    dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                    dx11SrvDescription.Texture1DArray.ArraySize = dx11Texture1DDescription.ArraySize;
                    dx11SrvDescription.Texture1DArray.FirstArraySlice = 0;
                    dx11SrvDescription.Texture1DArray.MipLevels = dx11Texture1DDescription.MipLevels;
                    dx11SrvDescription.Texture1DArray.MostDetailedMip = 0;
                }
                else
                {
                    dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                    dx11UavDescription.Texture1D.MipSlice = 0;

                    dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                    dx11SrvDescription.Texture1D.MipLevels = dx11Texture1DDescription.MipLevels;
                    dx11SrvDescription.Texture1D.MostDetailedMip = 0;
                }
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
                reinterpret_cast<ID3D11Texture2D*>(dx11Resource)->GetDesc(&dx11Texture2DDescription);
                descFormat = patchDxgiFormatWithFfxUsage(dx11Texture2DDescription.Format, backendResource->resourceDescription.format);
                dx11UavDescription.Format = convertFormatUav(descFormat);
                dx11SrvDescription.Format = convertFormatUav(descFormat);
                if (dx11Texture2DDescription.ArraySize > 1 || requestArrayView)
                {
                    dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                    dx11UavDescription.Texture2DArray.ArraySize = dx11Texture2DDescription.ArraySize;
                    dx11UavDescription.Texture2DArray.FirstArraySlice = 0;
                    dx11UavDescription.Texture2DArray.MipSlice = 0;

                    dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                    dx11SrvDescription.Texture2DArray.ArraySize = dx11Texture2DDescription.ArraySize;
                    dx11SrvDescription.Texture2DArray.FirstArraySlice = 0;
                    dx11SrvDescription.Texture2DArray.MipLevels = dx11Texture2DDescription.MipLevels;
                    dx11SrvDescription.Texture2DArray.MostDetailedMip = 0;
                }
                else
                {
                    dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                    dx11UavDescription.Texture2D.MipSlice = 0;

                    dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    dx11SrvDescription.Texture2D.MipLevels = dx11Texture2DDescription.MipLevels;
                    dx11SrvDescription.Texture2D.MostDetailedMip = 0;
                }
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
                reinterpret_cast<ID3D11Texture3D*>(dx11Resource)->GetDesc(&dx11Texture3DDescription);
                descFormat = patchDxgiFormatWithFfxUsage(dx11Texture3DDescription.Format, backendResource->resourceDescription.format);
                dx11UavDescription.Format = convertFormatUav(descFormat);
                dx11SrvDescription.Format = convertFormatUav(descFormat);
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                dx11SrvDescription.Texture3D.MipLevels = dx11Texture3DDescription.MipLevels;
                dx11SrvDescription.Texture3D.MostDetailedMip = 0;
                break;

            default:
                break;
            }

            if (resourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER) {

                dx11SrvDescription.Buffer.FirstElement = 0;
                dx11SrvDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr[0]));

                // UAV
                if (dx11BufferDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                    dx11UavDescription.Buffer.FirstElement = 0;
                    dx11UavDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                    TIF(dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[0]));
                }
            }
            else {

                // CPU readable
                TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr[0]));

                // UAV
                if (dx11Texture1DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                    dx11Texture2DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                    dx11Texture3DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                    const int32_t mipLevels = FFX_MAXIMUM(FFX_MAXIMUM(dx11Texture1DDescription.MipLevels, dx11Texture2DDescription.MipLevels), dx11Texture3DDescription.MipLevels);

                    for (int32_t currentMipIndex = 0; currentMipIndex < mipLevels; ++currentMipIndex) {

                        dx11UavDescription.Texture2D.MipSlice = currentMipIndex;

                        dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[currentMipIndex]);
                    }
                }
            }
        }
    }

    effectContext.vramUsage.totalUsageInBytes += resourceSize;
    if ((createResourceDescription->resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
    {
        effectContext.vramUsage.aliasableUsageInBytes += resourceSize;
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceDX11(
    FfxInterface* backendInterface,
    FfxResourceInternal resource,
    FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    if ((resource.internalIndex >= int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT)) && (resource.internalIndex < int32_t(effectContextId * FFX_MAX_RESOURCE_COUNT + FFX_MAX_RESOURCE_COUNT))) {

        for (int32_t currentMipIndex = 0; currentMipIndex < 16; ++currentMipIndex) {
            if (backendContext->pResources[resource.internalIndex].srvPtr[currentMipIndex]) {
                backendContext->pResources[resource.internalIndex].srvPtr[currentMipIndex]->Release();
                backendContext->pResources[resource.internalIndex].srvPtr[currentMipIndex] = nullptr;
            }
            if (backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex]) {
                backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex]->Release();
                backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex] = nullptr;
            }
        }
        if (backendContext->pResources[resource.internalIndex].resourcePtr) {

            uint64_t resourceSize = GetResourceGpuMemorySizeDX11(backendContext->pResources[resource.internalIndex].resourcePtr);

            // update effect memory usage
            effectContext.vramUsage.totalUsageInBytes -= resourceSize;
            if ((backendContext->pResources[resource.internalIndex].resourceDescription.flags & FFX_RESOURCE_FLAGS_ALIASABLE) == FFX_RESOURCE_FLAGS_ALIASABLE)
            {
                effectContext.vramUsage.aliasableUsageInBytes -= resourceSize;
            }

            backendContext->pResources[resource.internalIndex].resourcePtr->Release();
            backendContext->pResources[resource.internalIndex].resourcePtr = nullptr;
        }

        return FFX_OK;
    }

    return FFX_ERROR_OUT_OF_RANGE;
}

FfxErrorCode RegisterResourceDX11(
    FfxInterface* backendInterface,
    const FfxResource* inFfxResource,
    FfxUInt32 effectContextId,
    FfxResourceInternal* outFfxResourceInternal
)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)(backendInterface->scratchBuffer);
    ID3D11Device* dx11Device = reinterpret_cast<ID3D11Device*>(backendContext->device);
    ID3D11Resource* dx11Resource = reinterpret_cast<ID3D11Resource*>(inFfxResource->resource);
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    FfxResourceStates state = inFfxResource->state;

    if (dx11Resource == nullptr) {

        outFfxResourceInternal->internalIndex = 0; // Always maps to FFX_<feature>_RESOURCE_IDENTIFIER_NULL;
        return FFX_OK;
    }

    FFX_ASSERT(effectContext.nextDynamicResource > effectContext.nextStaticResource);
    outFfxResourceInternal->internalIndex = effectContext.nextDynamicResource--;

    BackendContext_DX11::Resource* backendResource = &backendContext->pResources[outFfxResourceInternal->internalIndex];

    if (backendResource->resourcePtr == dx11Resource)
    {
        return FFX_OK;
    }

    DestroyResourceDX11(backendInterface, *outFfxResourceInternal, effectContextId);

    backendResource->resourcePtr = dx11Resource;
    if (backendResource->resourcePtr)
        backendResource->resourcePtr->AddRef();

#ifdef _DEBUG
    const wchar_t* name = inFfxResource->name;
    if (name) {
        wcscpy_s(backendResource->resourceName, name);
    }
#endif

    // create resource views
    if (dx11Resource) {

        D3D11_UNORDERED_ACCESS_VIEW_DESC dx11UavDescription = {};
        D3D11_SHADER_RESOURCE_VIEW_DESC dx11SrvDescription = {};

        // we still want to respect the format provided in the description for SRGB or TYPELESS resources
        DXGI_FORMAT descFormat = {};

        bool requestArrayView = FFX_CONTAINS_FLAG(inFfxResource->description.usage, FFX_RESOURCE_USAGE_ARRAYVIEW);

        D3D11_RESOURCE_DIMENSION resourceDimension = D3D11_RESOURCE_DIMENSION(0);
        dx11Resource->GetType(&resourceDimension);

        D3D11_BUFFER_DESC dx11BufferDesc = {};
        D3D11_TEXTURE1D_DESC dx11Texture1DDesc = {};
        D3D11_TEXTURE2D_DESC dx11Texture2DDesc = {};
        D3D11_TEXTURE3D_DESC dx11Texture3DDesc = {};

        switch (resourceDimension) {

        case D3D11_RESOURCE_DIMENSION_BUFFER:
            reinterpret_cast<ID3D11Buffer*>(dx11Resource)->GetDesc(&dx11BufferDesc);
            descFormat = patchDxgiFormatWithFfxUsage(DXGI_FORMAT_UNKNOWN, inFfxResource->description.format);
            dx11UavDescription.Format = convertFormatUav(descFormat);
            dx11SrvDescription.Format = convertFormatSrv(descFormat);
            dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
            backendResource->resourceDescription.size = inFfxResource->description.size;
            backendResource->resourceDescription.stride = inFfxResource->description.stride;
            backendResource->resourceDescription.alignment = 0;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            reinterpret_cast<ID3D11Texture1D*>(dx11Resource)->GetDesc(&dx11Texture1DDesc);
            descFormat = patchDxgiFormatWithFfxUsage(dx11Texture1DDesc.Format, inFfxResource->description.format);
            dx11UavDescription.Format = convertFormatUav(descFormat);
            dx11SrvDescription.Format = convertFormatSrv(descFormat);
            if (dx11Texture1DDesc.ArraySize > 1 || requestArrayView)
            {
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1DARRAY;
                dx11UavDescription.Texture1DArray.ArraySize = dx11Texture1DDesc.ArraySize;
                dx11UavDescription.Texture1DArray.FirstArraySlice = 0;
                dx11UavDescription.Texture1DArray.MipSlice = 0;

                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1DARRAY;
                dx11SrvDescription.Texture1DArray.ArraySize = dx11Texture1DDesc.ArraySize;
                dx11SrvDescription.Texture1DArray.FirstArraySlice = 0;
                dx11SrvDescription.Texture1DArray.MipLevels = dx11Texture1DDesc.MipLevels;
                dx11SrvDescription.Texture1DArray.MostDetailedMip = 0;
            }
            else
            {
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE1D;
                dx11UavDescription.Texture1D.MipSlice = 0;

                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
                dx11SrvDescription.Texture1D.MipLevels = dx11Texture1DDesc.MipLevels;
                dx11SrvDescription.Texture1D.MostDetailedMip = 0;
            }

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE1D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
            reinterpret_cast<ID3D11Texture2D*>(dx11Resource)->GetDesc(&dx11Texture2DDesc);
            descFormat = patchDxgiFormatWithFfxUsage(dx11Texture2DDesc.Format, inFfxResource->description.format);
            dx11UavDescription.Format = convertFormatUav(descFormat);
            dx11SrvDescription.Format = convertFormatSrv(descFormat);
            if (dx11Texture2DDesc.ArraySize > 1 || requestArrayView)
            {
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2DARRAY;
                dx11UavDescription.Texture2DArray.ArraySize = dx11Texture2DDesc.ArraySize;
                dx11UavDescription.Texture2DArray.FirstArraySlice = 0;
                dx11UavDescription.Texture2DArray.MipSlice = 0;

                dx11SrvDescription.ViewDimension = inFfxResource->description.type == FFX_RESOURCE_TYPE_TEXTURE_CUBE ? D3D11_SRV_DIMENSION_TEXTURECUBE : D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
                dx11SrvDescription.Texture2DArray.ArraySize = dx11Texture2DDesc.ArraySize;
                dx11SrvDescription.Texture2DArray.FirstArraySlice = 0;
                dx11SrvDescription.Texture2DArray.MipLevels = dx11Texture2DDesc.MipLevels;
                dx11SrvDescription.Texture2DArray.MostDetailedMip = 0;
            }
            else
            {
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
                dx11UavDescription.Texture2D.MipSlice = 0;

                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                dx11SrvDescription.Texture2D.MipLevels = dx11Texture2DDesc.MipLevels;
                dx11SrvDescription.Texture2D.MostDetailedMip = 0;
            }

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE2D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.height = inFfxResource->description.height;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
            reinterpret_cast<ID3D11Texture3D*>(dx11Resource)->GetDesc(&dx11Texture3DDesc);
            descFormat = patchDxgiFormatWithFfxUsage(dx11Texture3DDesc.Format, inFfxResource->description.format);
            dx11UavDescription.Format = convertFormatUav(descFormat);
            dx11SrvDescription.Format = convertFormatSrv(descFormat);
            dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
            dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
            dx11SrvDescription.Texture3D.MipLevels = dx11Texture3DDesc.MipLevels;
            dx11SrvDescription.Texture3D.MostDetailedMip = 0;

            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_TEXTURE3D;
            backendResource->resourceDescription.format = inFfxResource->description.format;
            backendResource->resourceDescription.width = inFfxResource->description.width;
            backendResource->resourceDescription.height = inFfxResource->description.height;
            backendResource->resourceDescription.mipCount = inFfxResource->description.mipCount;
            backendResource->resourceDescription.depth = inFfxResource->description.depth;
            break;

        default:
            break;
        }

        if (resourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER) {

            // UAV
            if (dx11BufferDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                dx11UavDescription.Buffer.FirstElement = 0;
                dx11UavDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                TIF(dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[0]));
            }

            dx11SrvDescription.Buffer.FirstElement = 0;
            dx11SrvDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

            TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr[0]));
        }
        else {

            // CPU readable
            TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr[0]));

            // UAV
            if (dx11Texture1DDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                dx11Texture2DDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                dx11Texture3DDesc.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                const int32_t uavDescriptorsCount = backendResource->resourceDescription.mipCount;

                for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorsCount; ++currentMipIndex) {

                    switch (resourceDimension)
                    {
                    case D3D11_RESOURCE_DIMENSION_BUFFER:
                        break;

                    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
                    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
                        // TextureXD<Y>.MipSlice values map to same mem
                        dx11UavDescription.Texture2D.MipSlice = currentMipIndex;
                        break;

                    default:
                        FFX_ASSERT_MESSAGE(false, "Invalid View Dimension");
                        break;
                    }

                    dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[currentMipIndex]);
                }
            }
        }
    }

    return FFX_OK;
}

FfxResource GetResourceDX11(FfxInterface* backendInterface, FfxResourceInternal inResource)
{
    FFX_ASSERT(nullptr != backendInterface);
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FfxResourceDescription ffxResDescription = backendInterface->fpGetResourceDescription(backendInterface, inResource);

    FfxResource resource = {};
    resource.resource = resource.resource = reinterpret_cast<void*>(backendContext->pResources[inResource.internalIndex].resourcePtr);
    resource.state = FFX_RESOURCE_STATE_COMMON;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (backendContext->pResources[inResource.internalIndex].resourceName)
    {
        wcscpy_s(resource.name, backendContext->pResources[inResource.internalIndex].resourceName);
    }
#endif

    return resource;
}


// dispose dynamic resources: This should be called at the end of the frame
FfxErrorCode UnregisterResourcesDX11(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX11* backendContext = (BackendContext_DX11*)(backendInterface->scratchBuffer);
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];

    // Walk back all the resources that don't belong to us and reset them to their initial state
    for (uint32_t resourceIndex = ++effectContext.nextDynamicResource; resourceIndex < (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT; ++resourceIndex)
    {
        FfxResourceInternal internalResource;
        internalResource.internalIndex = resourceIndex;
    }

    effectContext.nextDynamicResource      = (effectContextId * FFX_MAX_RESOURCE_COUNT) + FFX_MAX_RESOURCE_COUNT - 1;

    return FFX_OK;
}

FfxResourceDescription GetResourceDescriptorDX11(
    FfxInterface* backendInterface,
    FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FfxResourceDescription resourceDescription = backendContext->pResources[resource.internalIndex].resourceDescription;
    return resourceDescription;
}

FfxErrorCode StageConstantBufferDataDX11(FfxInterface* backendInterface, void* data, FfxUInt32 size, FfxConstantBuffer* constantBuffer)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    if (data && constantBuffer)
    {
        if ((backendContext->stagingRingBufferBase + FFX_ALIGN_UP(size, 256)) >= FFX_CONSTANT_BUFFER_RING_BUFFER_SIZE)
            backendContext->stagingRingBufferBase = 0;

        uint32_t* dstPtr = (uint32_t*)(backendContext->pStagingRingBuffer + backendContext->stagingRingBufferBase);

        memcpy(dstPtr, data, size);

        constantBuffer->data            = dstPtr;
        constantBuffer->num32BitEntries = size / sizeof(uint32_t);

        backendContext->stagingRingBufferBase += FFX_ALIGN_UP(size, 256);

        return FFX_OK;
    }
    else
        return FFX_ERROR_INVALID_POINTER;
}

D3D11_TEXTURE_ADDRESS_MODE FfxGetAddressModeDX11(const FfxAddressMode& addressMode)
{
    switch (addressMode)
    {
    case FFX_ADDRESS_MODE_WRAP:
        return D3D11_TEXTURE_ADDRESS_WRAP;
    case FFX_ADDRESS_MODE_MIRROR:
        return D3D11_TEXTURE_ADDRESS_MIRROR;
    case FFX_ADDRESS_MODE_CLAMP:
        return D3D11_TEXTURE_ADDRESS_CLAMP;
    case FFX_ADDRESS_MODE_BORDER:
        return D3D11_TEXTURE_ADDRESS_BORDER;
    case FFX_ADDRESS_MODE_MIRROR_ONCE:
        return D3D11_TEXTURE_ADDRESS_MIRROR_ONCE;
    default:
        FFX_ASSERT_MESSAGE(false, "Unsupported addressing mode requested. Please implement");
        return D3D11_TEXTURE_ADDRESS_WRAP;
        break;
    }
}

FfxErrorCode CreatePipelineDX11(
    FfxInterface* backendInterface,
    FfxEffect effect,
    FfxPass pass,
    uint32_t permutationOptions,
    const FfxPipelineDescription* pipelineDescription,
    FfxUInt32                     effectContextId,
    FfxPipelineState* outPipeline)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != pipelineDescription);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    ID3D11Device* dx11Device = backendContext->device;

    FfxShaderBlob shaderBlob = { };
    ffxGetPermutationBlobByIndex(effect, pass, FFX_BIND_COMPUTE_SHADER_STAGE, permutationOptions, &shaderBlob);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    int32_t staticTextureSrvCount = 0;
    int32_t staticBufferSrvCount = 0;
    int32_t staticTextureUavCount = 0;
    int32_t staticBufferUavCount = 0;

    int32_t staticTextureSrvSpace = -1;
    int32_t staticBufferSrvSpace = -1;
    int32_t staticTextureUavSpace = -1;
    int32_t staticBufferUavSpace = -1;

    // Only set the command signature if this is setup as an indirect workload
    outPipeline->cmdSignature = nullptr;

    uint32_t flattenedSrvTextureCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvTextureCount; ++srvIndex)
    {
        uint32_t slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        uint32_t spaceIndex = shaderBlob.boundSRVTextureSpaces[srvIndex];
        uint32_t bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticTextureSrvSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvTextureCount++;

            outPipeline->srvTextureBindings[bindingIndex].slotIndex = slotIndex;
            outPipeline->srvTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                0,
                shaderBlob.boundSRVTextureNames[srvIndex],
                -1,
                outPipeline->srvTextureBindings[bindingIndex].name,
                int(std::size(outPipeline->srvTextureBindings[bindingIndex].name)));
        }
    }

    outPipeline->srvTextureCount = flattenedSrvTextureCount;
    FFX_ASSERT(outPipeline->srvTextureCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavTextureCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavTextureCount; ++uavIndex)
    {
        uint32_t slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        uint32_t spaceIndex = shaderBlob.boundUAVTextureSpaces[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticTextureUavSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavTextureCount++;

            outPipeline->uavTextureBindings[bindingIndex].slotIndex = slotIndex;
            outPipeline->uavTextureBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                0,
                shaderBlob.boundUAVTextureNames[uavIndex],
                -1,
                outPipeline->uavTextureBindings[bindingIndex].name,
                int(std::size(outPipeline->uavTextureBindings[bindingIndex].name)));
        }
    }

    outPipeline->uavTextureCount = flattenedUavTextureCount;
    FFX_ASSERT(outPipeline->uavTextureCount < FFX_MAX_NUM_UAVS);

    uint32_t flattenedSrvBufferCount = 0;

    for (uint32_t srvIndex = 0; srvIndex < shaderBlob.srvBufferCount; ++srvIndex)
    {
        uint32_t slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        uint32_t spaceIndex = shaderBlob.boundSRVBufferSpaces[srvIndex];
        uint32_t bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticBufferSrvSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedSrvBufferCount++;

            outPipeline->srvBufferBindings[bindingIndex].slotIndex = slotIndex;
            outPipeline->srvBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                0,
                shaderBlob.boundSRVBufferNames[srvIndex],
                -1,
                outPipeline->srvBufferBindings[bindingIndex].name,
                int(std::size(outPipeline->srvBufferBindings[bindingIndex].name)));
        }
    }

    outPipeline->srvBufferCount = flattenedSrvBufferCount;
    FFX_ASSERT(outPipeline->srvBufferCount < FFX_MAX_NUM_SRVS);

    uint32_t flattenedUavBufferCount = 0;

    for (uint32_t uavIndex = 0; uavIndex < shaderBlob.uavBufferCount; ++uavIndex)
    {
        uint32_t slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        uint32_t spaceIndex = shaderBlob.boundUAVBufferSpaces[uavIndex];
        uint32_t bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];

        // Skip static resources
        if (spaceIndex == uint32_t(staticBufferUavSpace))
            continue;

        for (uint32_t arrayIndex = 0; arrayIndex < bindCount; arrayIndex++)
        {
            uint32_t bindingIndex = flattenedUavBufferCount++;

            outPipeline->uavBufferBindings[bindingIndex].slotIndex = slotIndex;
            outPipeline->uavBufferBindings[bindingIndex].arrayIndex = arrayIndex;
            MultiByteToWideChar(CP_UTF8,
                0,
                shaderBlob.boundUAVBufferNames[uavIndex],
                -1,
                outPipeline->uavBufferBindings[bindingIndex].name,
                int(std::size(outPipeline->uavBufferBindings[bindingIndex].name)));
        }
    }

    outPipeline->uavBufferCount = flattenedUavBufferCount;
    FFX_ASSERT(outPipeline->uavBufferCount < FFX_MAX_NUM_UAVS);

    for (uint32_t cbIndex = 0; cbIndex < shaderBlob.cbvCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].arrayIndex = 1;
        MultiByteToWideChar(CP_UTF8,
            0,
            shaderBlob.boundConstantBufferNames[cbIndex],
            -1,
            outPipeline->constantBufferBindings[cbIndex].name,
            int(std::size(outPipeline->constantBufferBindings[cbIndex].name)));
    }

    outPipeline->constCount = shaderBlob.cbvCount;
    FFX_ASSERT(outPipeline->constCount < FFX_MAX_NUM_CONST_BUFFERS);

    outPipeline->staticTextureSrvCount = staticTextureSrvCount;
    outPipeline->staticBufferSrvCount = staticBufferSrvCount;
    outPipeline->staticTextureUavCount = staticTextureUavCount;
    outPipeline->staticBufferUavCount = staticBufferUavCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    // patch GroupMemoryBarrier to GroupMemoryBarrierWithGroupSync
    DWORD* data = new DWORD[shaderBlob.size / sizeof(DWORD)];
    if (data == nullptr)
        return FFX_ERROR_INSUFFICIENT_MEMORY;
    memcpy(data, shaderBlob.data, shaderBlob.size);
    for (uint32_t i = 0; i < shaderBlob.size / sizeof(DWORD); ++i)
    {
        if (data[i] == MAKEFOURCC('S','H','E','X'))
        {
            DWORD* shex = &data[i];
            DWORD count = data[i + 1];
            bool hash = false;
            for (uint32_t i = 0; i < count / sizeof(DWORD); ++i)
            {
                if (shex[i] == 0x010010BE)
                {
                    shex[i] = 0x010018BE;
                    hash = true;
                }
            }
            if (hash)
            {
                CalculateDXBCChecksum(data, shaderBlob.size, &data[1]);
            }
            break;
        }
    }

    // create the PSO
    if (FAILED(dx11Device->CreateComputeShader(data, shaderBlob.size, nullptr, (ID3D11ComputeShader**)&outPipeline->pipeline)))
    {
        delete[] data;
        return FFX_ERROR_BACKEND_API_ERROR;
    }
    delete[] data;

    // Set the pipeline name
    SetNameDX11(reinterpret_cast<ID3D11ComputeShader*>(outPipeline->pipeline), pipelineDescription->name);

    return FFX_OK;
}

FfxErrorCode DestroyPipelineDX11(
    FfxInterface* backendInterface,
    FfxPipelineState* pipeline,
    FfxUInt32 effectContextId)
{
    FFX_ASSERT(backendInterface != nullptr);
    if (!pipeline) {
        return FFX_OK;
    }

    // destroy pipeline
    ID3D11ComputeShader* dx11Pipeline = reinterpret_cast<ID3D11ComputeShader*>(pipeline->pipeline);
    if (dx11Pipeline) {
        dx11Pipeline->Release();
    }
    pipeline->pipeline = nullptr;

    return FFX_OK;
}

FfxErrorCode ScheduleGpuJobDX11(
    FfxInterface* backendInterface,
    const FfxGpuJobDescription* job
)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != job);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FFX_ASSERT(backendContext->gpuJobCount < FFX_MAX_GPU_JOBS);

    backendContext->pGpuJobs[backendContext->gpuJobCount] = *job;
    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext* dx11DeviceContext)
{
    ID3D11UnorderedAccessView** uavs = backendContext->uavs;
    ID3D11ShaderResourceView** srvs = backendContext->srvs;
    memset(uavs, 0, sizeof(backendContext->uavs));
    memset(srvs, 0, sizeof(backendContext->srvs));

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    uint32_t minimumUav = UINT32_MAX;
    uint32_t maximumUav = 0;
    {
        // Set a baseline minimal value
        uint32_t maximumUavIndex = job->computeJobDescriptor.pipeline.uavTextureCount + job->computeJobDescriptor.pipeline.uavBufferCount;

        for (uint32_t uavTextureBinding = 0; uavTextureBinding < job->computeJobDescriptor.pipeline.uavTextureCount; uavTextureBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[uavTextureBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.uavTextureBindings[uavTextureBinding].arrayIndex;

            if (slotIndex > maximumUavIndex)
                maximumUavIndex = slotIndex;
        }

        for (uint32_t uavBufferBinding = 0; uavBufferBinding < job->computeJobDescriptor.pipeline.uavBufferCount; uavBufferBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[uavBufferBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.uavTextureBindings[uavBufferBinding].arrayIndex;

            if (slotIndex > maximumUavIndex)
                maximumUavIndex = slotIndex;
        }

        if (maximumUavIndex)
        {
            // Set Texture UAVs
            for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex) {

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex];

                // source: UAV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].resource.internalIndex;
                const uint32_t uavIndex = job->computeJobDescriptor.uavTextures[currentPipelineUavIndex].mip;
                ID3D11UnorderedAccessView* uavPtr = backendContext->pResources[resourceIndex].uavPtr[uavIndex];

                // where to bind it
                const uint32_t currentUavResourceIndex = binding.slotIndex + binding.arrayIndex;

                uavs[currentUavResourceIndex] = uavPtr;

                minimumUav = minimumUav < currentUavResourceIndex ? minimumUav : currentUavResourceIndex;
                maximumUav = maximumUav > currentUavResourceIndex ? maximumUav : currentUavResourceIndex;
            }

            // Set Buffer UAVs
            for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex) {

                // continue if this is a null resource.
                if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex == 0)
                    continue;

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex];

                // where to bind it
                const uint32_t currentUavResourceIndex = binding.slotIndex + binding.arrayIndex;

                if (job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].size > 0)
                {
                    bool     isStructured = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride > 0;
                    uint32_t stride       = isStructured ? job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride : sizeof(uint32_t);

                    // source: UAV of buffer to bind
                    const uint32_t resourceIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex;
                    const uint32_t uavIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].offset / stride;
                    ID3D11UnorderedAccessView* uavPtr = backendContext->pResources[resourceIndex].uavPtr[uavIndex];

                    if (uavPtr == nullptr)
                    {
                        // if size is non-zero create a dynamic descriptor directly on the GPU heap
                        ID3D11Resource* buffer = getDX11ResourcePtr(backendContext, job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].resource.internalIndex);
                        FFX_ASSERT(buffer != NULL);

                        D3D11_UNORDERED_ACCESS_VIEW_DESC dx11UavDescription = {};

                        bool     isStructured = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride > 0;
                        uint32_t stride       = isStructured ? job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].stride : sizeof(uint32_t);

                        dx11UavDescription.Format                      = isStructured ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS;
                        dx11UavDescription.ViewDimension               = D3D11_UAV_DIMENSION_BUFFER;
                        dx11UavDescription.Buffer.FirstElement         = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].offset / stride;
                        dx11UavDescription.Buffer.NumElements          = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].size / stride;
                        dx11UavDescription.Buffer.Flags                = isStructured ? 0 : D3D11_BUFFER_UAV_FLAG_RAW;

                        TIF(dx11Device->CreateUnorderedAccessView(buffer, &dx11UavDescription, &uavPtr));

                        backendContext->pResources[resourceIndex].uavPtr[uavIndex] = uavPtr;
                    }

                    uavs[currentUavResourceIndex] = uavPtr;

                    minimumUav = minimumUav < currentUavResourceIndex ? minimumUav : currentUavResourceIndex;
                    maximumUav = maximumUav > currentUavResourceIndex ? maximumUav : currentUavResourceIndex;
                }
                else
                {

                }
            }
        }
    }

    // bind texture & buffer SRVs
    uint32_t minimumSrv = UINT32_MAX;
    uint32_t maximumSrv = 0;
    {
        // Set a baseline minimal value
        uint32_t maximumSrvIndex = job->computeJobDescriptor.pipeline.srvTextureCount + job->computeJobDescriptor.pipeline.srvBufferCount;

        for (uint32_t srvTextureBinding = 0; srvTextureBinding < job->computeJobDescriptor.pipeline.srvTextureCount; srvTextureBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[srvTextureBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.srvTextureBindings[srvTextureBinding].arrayIndex;

            if (slotIndex > maximumSrvIndex)
                maximumSrvIndex = slotIndex;
        }

        for (uint32_t srvBufferBinding = 0; srvBufferBinding < job->computeJobDescriptor.pipeline.srvBufferCount; srvBufferBinding++)
        {
            uint32_t slotIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[srvBufferBinding].slotIndex +
                                 job->computeJobDescriptor.pipeline.srvTextureBindings[srvBufferBinding].arrayIndex;

            if (slotIndex > maximumSrvIndex)
                maximumSrvIndex = slotIndex;
        }

        if (maximumSrvIndex)
        {
            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
            {
                if (job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex == 0)
                    break;

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex];

                // source: SRV of resource to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.srvTextures[currentPipelineSrvIndex].resource.internalIndex;
                ID3D11ShaderResourceView* srvPtr = backendContext->pResources[resourceIndex].srvPtr[0];

                // Where to bind it
                uint32_t currentSrvResourceIndex = binding.slotIndex + binding.arrayIndex;

                srvs[currentSrvResourceIndex] = srvPtr;

                minimumSrv = minimumSrv < currentSrvResourceIndex ? minimumSrv : currentSrvResourceIndex;
                maximumSrv = maximumSrv > currentSrvResourceIndex ? maximumSrv : currentSrvResourceIndex;
            }

            // Set Buffer SRVs
            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
            {
                // continue if this is a null resource.
                if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex == 0)
                    continue;

                const FfxResourceBinding binding = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex];

                // where to bind it
                const uint32_t currentSrvResourceIndex = binding.slotIndex + binding.arrayIndex;

                if (job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].size > 0)
                {
                    bool     isStructured = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride > 0;
                    uint32_t stride       = isStructured ? job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride : sizeof(uint32_t);

                    // source: SRV of buffer to bind
                    const uint32_t resourceIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex;
                    const uint32_t srvIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].offset / stride;
                    ID3D11ShaderResourceView* srvPtr = backendContext->pResources[resourceIndex].srvPtr[srvIndex];

                    if (srvPtr == nullptr)
                    {
                        // if size is non-zero create a dynamic descriptor directly on the GPU heap
                        ID3D11Resource* buffer = getDX11ResourcePtr(backendContext, job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].resource.internalIndex);
                        FFX_ASSERT(buffer != NULL);

                        D3D11_SHADER_RESOURCE_VIEW_DESC dx11SrvDescription = {};

                        bool     isStructured = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride > 0;
                        uint32_t stride       = isStructured ? job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].stride : sizeof(uint32_t);

                        dx11SrvDescription.Format                     = isStructured ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R32_TYPELESS;
                        dx11SrvDescription.ViewDimension              = D3D11_SRV_DIMENSION_BUFFEREX;
                        dx11SrvDescription.BufferEx.FirstElement      = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].offset / stride;
                        dx11SrvDescription.BufferEx.NumElements       = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].size / stride;
                        dx11SrvDescription.BufferEx.Flags             = isStructured ? 0 : D3D11_BUFFEREX_SRV_FLAG_RAW;

                        TIF(dx11Device->CreateShaderResourceView(buffer, &dx11SrvDescription, &srvPtr));

                        backendContext->pResources[resourceIndex].srvPtr[srvIndex] = srvPtr;
                    }

                    srvs[currentSrvResourceIndex] = srvPtr;

                    minimumSrv = minimumSrv < currentSrvResourceIndex ? minimumSrv : currentSrvResourceIndex;
                    maximumSrv = maximumSrv > currentSrvResourceIndex ? maximumSrv : currentSrvResourceIndex;
                }
                else
                {

                }
            }
        }
    }

    // bind pipeline
    ID3D11ComputeShader* dx11ComputeShader = reinterpret_cast<ID3D11ComputeShader*>(job->computeJobDescriptor.pipeline.pipeline);
    dx11DeviceContext->CSSetShader(dx11ComputeShader, nullptr, 0);

    // copy data to constant buffer and bind
    {
        std::lock_guard<std::mutex> cbLock{ backendContext->constantBufferMutex };
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex) {

            uint32_t size = FFX_ALIGN_UP(job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t), 256);

            // Direct3D 11.0
            if (backendContext->deviceContext1 == NULL) {

                if (backendContext->constantBufferSize[currentRootConstantIndex] < size) {

                    if (backendContext->constantBufferResource[currentRootConstantIndex] != NULL) {
                        backendContext->constantBufferResource[currentRootConstantIndex]->Release();
                        backendContext->constantBufferResource[currentRootConstantIndex] = NULL;
                    }
                    backendContext->constantBufferSize[currentRootConstantIndex] = size;

                    D3D11_BUFFER_DESC constDesc = {};
                    constDesc.ByteWidth = size;
                    constDesc.Usage = D3D11_USAGE_DYNAMIC;
                    constDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
                    constDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
                    TIF(dx11Device->CreateBuffer(&constDesc, nullptr, &backendContext->constantBufferResource[currentRootConstantIndex]));
                    SetNameDX11(backendContext->constantBufferResource[currentRootConstantIndex], L"FFX_DX11_ConstantBuffer");
                }

                if (backendContext->constantBufferResource[currentRootConstantIndex] != NULL) {

                    D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
                    TIF(backendContext->deviceContext->Map(backendContext->constantBufferResource[currentRootConstantIndex], 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource));

                    if (mappedSubresource.pData) {
                        memcpy(mappedSubresource.pData, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));
                        backendContext->deviceContext->Unmap(backendContext->constantBufferResource[currentRootConstantIndex], 0);
                    }

                    dx11DeviceContext->CSSetConstantBuffers(currentRootConstantIndex, 1, &backendContext->constantBufferResource[currentRootConstantIndex]);
                }

                continue;
            }

            if (backendContext->constantBufferOffset[0] + size >= backendContext->constantBufferSize[0])
                backendContext->constantBufferOffset[0] = 0;

            void* pBuffer = (void*)((uint8_t*)(backendContext->constantBufferMem[0]) + backendContext->constantBufferOffset[0]);
            memcpy(pBuffer, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));

            uint32_t first = backendContext->constantBufferOffset[0] / sizeof(FfxFloat32x4);
            uint32_t num = size / sizeof(FfxFloat32x4);

            backendContext->deviceContext1->CSSetConstantBuffers1(currentRootConstantIndex, 1, &backendContext->constantBufferResource[0], &first, &num);

            // update the offset
            backendContext->constantBufferOffset[0] += size;
        }
    }

    // bind UAVs
    if (minimumUav <= maximumUav) {

        uint32_t count = maximumUav - minimumUav + 1;
        dx11DeviceContext->CSSetUnorderedAccessViews(minimumUav, count, uavs, nullptr);
    }

    // bind SRVs
    if (minimumSrv <= maximumSrv) {

        uint32_t count = maximumSrv - minimumSrv + 1;
        dx11DeviceContext->CSSetShaderResources(minimumSrv, count, srvs);
    }

    // dispatch
    dx11DeviceContext->Dispatch(job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);

    // unbind UAVs
    static ID3D11UnorderedAccessView* const emptyUAVs[D3D11_1_UAV_SLOT_COUNT] = {};
    if (minimumUav <= maximumUav) {

        uint32_t count = maximumUav - minimumUav + 1;
        dx11DeviceContext->CSSetUnorderedAccessViews(minimumUav, count, emptyUAVs, nullptr);
    }

    // unbind SRVs
    static ID3D11ShaderResourceView* const emptySRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT] = {};
    if (minimumSrv <= maximumSrv) {

        uint32_t count = maximumSrv - minimumSrv + 1;
        dx11DeviceContext->CSSetShaderResources(minimumSrv, count, emptySRVs);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext* dx11DeviceContext)
{
    ID3D11Resource* dx11ResourceSrc = getDX11ResourcePtr(backendContext, job->copyJobDescriptor.src.internalIndex);
    ID3D11Resource* dx11ResourceDst = getDX11ResourcePtr(backendContext, job->copyJobDescriptor.dst.internalIndex);

    dx11DeviceContext->CopyResource(dx11ResourceDst, dx11ResourceSrc);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext* dx11DeviceContext)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_DX11::Resource ffxResource = backendContext->pResources[idx];

    uint32_t clearColorAsUint[4];
    clearColorAsUint[0] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[0]);
    clearColorAsUint[1] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[1]);
    clearColorAsUint[2] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[2]);
    clearColorAsUint[3] = reinterpret_cast<uint32_t&> (job->clearJobDescriptor.color[3]);
    dx11DeviceContext->ClearUnorderedAccessViewUint(ffxResource.uavPtr[0], clearColorAsUint);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobDiscard(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext* dx11DeviceContext)
{
    uint32_t                            idx = job->discardJobDescriptor.target.internalIndex;
    BackendContext_DX11::Resource       ffxResource = backendContext->pResources[idx];
    ID3D11Resource* dx11Resource = reinterpret_cast<ID3D11Resource*>(ffxResource.resourcePtr);

    if (backendContext->deviceContext1)
        backendContext->deviceContext1->DiscardResource(dx11Resource);

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsDX11(
    FfxInterface* backendInterface,
    FfxCommandList commandList,
    FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FfxErrorCode errorCode = FFX_OK;

    // execute all GpuJobs
    for (uint32_t currentGpuJobIndex = 0; currentGpuJobIndex < backendContext->gpuJobCount; ++currentGpuJobIndex) {

        FfxGpuJobDescription* GpuJob = &backendContext->pGpuJobs[currentGpuJobIndex];
        ID3D11Device* dx11Device = backendContext->device;
        ID3D11DeviceContext* dx11DeviceContext = backendContext->deviceContext;

        switch (GpuJob->jobType) {

            case FFX_GPU_JOB_CLEAR_FLOAT:
                errorCode = executeGpuJobClearFloat(backendContext, GpuJob, dx11Device, dx11DeviceContext);
                break;

            case FFX_GPU_JOB_COPY:
                errorCode = executeGpuJobCopy(backendContext, GpuJob, dx11Device, dx11DeviceContext);
                break;

            case FFX_GPU_JOB_COMPUTE:
                errorCode = executeGpuJobCompute(backendContext, GpuJob, dx11Device, dx11DeviceContext);
                break;

            case FFX_GPU_JOB_BARRIER:
                break;

            case FFX_GPU_JOB_DISCARD:
                errorCode = executeGpuJobDiscard(backendContext, GpuJob, dx11Device, dx11DeviceContext);
                break;

            default:
                break;
        }
    }

    // check the execute function returned cleanly.
    FFX_RETURN_ON_ERROR(
        errorCode == FFX_OK,
        FFX_ERROR_BACKEND_API_ERROR);

    backendContext->gpuJobCount = 0;

    return FFX_OK;
}
