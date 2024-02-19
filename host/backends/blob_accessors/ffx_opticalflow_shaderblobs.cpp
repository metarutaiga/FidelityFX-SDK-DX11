// This file is part of the FidelityFX SDK.
// 
// Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
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


#include <host/ffx_util.h>
#include "ffx_opticalflow_shaderblobs.h"
#include "host/components/opticalflow/ffx_opticalflow_private.h"

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

#include <string.h> // for memset

static FfxShaderBlob opticalflowGetComputeLuminancePyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_luminance_pyramid_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_luminance_pyramid_pass_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetComputeScdDivergencePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_scd_divergence_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_scd_divergence_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_scd_divergence_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_scd_divergence_pass_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetGenerateScdHistogramPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_generate_scd_histogram_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_generate_scd_histogram_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_generate_scd_histogram_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_generate_scd_histogram_pass_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetPrepareLumaPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_prepare_luma_pass_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_prepare_luma_pass_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_prepare_luma_pass_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_prepare_luma_pass_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetComputeOpticalFlowAdvancedPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_compute_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}
 
static FfxShaderBlob opticalflowGetFilterOpticalFlowPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_filter_optical_flow_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_filter_optical_flow_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_filter_optical_flow_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_filter_optical_flow_pass_v5_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

static FfxShaderBlob opticalflowGetScaleOpticalFlowAdvancedPassV5PermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, OPTICALFLOW_HDR_COLOR_INPUT);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    static const char* boundConstantBufferNames[] = { "" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].data
                : g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].data,
        is16bit ? g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_16bit_permutations[HDR_COLOR_INPUT].size
                : g_ffx_opticalflow_scale_optical_flow_advanced_pass_v5_permutations[HDR_COLOR_INPUT].size,
        1,
        1,
        1,
        0,
        0,
        1,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
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
