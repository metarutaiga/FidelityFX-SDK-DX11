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
#include "ffx_cas_shaderblobs.h"
#include "cas/ffx_cas_private.h"

#include "permutations/ffx_cas_sharpen_pass_permutations.h"

#include "permutations/ffx_cas_sharpen_pass_16bit_permutations.h"

#include <string.h>  // for memset

static FfxShaderBlob casGetSharpenPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int SHARPEN_ONLY = FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_SHARPEN_ONLY);
    int SPACE_CONVERSION = 0;
    if (FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_COLOR_SPACE_LINEAR))
        SPACE_CONVERSION = 0;
    else if (FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_COLOR_SPACE_GAMMA20))
        SPACE_CONVERSION = 1;
    else if (FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_COLOR_SPACE_GAMMA22))
        SPACE_CONVERSION = 2;
    else if (FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_COLOR_SPACE_SRGB_OUTPUT))
        SPACE_CONVERSION = 3;
    else if (FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_COLOR_SPACE_SRGB_INPUT_OUTPUT))
        SPACE_CONVERSION = 4;
    else
        FFX_ASSERT_FAIL("Unknown color-space permutation.");

    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_color                     texture  float4          2d             t0      1 
    // rw_output_color                       UAV  float4          2d             u0      1 
    // cbCAS                             cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbCAS" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_input_color" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "rw_output_color" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_cas_sharpen_pass_16bit_permutations[SHARPEN_ONLY][SPACE_CONVERSION].data
                : g_ffx_cas_sharpen_pass_permutations[SHARPEN_ONLY][SPACE_CONVERSION].data,
        is16bit ? g_ffx_cas_sharpen_pass_16bit_permutations[SHARPEN_ONLY][SPACE_CONVERSION].size
                : g_ffx_cas_sharpen_pass_permutations[SHARPEN_ONLY][SPACE_CONVERSION].size,
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
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
    };

    return blob;
}

FfxErrorCode casGetPermutationBlobByIndex(FfxCasPass passId, uint32_t permutationOptions, FfxShaderBlob* outBlob)
{
    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit  = FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId)
    {
    case FFX_CAS_PASS_SHARPEN:
    {
        FfxShaderBlob blob = casGetSharpenPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
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

FfxErrorCode casIsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFX_CONTAINS_FLAG(permutationOptions, CAS_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFX_OK;
}
