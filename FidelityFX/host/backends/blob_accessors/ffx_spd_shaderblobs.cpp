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


#include <FidelityFX/host/ffx_util.h>
#include "ffx_spd_shaderblobs.h"
#include "spd/ffx_spd_private.h"

#include "permutations/ffx_spd_downsample_pass_permutations.h"
#include "permutations/ffx_spd_downsample_pass_16bit_permutations.h"

#include <string.h> // for memset

static FfxShaderBlob spdGetDownsamplePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int LINEAR_SAMPLE = FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_LINEAR_SAMPLE);
    int WAVE_INTEROP_LDS = FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_WAVE_INTEROP_LDS);
    int DOWNSAMPLE_FILTER = 0;
    if (FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_DOWNSAMPLE_FILTER_MEAN))
        DOWNSAMPLE_FILTER = 0;
    else if (FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_DOWNSAMPLE_FILTER_MIN))
        DOWNSAMPLE_FILTER = 1;
    else if (FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_DOWNSAMPLE_FILTER_MAX))
        DOWNSAMPLE_FILTER = 2;
    else
        FFX_ASSERT_FAIL("Unknown filter.");

    // Resource Bindings:
    //
    // Name                                   Type  Format         Dim      HLSL Bind  Count
    // -------------------------------- ---------- ------- ----------- -------------- ------
    // rw_internal_global_atomic               UAV  struct         r/w             u0      1 
    // rw_input_downsample_src_mid_mip         UAV  float4     2darray             u1      1 
    // rw_input_downsample_src_mips[0]         UAV  float4     2darray             u2      1 
    // rw_input_downsample_src_mips[1]         UAV  float4     2darray             u3      1 
    // rw_input_downsample_src_mips[2]         UAV  float4     2darray             u4      1 
    // rw_input_downsample_src_mips[3]         UAV  float4     2darray             u5      1 
    // rw_input_downsample_src_mips[4]         UAV  float4     2darray             u6      1 
    // rw_input_downsample_src_mips[5]         UAV  float4     2darray             u7      1 
    // rw_input_downsample_src_mips[7]         UAV  float4     2darray             u9      1 
    // rw_input_downsample_src_mips[8]         UAV  float4     2darray            u10      1 
    // rw_input_downsample_src_mips[9]         UAV  float4     2darray            u11      1 
    // rw_input_downsample_src_mips[10]        UAV  float4     2darray            u12      1 
    // rw_input_downsample_src_mips[11]        UAV  float4     2darray            u13      1 
    // rw_input_downsample_src_mips[12]        UAV  float4     2darray            u14      1 
    // cbSPD                               cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbSPD" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "rw_internal_global_atomic", "rw_input_downsample_src_mid_mip", "rw_input_downsample_src_mips" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 13 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_spd_downsample_pass_16bit_permutations[LINEAR_SAMPLE][WAVE_INTEROP_LDS][DOWNSAMPLE_FILTER].data
                : g_ffx_spd_downsample_pass_permutations[LINEAR_SAMPLE][WAVE_INTEROP_LDS][DOWNSAMPLE_FILTER].data,
        is16bit ? g_ffx_spd_downsample_pass_16bit_permutations[LINEAR_SAMPLE][WAVE_INTEROP_LDS][DOWNSAMPLE_FILTER].size
                : g_ffx_spd_downsample_pass_permutations[LINEAR_SAMPLE][WAVE_INTEROP_LDS][DOWNSAMPLE_FILTER].size,
        __crt_countof(boundConstantBufferNames),
        0,
        __crt_countof(boundUAVTextureNames),
        0,
        0,
        0,
        0,
        boundConstantBufferNames,
        boundConstantBuffers,
        boundConstantBufferCounts,
        0,
        nullptr,
        nullptr,
        nullptr,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

FfxErrorCode spdGetPermutationBlobByIndex(
    FfxSpdPass passId,
    uint32_t permutationOptions,
    FfxShaderBlob* outBlob) {

    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit = FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_SPD_PASS_DOWNSAMPLE:
        {
            FfxShaderBlob blob = spdGetDownsamplePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
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

FfxErrorCode spdIsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFX_CONTAINS_FLAG(permutationOptions, SPD_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFX_OK;
}
