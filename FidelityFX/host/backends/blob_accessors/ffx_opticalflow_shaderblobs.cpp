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

#include <FidelityFX/host/ffx_util.h>
#include "ffx_opticalflow_shaderblobs.h"
#include "opticalflow/ffx_opticalflow_private.h"

#include "permutations/ffx_opticalflow_compute_luminance_pyramid_pass_permutations.h"
#include "permutations/ffx_opticalflow_compute_luminance_pyramid_pass_16bit_permutations.h"

#include "permutations/ffx_opticalflow_compute_optical_flow_advanced_pass_v5_permutations.h"
#include "permutations/ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations.h"

#include "permutations/ffx_opticalflow_compute_scd_divergence_pass_permutations.h"
#include "permutations/ffx_opticalflow_compute_scd_divergence_pass_16bit_permutations.h"

#include "permutations/ffx_opticalflow_filter_optical_flow_pass_v5_permutations.h"
#include "permutations/ffx_opticalflow_filter_optical_flow_pass_v5_16bit_permutations.h"

#include "permutations/ffx_opticalflow_generate_scd_histogram_pass_permutations.h"
#include "permutations/ffx_opticalflow_generate_scd_histogram_pass_16bit_permutations.h"

#include "permutations/ffx_opticalflow_prepare_luma_pass_permutations.h"
#include "permutations/ffx_opticalflow_prepare_luma_pass_16bit_permutations.h"

#include "permutations/ffx_opticalflow_scale_optical_flow_advanced_pass_v5_permutations.h"
#include "permutations/ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit_permutations.h"

#include "permutations_amd/amd_opticalflow_compute_luminance_pyramid_pass_permutations.h"
#include "permutations_amd/amd_opticalflow_compute_luminance_pyramid_pass_16bit_permutations.h"

#include "permutations_amd/amd_opticalflow_compute_optical_flow_advanced_pass_v5_permutations.h"
#include "permutations_amd/amd_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations.h"

#include "permutations_nvidia/nv_opticalflow_compute_luminance_pyramid_pass_permutations.h"
#include "permutations_nvidia/nv_opticalflow_compute_luminance_pyramid_pass_16bit_permutations.h"

#include "permutations_nvidia/nv_opticalflow_compute_optical_flow_advanced_pass_v5_permutations.h"
#include "permutations_nvidia/nv_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations.h"

#include <string.h> // for memset

static FfxShaderBlob opticalflowGetComputeLuminancePyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // rw_optical_flow_input                 UAV    uint          2d             u0      1 
    // rw_optical_flow_input_level_1         UAV    uint          2d             u1      1 
    // rw_optical_flow_input_level_2         UAV    uint          2d             u2      1 
    // rw_optical_flow_input_level_3         UAV    uint          2d             u3      1 
    // rw_optical_flow_input_level_4         UAV    uint          2d             u4      1 
    // rw_optical_flow_input_level_5         UAV    uint          2d             u5      1 
    // rw_optical_flow_input_level_6         UAV    uint          2d             u6      1 
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_input", "rw_optical_flow_input_level_1", "rw_optical_flow_input_level_2", "rw_optical_flow_input_level_3", "rw_optical_flow_input_level_4", "rw_optical_flow_input_level_5", "rw_optical_flow_input_level_6" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4, 5, 6 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].size,
        0,
        0,
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    extern FfxUInt32 ffxDeviceVendor;
    if (ffxDeviceVendor == '1002') {
        const_cast<const uint8_t*&>(blob.data) =
            is16bit ? g_amd_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].data
                    : g_amd_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].data;
        const_cast<uint32_t&>(blob.size) =
            is16bit ? g_amd_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].size
                    : g_amd_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].size;
    }
    if (ffxDeviceVendor == '10de') {
        const_cast<const uint8_t*&>(blob.data) =
            is16bit ? g_nv_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].data
                    : g_nv_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].data;
        const_cast<uint32_t&>(blob.size) =
            is16bit ? g_nv_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].size
                    : g_nv_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].size;
    }

    return blob;
}

static FfxShaderBlob opticalflowGetComputeScdDivergencePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    // rw_optical_flow_scd_histogram                 UAV    uint          2d             u0      1 
    // rw_optical_flow_scd_previous_histogram        UAV   float          2d             u1      1 
    // rw_optical_flow_scd_temp                      UAV    uint          2d             u2      1 
    // rw_optical_flow_scd_output                    UAV    uint          2d             u3      1 
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_scd_histogram", "rw_optical_flow_scd_previous_histogram", "rw_optical_flow_scd_temp", "rw_optical_flow_scd_output" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_scd_divergence_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_scd_divergence_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_scd_divergence_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_scd_divergence_pass_permutations[HDR_COLOR_INPUT].size,
        0,
        0,
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetGenerateScdHistogramPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_optical_flow_input              texture    uint          2d             t0      1 
    // rw_optical_flow_scd_histogram         UAV    uint          2d             u0      1 
    // cbOF                              cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbOF" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow_input" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_scd_histogram" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_generate_scd_histogram_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_generate_scd_histogram_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_generate_scd_histogram_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_generate_scd_histogram_pass_permutations[HDR_COLOR_INPUT].size,
        __crt_countof(boundConstantBufferNames),
        __crt_countof(boundSRVTextureNames),
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetPrepareLumaPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_color                     texture  float4          2d             t0      1 
    // rw_optical_flow_input                 UAV    uint          2d             u0      1 
    // cbOF                              cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbOF" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_color" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_input" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_prepare_luma_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_prepare_luma_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_prepare_luma_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_prepare_luma_pass_permutations[HDR_COLOR_INPUT].size,
        __crt_countof(boundConstantBufferNames),
        __crt_countof(boundSRVTextureNames),
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetComputeOpticalFlowAdvancedPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_optical_flow_input              texture    uint          2d             t0      1 
    // r_optical_flow_previous_input     texture    uint          2d             t1      1 
    // rw_optical_flow                       UAV   sint2          2d             u0      1 
    // rw_optical_flow_scd_output            UAV    uint          2d             u1      1 
    // cbOF                              cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbOF" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow_input", "r_optical_flow_previous_input" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow", "rw_optical_flow_scd_output" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size,
        __crt_countof(boundConstantBufferNames),
        __crt_countof(boundSRVTextureNames),
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    extern FfxUInt32 ffxDeviceVendor;
    if (ffxDeviceVendor == '1002') {
        const_cast<const uint8_t*&>(blob.data) =
            is16bit ? g_amd_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                    : g_amd_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data;
        const_cast<uint32_t&>(blob.size) =
            is16bit ? g_amd_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                    : g_amd_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size;
    }
    if (ffxDeviceVendor == '10de') {
        const_cast<const uint8_t*&>(blob.data) =
            is16bit ? g_nv_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                    : g_nv_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data;
        const_cast<uint32_t&>(blob.size) =
            is16bit ? g_nv_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                    : g_nv_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size;
    }

    return blob;
}
 
static FfxShaderBlob opticalflowGetFilterOpticalFlowPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_optical_flow_previous           texture   sint2          2d             t0      1 
    // rw_optical_flow                       UAV   sint2          2d             u0      1 
    static const char* boundSRVTextureNames[] = { "r_optical_flow_previous" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_filter_optical_flow_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_filter_optical_flow_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_filter_optical_flow_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_filter_optical_flow_pass_v5_permutations[HDR_COLOR_INPUT].size,
        0,
        __crt_countof(boundSRVTextureNames),
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetScaleOpticalFlowAdvancedPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_optical_flow_input              texture    uint          2d             t0      1 
    // r_optical_flow_previous_input     texture    uint          2d             t1      1 
    // r_optical_flow                    texture   sint2          2d             t2      1 
    // rw_optical_flow_next_level            UAV   sint2          2d             u0      1 
    // rw_optical_flow_scd_output            UAV    uint          2d             u1      1 
    // cbOF                              cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbOF" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow_input", "r_optical_flow_previous_input", "r_optical_flow" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_next_level", "rw_optical_flow_scd_output" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size,
        __crt_countof(boundConstantBufferNames),
        __crt_countof(boundSRVTextureNames),
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

FfxErrorCode opticalflowGetPermutationBlobByIndex(
    FfxOpticalflowPass passId,
    uint32_t permutationOptions,
    FfxShaderBlob* outBlob) {

    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit = FFX_CONTAINS_FLAG(permutationOptions,  OPTICALFLOW_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_OPTICALFLOW_PASS_PREPARE_LUMA:
        {
            FfxShaderBlob blob = opticalflowGetPrepareLumaPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_OPTICALFLOW_PASS_GENERATE_OPTICAL_FLOW_INPUT_PYRAMID:
        {
            FfxShaderBlob blob = opticalflowGetComputeLuminancePyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_OPTICALFLOW_PASS_GENERATE_SCD_HISTOGRAM:
        {
            FfxShaderBlob blob = opticalflowGetGenerateScdHistogramPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_OPTICALFLOW_PASS_COMPUTE_SCD_DIVERGENCE:
        {
            FfxShaderBlob blob = opticalflowGetComputeScdDivergencePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_OPTICALFLOW_PASS_COMPUTE_OPTICAL_FLOW_ADVANCED_V5:
        {
            FfxShaderBlob blob = opticalflowGetComputeOpticalFlowAdvancedPassV5PermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }
        
        case FFX_OPTICALFLOW_PASS_FILTER_OPTICAL_FLOW_V5:
        {
            FfxShaderBlob blob = opticalflowGetFilterOpticalFlowPassV5PermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }
        
        case FFX_OPTICALFLOW_PASS_SCALE_OPTICAL_FLOW_ADVANCED_V5:
        {
            FfxShaderBlob blob = opticalflowGetScaleOpticalFlowAdvancedPassV5PermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        default:
            FFX_ASSERT_FAIL("Should never reach here.");
            break;
    }

    // return an empty blob
    memset(outBlob, 0, sizeof(FfxShaderBlob));
    return FFX_OK;
}

FfxErrorCode opticalflowIsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFX_OK;
}
