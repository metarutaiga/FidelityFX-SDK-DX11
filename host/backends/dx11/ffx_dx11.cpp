// This file is part of the FidelityFX SDK.
//
// Copyright © 2023 Advanced Micro Devices, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <host/ffx_interface.h>
#include <host/ffx_util.h>
#include <host/ffx_assert.h>
#include <host/backends/dx11/ffx_dx11.h>
#include <host/backends/ffx_shader_blobs.h>
#include <codecvt>  // convert string to wstring

// DX11 prototypes for functions in the backend interface
FfxUInt32 GetSDKVersionDX11(FfxInterface* backendInterface);
FfxErrorCode CreateBackendContextDX11(FfxInterface* backendInterface, FfxUInt32* effectContextId);
FfxErrorCode GetDeviceCapabilitiesDX11(FfxInterface* backendInterface, FfxDeviceCapabilities* deviceCapabilities);
FfxErrorCode DestroyBackendContextDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId);
FfxErrorCode CreateResourceDX11(FfxInterface* backendInterface, const FfxCreateResourceDescription* desc, FfxUInt32 effectContextId, FfxResourceInternal* outTexture);
FfxErrorCode DestroyResourceDX11(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode RegisterResourceDX11(FfxInterface* backendInterface, const FfxResource* inResource, FfxUInt32 effectContextId, FfxResourceInternal* outResourceInternal);
FfxResource GetResourceDX11(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode UnregisterResourcesDX11(FfxInterface* backendInterface, FfxCommandList commandList, FfxUInt32 effectContextId);
FfxResourceDescription GetResourceDescriptorDX11(FfxInterface* backendInterface, FfxResourceInternal resource);
FfxErrorCode CreatePipelineDX11(FfxInterface* backendInterface, FfxEffect effect, FfxPass passId, uint32_t permutationOptions, const FfxPipelineDescription*  desc, FfxUInt32 effectContextId, FfxPipelineState* outPass);
FfxErrorCode DestroyPipelineDX11(FfxInterface* backendInterface, FfxPipelineState* pipeline, FfxUInt32 effectContextId);
FfxErrorCode ScheduleGpuJobDX11(FfxInterface* backendInterface, const FfxGpuJobDescription* job);
FfxErrorCode ExecuteGpuJobsDX11(FfxInterface* backendInterface, FfxCommandList commandList);

#define FFX_MAX_RESOURCE_IDENTIFIER_COUNT   (128)

// To track parallel effect context usage
static uint32_t s_BackendRefCount = 0;
static size_t   s_MaxEffectContexts = 0;

typedef struct BackendContext_DX11 {

    // store for resources and resourceViews
    typedef struct Resource
    {
#ifdef _DEBUG
        wchar_t                     resourceName[64] = {};
#endif
        ID3D11Resource*             resourcePtr;
        FfxResourceDescription      resourceDescription;
        ID3D11ShaderResourceView*   srvPtr;
        ID3D11UnorderedAccessView*  uavPtr[16];
    } Resource;

    ID3D11Device*           device = nullptr;
    ID3D11DeviceContext1*   deviceContext = nullptr;

    FfxGpuJobDescription*   pGpuJobs;
    uint32_t                gpuJobCount;

    void*                   constantBufferMem;
    ID3D11Buffer*           constantBufferResource;
    uint32_t                constantBufferSize;
    uint32_t                constantBufferOffset;

    typedef struct alignas(32) EffectContext {

        // Resource allocation
        uint32_t            nextStaticResource;
        uint32_t            nextDynamicResource;

        // Usage
        bool                active;

    } EffectContext;

    // Resource holder
    Resource*               pResources;
    EffectContext*          pEffectContexts;

} BackendContext_DX11;

FFX_API size_t ffxGetScratchMemorySizeDX11(size_t maxContexts)
{
    uint32_t resourceArraySize   = FFX_ALIGN_UP(maxContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX11::Resource), sizeof(uint64_t));
    uint32_t contextArraySize    = FFX_ALIGN_UP(maxContexts * sizeof(BackendContext_DX11::EffectContext), sizeof(uint32_t));
    uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(maxContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));

    return FFX_ALIGN_UP(sizeof(BackendContext_DX11) + resourceArraySize + contextArraySize + gpuJobDescArraySize, sizeof(uint64_t));
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
    size_t maxContexts) {

    FFX_RETURN_ON_ERROR(
        !s_BackendRefCount,
        FFX_ERROR_BACKEND_API_ERROR);
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
    backendInterface->fpCreateBackendContext = CreateBackendContextDX11;
    backendInterface->fpGetDeviceCapabilities = GetDeviceCapabilitiesDX11;
    backendInterface->fpDestroyBackendContext = DestroyBackendContextDX11;
    backendInterface->fpCreateResource = CreateResourceDX11;
    backendInterface->fpDestroyResource = DestroyResourceDX11;
    backendInterface->fpGetResource = GetResourceDX11;
    backendInterface->fpRegisterResource = RegisterResourceDX11;
    backendInterface->fpUnregisterResources = UnregisterResourcesDX11;
    backendInterface->fpGetResourceDescription = GetResourceDescriptorDX11;
    backendInterface->fpCreatePipeline = CreatePipelineDX11;
    backendInterface->fpDestroyPipeline = DestroyPipelineDX11;
    backendInterface->fpScheduleGpuJob = ScheduleGpuJobDX11;
    backendInterface->fpExecuteGpuJobs = ExecuteGpuJobsDX11;

    // Memory assignments
    backendInterface->scratchBuffer = scratchBuffer;
    backendInterface->scratchBufferSize = scratchBufferSize;

    // Set the device
    backendInterface->device = device;

    // Assign the max number of contexts we'll be using
    s_MaxEffectContexts = maxContexts;

    return FFX_OK;
}

FfxCommandList ffxGetCommandListDX11(ID3D11DeviceContext1* deviceContext)
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
    resource.resource = reinterpret_cast<void*>(dx11Resource);
    resource.state = state;
    resource.description = ffxResDescription;

#ifdef _DEBUG
    if (ffxResName) {
        wcscpy_s(resource.name, ffxResName);
    }
#endif

    return resource;
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

        // Colors can map as is
    default:
        return format;
    }
}

DXGI_FORMAT ffxGetDX11FormatFromSurfaceFormat(FfxSurfaceFormat surfaceFormat)
{
    switch (surfaceFormat) {

        case(FFX_SURFACE_FORMAT_R32G32B32A32_TYPELESS):
            return DXGI_FORMAT_R32G32B32A32_TYPELESS;
        case(FFX_SURFACE_FORMAT_R32G32B32A32_FLOAT):
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT):
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case(FFX_SURFACE_FORMAT_R32G32_FLOAT):
            return DXGI_FORMAT_R32G32_FLOAT;
        case(FFX_SURFACE_FORMAT_R32_UINT):
            return DXGI_FORMAT_R32_UINT;
        case(FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS):
            return DXGI_FORMAT_R8G8B8A8_TYPELESS;
        case(FFX_SURFACE_FORMAT_R8G8B8A8_UNORM):
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case(FFX_SURFACE_FORMAT_R8G8B8A8_SNORM):
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case(FFX_SURFACE_FORMAT_R11G11B10_FLOAT):
            return DXGI_FORMAT_R11G11B10_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16_FLOAT):
            return DXGI_FORMAT_R16G16_FLOAT;
        case(FFX_SURFACE_FORMAT_R16G16_UINT):
            return DXGI_FORMAT_R16G16_UINT;
        case(FFX_SURFACE_FORMAT_R16_FLOAT):
            return DXGI_FORMAT_R16_FLOAT;
        case(FFX_SURFACE_FORMAT_R16_UINT):
            return DXGI_FORMAT_R16_UINT;
        case(FFX_SURFACE_FORMAT_R16_UNORM):
            return DXGI_FORMAT_R16_UNORM;
        case(FFX_SURFACE_FORMAT_R16_SNORM):
            return DXGI_FORMAT_R16_SNORM;
        case(FFX_SURFACE_FORMAT_R8_UNORM):
            return DXGI_FORMAT_R8_UNORM;
        case(FFX_SURFACE_FORMAT_R8G8_UNORM):
            return DXGI_FORMAT_R8G8_UNORM;
        case(FFX_SURFACE_FORMAT_R32_FLOAT):
            return DXGI_FORMAT_R32_FLOAT;
        case(FFX_SURFACE_FORMAT_UNKNOWN):
            return DXGI_FORMAT_UNKNOWN;

        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return DXGI_FORMAT_UNKNOWN;
    }
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
        case(DXGI_FORMAT_R16G16B16A16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16B16A16_FLOAT;
        case(DXGI_FORMAT_R32G32_FLOAT):
            return FFX_SURFACE_FORMAT_R32G32_FLOAT;
        case(DXGI_FORMAT_R32_UINT):
            return FFX_SURFACE_FORMAT_R32_UINT;
        case(DXGI_FORMAT_D32_FLOAT):
        case(DXGI_FORMAT_R32_FLOAT):
            return FFX_SURFACE_FORMAT_R32_FLOAT;
        case(DXGI_FORMAT_R8G8B8A8_TYPELESS):
            return FFX_SURFACE_FORMAT_R8G8B8A8_TYPELESS;
        case(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB):
            return FFX_SURFACE_FORMAT_R8G8B8A8_SRGB;
        case(DXGI_FORMAT_R8G8B8A8_UNORM):
            return FFX_SURFACE_FORMAT_R8G8B8A8_UNORM;
        case(DXGI_FORMAT_R11G11B10_FLOAT):
            return FFX_SURFACE_FORMAT_R11G11B10_FLOAT;
        case(DXGI_FORMAT_R16G16_FLOAT):
            return FFX_SURFACE_FORMAT_R16G16_FLOAT;
        case(DXGI_FORMAT_R16G16_UINT):
            return FFX_SURFACE_FORMAT_R16G16_UINT;
        case(DXGI_FORMAT_R16_FLOAT):
            return FFX_SURFACE_FORMAT_R16_FLOAT;
        case(DXGI_FORMAT_R16_UINT):
            return FFX_SURFACE_FORMAT_R16_UINT;
        case(DXGI_FORMAT_R16_UNORM):
            return FFX_SURFACE_FORMAT_R16_UNORM;
        case(DXGI_FORMAT_R16_SNORM):
            return FFX_SURFACE_FORMAT_R16_SNORM;
        case(DXGI_FORMAT_R8_UNORM):
            return FFX_SURFACE_FORMAT_R8_UNORM;
        case(DXGI_FORMAT_R8_UINT):
            return FFX_SURFACE_FORMAT_R8_UINT;
        case(DXGI_FORMAT_UNKNOWN):
            return FFX_SURFACE_FORMAT_UNKNOWN;
        default:
            FFX_ASSERT_MESSAGE(false, "Format not yet supported");
            return FFX_SURFACE_FORMAT_UNKNOWN;
    }
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

// initialize the DX11 backend
FfxErrorCode CreateBackendContextDX11(FfxInterface* backendInterface, FfxUInt32* effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    FFX_ASSERT(NULL != backendInterface->device);

    HRESULT result = S_OK;
    ID3D11Device* dx11Device = reinterpret_cast<ID3D11Device*>(backendInterface->device);

    // set up some internal resources we need (space for resource views and constant buffers)
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    // Set things up if this is the first invocation
    if (!s_BackendRefCount) {

        // Clear everything out
        memset(backendContext, 0, sizeof(*backendContext));

        if (dx11Device != NULL) {

            dx11Device->AddRef();
            backendContext->device = dx11Device;

            ID3D11DeviceContext* dx11DeviceContext = nullptr;
            dx11Device->GetImmediateContext(&dx11DeviceContext);
            dx11DeviceContext->QueryInterface(IID_PPV_ARGS(&backendContext->deviceContext));
            dx11DeviceContext->Release();

            if (backendContext->deviceContext == nullptr)
                return FFX_ERROR_NULL_DEVICE;
        }

        // Map all of our pointers
        uint32_t gpuJobDescArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_GPU_JOBS * sizeof(FfxGpuJobDescription), sizeof(uint32_t));
        uint32_t resourceArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * FFX_MAX_RESOURCE_COUNT * sizeof(BackendContext_DX11::Resource), sizeof(uint64_t));
        uint32_t contextArraySize = FFX_ALIGN_UP(s_MaxEffectContexts * sizeof(BackendContext_DX11::EffectContext), sizeof(uint32_t));
        uint8_t* pMem = (uint8_t*)((BackendContext_DX11*)(backendContext + 1));

        // Map gpu job array
        backendContext->pGpuJobs = (FfxGpuJobDescription*)pMem;
        memset(backendContext->pGpuJobs, 0, gpuJobDescArraySize);
        pMem += gpuJobDescArraySize;

        // Map the resources
        backendContext->pResources = (BackendContext_DX11::Resource*)(pMem);
        memset(backendContext->pResources, 0, resourceArraySize);
        pMem += resourceArraySize;

        // Map the effect contexts
        backendContext->pEffectContexts = reinterpret_cast<BackendContext_DX11::EffectContext*>(pMem);
        memset(backendContext->pEffectContexts, 0, contextArraySize);

        // create dynamic ring buffer for constant uploads
        backendContext->constantBufferSize = FFX_ALIGN_UP(FFX_MAX_CONST_SIZE, 256) *
            s_MaxEffectContexts * FFX_MAX_PASS_COUNT * FFX_MAX_QUEUED_FRAMES; // Size aligned to 256

        D3D11_BUFFER_DESC constDesc = {};
        constDesc.ByteWidth = backendContext->constantBufferSize;
        constDesc.Usage = D3D11_USAGE_DYNAMIC;
        constDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        TIF(dx11Device->CreateBuffer(&constDesc, nullptr, &backendContext->constantBufferResource));
//      backendContext->constantBufferResource->SetName(L"FFX_DX11_DynamicRingBuffer");

        // map it
        D3D11_MAPPED_SUBRESOURCE mappedSubresource = {};
        TIF(backendContext->deviceContext->Map(backendContext->constantBufferResource, 0, 
            D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mappedSubresource));
        backendContext->constantBufferMem = mappedSubresource.pData;
        backendContext->constantBufferOffset = 0;
    }

    // Increment the ref count
    ++s_BackendRefCount;

    // Get an available context id
    for (size_t i = 0; i < s_MaxEffectContexts; ++i) {
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
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_5_1;
        break;

    case D3D_FEATURE_LEVEL_12_0:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_0;
        break;

    case D3D_FEATURE_LEVEL_12_1:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_3;
        break;

    case D3D_FEATURE_LEVEL_12_2:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_5;
        break;

    default:
        deviceCapabilities->minimumSupportedShaderModel = FFX_SHADER_MODEL_6_6;
        break;
    }

    // check if we have 16bit floating point.
    D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT d3d11Options = {};
    if (SUCCEEDED(dx11Device->CheckFeatureSupport(D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT, &d3d11Options, sizeof(d3d11Options)))) {

        deviceCapabilities->fp16Supported = (d3d11Options.PixelShaderMinPrecision != 0);
    }

    return FFX_OK;
}

// deinitialize the DX11 backend
FfxErrorCode DestroyBackendContextDX11(FfxInterface* backendInterface, FfxUInt32 effectContextId)
{
    FFX_ASSERT(NULL != backendInterface);
    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    // Delete any resources allocated by this context
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    for (int32_t currentStaticResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentStaticResourceIndex < effectContext.nextStaticResource; ++currentStaticResourceIndex) {
        if (backendContext->pResources[currentStaticResourceIndex].resourcePtr) {
            FFX_ASSERT_MESSAGE(false, "FFXInterface: DX11: SDK Resource was not destroyed prior to destroying the backend context. There is a resource leak.");
            FfxResourceInternal internalResource = { currentStaticResourceIndex };
            DestroyResourceDX11(backendInterface, internalResource);
        }
    }
    for (int32_t currentResourceIndex = effectContextId * FFX_MAX_RESOURCE_COUNT; currentResourceIndex < effectContextId * FFX_MAX_RESOURCE_COUNT + FFX_MAX_RESOURCE_COUNT; ++currentResourceIndex) {
        if (backendContext->pResources[currentResourceIndex].resourcePtr) {
            FfxResourceInternal internalResource = { currentResourceIndex };
            DestroyResourceDX11(backendInterface, internalResource);
        }
    }

    // Free up for use by another context
    effectContext.nextStaticResource = 0;
    effectContext.active = false;

    // Decrement ref count
    --s_BackendRefCount;

    if (!s_BackendRefCount) {

        // release constant buffer pool
        backendContext->deviceContext->Unmap(backendContext->constantBufferResource, 0);
        backendContext->constantBufferResource->Release();
        backendContext->constantBufferMem = nullptr;
        backendContext->constantBufferOffset = 0;
        backendContext->constantBufferSize = 0;

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

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;
    BackendContext_DX11::EffectContext& effectContext = backendContext->pEffectContexts[effectContextId];
    ID3D11Device* dx11Device = backendContext->device;

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

        return FFX_OK;

    }
    else {

        switch (createResourceDescription->resourceDescription.type) {

        case FFX_RESOURCE_TYPE_BUFFER:
            TIF(dx11Device->CreateBuffer(&dx11BufferDescription, nullptr, (ID3D11Buffer**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE1D:
            TIF(dx11Device->CreateTexture1D(&dx11Texture1DDescription, nullptr, (ID3D11Texture1D**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE_CUBE:
        case FFX_RESOURCE_TYPE_TEXTURE2D:
            TIF(dx11Device->CreateTexture2D(&dx11Texture2DDescription, nullptr, (ID3D11Texture2D**)&dx11Resource));
            break;

        case FFX_RESOURCE_TYPE_TEXTURE3D:
            TIF(dx11Device->CreateTexture3D(&dx11Texture3DDescription, nullptr, (ID3D11Texture3D**)&dx11Resource));
            break;

        default:
            break;
        }

//      dx11Resource->SetName(createResourceDescription->name);
        backendResource->resourcePtr = dx11Resource;

#ifdef _DEBUG
        wcscpy_s(backendResource->resourceName, createResourceDescription->name);
#endif

        // Create SRVs and UAVs
        {
            D3D11_UNORDERED_ACCESS_VIEW_DESC dx11UavDescription = {};
            D3D11_SHADER_RESOURCE_VIEW_DESC dx11SrvDescription = {};

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
                dx11UavDescription.Format = convertFormatUav(DXGI_FORMAT_UNKNOWN);
                dx11SrvDescription.Format = convertFormatUav(DXGI_FORMAT_UNKNOWN);
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
                break;

            case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
                reinterpret_cast<ID3D11Texture1D*>(dx11Resource)->GetDesc(&dx11Texture1DDescription);
                dx11UavDescription.Format = convertFormatUav(dx11Texture1DDescription.Format);
                dx11SrvDescription.Format = convertFormatUav(dx11Texture1DDescription.Format);
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
                dx11UavDescription.Format = convertFormatUav(dx11Texture2DDescription.Format);
                dx11SrvDescription.Format = convertFormatUav(dx11Texture2DDescription.Format);
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
                dx11UavDescription.Format = convertFormatUav(dx11Texture3DDescription.Format);
                dx11SrvDescription.Format = convertFormatUav(dx11Texture3DDescription.Format);
                dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
                dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                dx11SrvDescription.Texture3D.MipLevels = dx11Texture3DDescription.MipLevels;
                dx11SrvDescription.Texture3D.MostDetailedMip = 0;
                break;

            default:
                break;
            }

            if (resourceDimension == D3D11_RESOURCE_DIMENSION_BUFFER) {

                // UAV
                if (dx11BufferDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                    dx11UavDescription.Buffer.FirstElement = 0;
                    dx11UavDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                    TIF(dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[0]));
                }
                else
                {
                    dx11SrvDescription.Buffer.FirstElement = 0;
                    dx11SrvDescription.Buffer.NumElements = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                    TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr));
                }
            }
            else {
                // CPU readable
                TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr));

                // UAV
                if (dx11Texture1DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                    dx11Texture2DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS ||
                    dx11Texture3DDescription.BindFlags & D3D11_BIND_UNORDERED_ACCESS) {

                    const int32_t uavDescriptorCount = backendResource->resourceDescription.mipCount;

                    for (int32_t currentMipIndex = 0; currentMipIndex < uavDescriptorCount; ++currentMipIndex) {

                        dx11UavDescription.Texture2D.MipSlice = currentMipIndex;

                        dx11Device->CreateUnorderedAccessView(dx11Resource, &dx11UavDescription, &backendResource->uavPtr[currentMipIndex]);
                    }
                }
            }
        }

        // create upload resource and upload job
        if (createResourceDescription->initData) {

            FfxResourceInternal copySrc;
            FfxCreateResourceDescription uploadDescription = { *createResourceDescription };
            uploadDescription.heapType = FFX_HEAP_TYPE_UPLOAD;
            uploadDescription.resourceDescription.usage = FFX_RESOURCE_USAGE_READ_ONLY;
            uploadDescription.initalState = FFX_RESOURCE_STATE_GENERIC_READ;

            backendInterface->fpCreateResource(backendInterface, &uploadDescription, effectContextId, &copySrc);

            // setup the upload job
            FfxGpuJobDescription copyJob = {

                FFX_GPU_JOB_COPY
            };
            copyJob.copyJobDescriptor.src = copySrc;
            copyJob.copyJobDescriptor.dst = *outTexture;

            backendInterface->fpScheduleGpuJob(backendInterface, &copyJob);
        }
    }

    return FFX_OK;
}

FfxErrorCode DestroyResourceDX11(
    FfxInterface* backendInterface,
    FfxResourceInternal resource)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    if (backendContext->pResources[resource.internalIndex].srvPtr) {
        backendContext->pResources[resource.internalIndex].srvPtr->Release();
        backendContext->pResources[resource.internalIndex].srvPtr = nullptr;
    }
    for (int32_t currentMipIndex = 0; currentMipIndex < 16; ++currentMipIndex) {
        if (backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex]) {
            backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex]->Release();
            backendContext->pResources[resource.internalIndex].uavPtr[currentMipIndex] = nullptr;
        }
    }
    if (backendContext->pResources[resource.internalIndex].resourcePtr) {
        backendContext->pResources[resource.internalIndex].resourcePtr->Release();
        backendContext->pResources[resource.internalIndex].resourcePtr = nullptr;
    }

    return FFX_OK;
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

    DestroyResourceDX11(backendInterface, *outFfxResourceInternal);

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
            dx11UavDescription.Format = convertFormatUav(DXGI_FORMAT_UNKNOWN);
            dx11SrvDescription.Format = convertFormatSrv(DXGI_FORMAT_UNKNOWN);
            dx11UavDescription.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
            dx11SrvDescription.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
            backendResource->resourceDescription.type = FFX_RESOURCE_TYPE_BUFFER;
            backendResource->resourceDescription.size = inFfxResource->description.size;
            backendResource->resourceDescription.stride = inFfxResource->description.stride;
            backendResource->resourceDescription.alignment = 0;
            break;

        case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
            reinterpret_cast<ID3D11Texture1D*>(dx11Resource)->GetDesc(&dx11Texture1DDesc);
            dx11UavDescription.Format = convertFormatUav(dx11Texture1DDesc.Format);
            dx11SrvDescription.Format = convertFormatSrv(dx11Texture1DDesc.Format);
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
            dx11UavDescription.Format = convertFormatUav(dx11Texture2DDesc.Format);
            dx11SrvDescription.Format = convertFormatSrv(dx11Texture2DDesc.Format);
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
            dx11UavDescription.Format = convertFormatUav(dx11Texture3DDesc.Format);
            dx11SrvDescription.Format = convertFormatSrv(dx11Texture3DDesc.Format);
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
            else
            {
                dx11SrvDescription.Buffer.FirstElement        = 0;
                dx11SrvDescription.Buffer.NumElements         = backendResource->resourceDescription.size / backendResource->resourceDescription.stride;

                TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr));
            }
        }
        else {

            // CPU readable
            TIF(dx11Device->CreateShaderResourceView(dx11Resource, &dx11SrvDescription, &backendResource->srvPtr));

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
    ffxGetPermutationBlobByIndex(effect, pass, permutationOptions, &shaderBlob);
    FFX_ASSERT(shaderBlob.data && shaderBlob.size);

    // Only set the command signature if this is setup as an indirect workload
    outPipeline->cmdSignature = nullptr;

    // populate the pass.
    outPipeline->srvTextureCount = shaderBlob.srvTextureCount;
    outPipeline->uavTextureCount = shaderBlob.uavTextureCount;
    outPipeline->srvBufferCount  = shaderBlob.srvBufferCount;
    outPipeline->uavBufferCount  = shaderBlob.uavBufferCount;

    // Todo when needed
    //outPipeline->samplerCount      = shaderBlob.samplerCount;
    //outPipeline->rtAccelStructCount= shaderBlob.rtAccelStructCount;

    outPipeline->constCount = shaderBlob.cbvCount;
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvTextureCount; ++srvIndex)
    {
        outPipeline->srvTextureBindings[srvIndex].slotIndex = shaderBlob.boundSRVTextures[srvIndex];
        outPipeline->srvTextureBindings[srvIndex].bindCount = shaderBlob.boundSRVTextureCounts[srvIndex];
        wcscpy_s(outPipeline->srvTextureBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVTextureNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavTextureCount; ++uavIndex)
    {
        outPipeline->uavTextureBindings[uavIndex].slotIndex = shaderBlob.boundUAVTextures[uavIndex];
        outPipeline->uavTextureBindings[uavIndex].bindCount = shaderBlob.boundUAVTextureCounts[uavIndex];
        wcscpy_s(outPipeline->uavTextureBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVTextureNames[uavIndex]).c_str());
    }
    for (uint32_t srvIndex = 0; srvIndex < outPipeline->srvBufferCount; ++srvIndex)
    {
        outPipeline->srvBufferBindings[srvIndex].slotIndex = shaderBlob.boundSRVBuffers[srvIndex];
        outPipeline->srvBufferBindings[srvIndex].bindCount = shaderBlob.boundSRVBufferCounts[srvIndex];
        wcscpy_s(outPipeline->srvBufferBindings[srvIndex].name, converter.from_bytes(shaderBlob.boundSRVBufferNames[srvIndex]).c_str());
    }
    for (uint32_t uavIndex = 0; uavIndex < outPipeline->uavBufferCount; ++uavIndex)
    {
        outPipeline->uavBufferBindings[uavIndex].slotIndex = shaderBlob.boundUAVBuffers[uavIndex];
        outPipeline->uavBufferBindings[uavIndex].bindCount = shaderBlob.boundUAVBufferCounts[uavIndex];
        wcscpy_s(outPipeline->uavBufferBindings[uavIndex].name, converter.from_bytes(shaderBlob.boundUAVBufferNames[uavIndex]).c_str());
    }
    for (uint32_t cbIndex = 0; cbIndex < outPipeline->constCount; ++cbIndex)
    {
        outPipeline->constantBufferBindings[cbIndex].slotIndex = shaderBlob.boundConstantBuffers[cbIndex];
        outPipeline->constantBufferBindings[cbIndex].bindCount = shaderBlob.boundConstantBufferCounts[cbIndex];
        wcscpy_s(outPipeline->constantBufferBindings[cbIndex].name, converter.from_bytes(shaderBlob.boundConstantBufferNames[cbIndex]).c_str());
    }

    // create the PSO
    if (FAILED(dx11Device->CreateComputeShader(shaderBlob.data, shaderBlob.size, nullptr, (ID3D11ComputeShader**)&outPipeline->pipeline)))
        return FFX_ERROR_BACKEND_API_ERROR;

    // Set the pipeline name
//  reinterpret_cast<ID3D11ComputeShader*>(outPipeline->pipeline)->SetName(pipelineDescription->name);

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

    if (job->jobType == FFX_GPU_JOB_COMPUTE) {

        // needs to copy SRVs and UAVs in case they are on the stack only
        FfxComputeJobDescription* computeJob = &backendContext->pGpuJobs[backendContext->gpuJobCount].computeJobDescriptor;
        const uint32_t numConstBuffers = job->computeJobDescriptor.pipeline.constCount;
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex< numConstBuffers; ++currentRootConstantIndex)
        {
            computeJob->cbs[currentRootConstantIndex].num32BitEntries = job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries;
            memcpy(computeJob->cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, computeJob->cbs[currentRootConstantIndex].num32BitEntries*sizeof(uint32_t));
        }
    }

    backendContext->gpuJobCount++;

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCompute(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext1* dx11DeviceContext)
{
    uint32_t minimumUav = UINT32_MAX;
    uint32_t maximumUav = 0;

    // bind texture & buffer UAVs (note the binding order here MUST match the root signature mapping order from CreatePipeline!)
    {
        // Set a baseline minimal value
        uint32_t maximumUavIndex = job->computeJobDescriptor.pipeline.uavTextureCount + job->computeJobDescriptor.pipeline.uavBufferCount;

        // Textures
        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex)
        {
            uint32_t uavResourceOffset = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex +
                                            job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount - 1;
            maximumUavIndex = uavResourceOffset > maximumUavIndex ? uavResourceOffset : maximumUavIndex;
        }

        // Buffers
        for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex)
        {
            uint32_t uavResourceOffset = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex;
            maximumUavIndex = uavResourceOffset > maximumUavIndex ? uavResourceOffset : maximumUavIndex;
        }

        if (maximumUavIndex)
        {
            // Set Texture UAVs
            for (uint32_t currentPipelineUavIndex = 0, currentUAVResource = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavTextureCount; ++currentPipelineUavIndex) {

                for (uint32_t uavEntry = 0; uavEntry < job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].bindCount; ++uavEntry, ++currentUAVResource)
                {
                    // source: UAV of resource to bind
                    const uint32_t resourceIndex = job->computeJobDescriptor.uavTextures[currentUAVResource].internalIndex;
                    ID3D11UnorderedAccessView* uavPtr = backendContext->pResources[resourceIndex].uavPtr[currentUAVResource];

                    // where to bind it
                    const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavTextureBindings[currentPipelineUavIndex].slotIndex + uavEntry;

                    dx11DeviceContext->CSSetUnorderedAccessViews(currentUavResourceIndex, 1, &uavPtr, nullptr);

                    minimumUav = minimumUav < currentUavResourceIndex ? minimumUav : currentUavResourceIndex;
                    maximumUav = maximumUav > currentUavResourceIndex ? maximumUav : currentUavResourceIndex;
                }
            }

            // Set Buffer UAVs
            for (uint32_t currentPipelineUavIndex = 0; currentPipelineUavIndex < job->computeJobDescriptor.pipeline.uavBufferCount; ++currentPipelineUavIndex) {

                // source: UAV of buffer to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.uavBuffers[currentPipelineUavIndex].internalIndex;
                ID3D11UnorderedAccessView* uavPtr = backendContext->pResources[resourceIndex].uavPtr[0];

                // where to bind it
                const uint32_t currentUavResourceIndex = job->computeJobDescriptor.pipeline.uavBufferBindings[currentPipelineUavIndex].slotIndex;

                dx11DeviceContext->CSSetUnorderedAccessViews(currentUavResourceIndex, 1, &uavPtr, nullptr);

                minimumUav = minimumUav < currentUavResourceIndex ? minimumUav : currentUavResourceIndex;
                maximumUav = maximumUav > currentUavResourceIndex ? maximumUav : currentUavResourceIndex;
            }
        }
    }

    // bind texture & buffer SRVs
    {
        // Set a baseline minimal value
        // Textures
        uint32_t maximumSrvIndex = job->computeJobDescriptor.pipeline.srvTextureCount;
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex) {

            const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex +
                                                     job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount - 1;
            maximumSrvIndex = currentSrvResourceIndex > maximumSrvIndex ? currentSrvResourceIndex : maximumSrvIndex;
        }
        // Buffers
        for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
        {
            uint32_t srvResourceOffset = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex;
            maximumSrvIndex            = srvResourceOffset > maximumSrvIndex ? srvResourceOffset : maximumSrvIndex;
        }

        if (maximumSrvIndex)
        {
            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvTextureCount; ++currentPipelineSrvIndex)
            {
                for (uint32_t bindNum = 0; bindNum < job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].bindCount; ++bindNum)
                {
                    uint32_t currPipelineSrvIndex = currentPipelineSrvIndex + bindNum;
                    if (job->computeJobDescriptor.srvTextures[currPipelineSrvIndex].internalIndex == 0)
                        break;

                    // source: SRV of resource to bind
                    const uint32_t resourceIndex = job->computeJobDescriptor.srvTextures[currPipelineSrvIndex].internalIndex;
                    ID3D11ShaderResourceView* srvPtr = backendContext->pResources[resourceIndex].srvPtr;

                    // Where to bind it
                    uint32_t currentSrvResourceIndex;
                    if (bindNum >= 1)
                    {
                        currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currentPipelineSrvIndex].slotIndex + bindNum;
                    }
                    else
                    {
                        currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvTextureBindings[currPipelineSrvIndex].slotIndex;
                    }

                    dx11DeviceContext->CSSetShaderResources(currentSrvResourceIndex, 1, &srvPtr);
                }
            }

            // Set Buffer SRVs
            for (uint32_t currentPipelineSrvIndex = 0; currentPipelineSrvIndex < job->computeJobDescriptor.pipeline.srvBufferCount; ++currentPipelineSrvIndex)
            {
                // source: SRV of buffer to bind
                const uint32_t resourceIndex = job->computeJobDescriptor.srvBuffers[currentPipelineSrvIndex].internalIndex;
                ID3D11ShaderResourceView* srvPtr = backendContext->pResources[resourceIndex].srvPtr;

                // where to bind it
                const uint32_t currentSrvResourceIndex = job->computeJobDescriptor.pipeline.srvBufferBindings[currentPipelineSrvIndex].slotIndex;

                dx11DeviceContext->CSSetShaderResources(currentSrvResourceIndex, 1, &srvPtr);
            }
        }
    }

    // bind pipeline
    ID3D11ComputeShader* dx11ComputeShader = reinterpret_cast<ID3D11ComputeShader*>(job->computeJobDescriptor.pipeline.pipeline);
    dx11DeviceContext->CSSetShader(dx11ComputeShader, nullptr, 0);

    // copy data to constant buffer and bind
    {
        for (uint32_t currentRootConstantIndex = 0; currentRootConstantIndex < job->computeJobDescriptor.pipeline.constCount; ++currentRootConstantIndex) {

            uint32_t size = FFX_ALIGN_UP(job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t), 256);

            if (backendContext->constantBufferOffset + size >= backendContext->constantBufferSize)
                backendContext->constantBufferOffset = 0;

            void* pBuffer = (void*)((uint8_t*)(backendContext->constantBufferMem) + backendContext->constantBufferOffset);
            memcpy(pBuffer, job->computeJobDescriptor.cbs[currentRootConstantIndex].data, job->computeJobDescriptor.cbs[currentRootConstantIndex].num32BitEntries * sizeof(uint32_t));

            uint32_t first = backendContext->constantBufferOffset / sizeof(FfxFloat32x4);
            uint32_t num = size / sizeof(FfxFloat32x4);

            dx11DeviceContext->CSSetConstantBuffers1(0, 1, &backendContext->constantBufferResource, &first, &num);

            // update the offset
            backendContext->constantBufferOffset += size;
        }
    }

    // Dispatch
    dx11DeviceContext->Dispatch(job->computeJobDescriptor.dimensions[0], job->computeJobDescriptor.dimensions[1], job->computeJobDescriptor.dimensions[2]);

    // Clear UAVs
    static ID3D11UnorderedAccessView* const empty[D3D11_1_UAV_SLOT_COUNT] = {};
    if (minimumUav <= maximumUav) {

        uint32_t count = maximumUav - minimumUav + 1;
        dx11DeviceContext->CSSetUnorderedAccessViews(minimumUav, count, empty, nullptr);
    }

    return FFX_OK;
}

static FfxErrorCode executeGpuJobCopy(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext1* dx11DeviceContext)
{
    ID3D11Resource* dx11ResourceSrc = getDX11ResourcePtr(backendContext, job->copyJobDescriptor.src.internalIndex);
    ID3D11Resource* dx11ResourceDst = getDX11ResourcePtr(backendContext, job->copyJobDescriptor.dst.internalIndex);

    dx11DeviceContext->CopyResource(dx11ResourceDst, dx11ResourceSrc);

    return FFX_OK;
}

static FfxErrorCode executeGpuJobClearFloat(BackendContext_DX11* backendContext, FfxGpuJobDescription* job, ID3D11Device* dx11Device, ID3D11DeviceContext1* dx11DeviceContext)
{
    uint32_t idx = job->clearJobDescriptor.target.internalIndex;
    BackendContext_DX11::Resource ffxResource = backendContext->pResources[idx];

    dx11DeviceContext->ClearUnorderedAccessViewFloat(ffxResource.uavPtr[0], job->clearJobDescriptor.color);

    return FFX_OK;
}

FfxErrorCode ExecuteGpuJobsDX11(
    FfxInterface* backendInterface,
    FfxCommandList commandList)
{
    FFX_ASSERT(NULL != backendInterface);

    BackendContext_DX11* backendContext = (BackendContext_DX11*)backendInterface->scratchBuffer;

    FfxErrorCode errorCode = FFX_OK;

    // execute all GpuJobs
    for (uint32_t currentGpuJobIndex = 0; currentGpuJobIndex < backendContext->gpuJobCount; ++currentGpuJobIndex) {

        FfxGpuJobDescription* GpuJob = &backendContext->pGpuJobs[currentGpuJobIndex];
        ID3D11Device* dx11Device = backendContext->device;
        ID3D11DeviceContext1* dx11DeviceContext = backendContext->deviceContext;

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
