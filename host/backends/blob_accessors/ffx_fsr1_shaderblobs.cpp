// This file is part of the FidelityFX SDK.
//
// Copyright (C) 2023 Advanced Micro Devices, Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this softwareand associated documentation files(the “Software”), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// 
// The above copyright noticeand this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <host/ffx_util.h>
#include "ffx_fsr1_shaderblobs.h"
#include <host/ffx_fsr1_private.h>

typedef unsigned char BYTE;

#include "ffx_fsr1_easu_pass_permutations.h"
#include "ffx_fsr1_rcas_pass_permutations.h"

#include "ffx_fsr1_easu_pass_16bit_permutations.h"
#include "ffx_fsr1_rcas_pass_16bit_permutations.h"

#include <string.h> // for memset

FfxErrorCode fsr1GetPermutationBlobByIndex(
    FfxFsr1Pass passId,
    uint32_t permutationOptions,
    FfxShaderBlob* outBlob) {

    bool is16bit = FFX_CONTAINS_FLAG(permutationOptions, FSR1_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_FSR1_PASS_EASU:
        case FFX_FSR1_PASS_EASU_RCAS:
        {
            FfxShaderBlob blob = {
                is16bit ? g_ffx_fsr1_easu_pass_16bit_permutations : g_ffx_fsr1_easu_pass_permutations,
                is16bit ? sizeof(g_ffx_fsr1_easu_pass_16bit_permutations) : sizeof(g_ffx_fsr1_easu_pass_permutations),
            };
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR1_PASS_RCAS:
        {
            FfxShaderBlob blob = {
                is16bit ? g_ffx_fsr1_rcas_pass_16bit_permutations : g_ffx_fsr1_rcas_pass_permutations,
                is16bit ? sizeof(g_ffx_fsr1_rcas_pass_16bit_permutations) : sizeof(g_ffx_fsr1_rcas_pass_permutations),
            };
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        default:
            FFX_ASSERT_FAIL("Should never reach here.");
            break;
    }

    // return an empty blob
    memset(&outBlob, 0, sizeof(FfxShaderBlob));
    return FFX_OK;
}
