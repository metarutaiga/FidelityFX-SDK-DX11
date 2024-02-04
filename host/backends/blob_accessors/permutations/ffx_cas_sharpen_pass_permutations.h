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


typedef unsigned char BYTE;

#include "ffx_cas_sharpen_pass_permutations_0_0.h"
#include "ffx_cas_sharpen_pass_permutations_0_1.h"
#include "ffx_cas_sharpen_pass_permutations_0_2.h"
#include "ffx_cas_sharpen_pass_permutations_0_3.h"
#include "ffx_cas_sharpen_pass_permutations_0_4.h"
#include "ffx_cas_sharpen_pass_permutations_1_0.h"
#include "ffx_cas_sharpen_pass_permutations_1_1.h"
#include "ffx_cas_sharpen_pass_permutations_1_2.h"
#include "ffx_cas_sharpen_pass_permutations_1_3.h"
#include "ffx_cas_sharpen_pass_permutations_1_4.h"

static const struct { const uint8_t* data; uint32_t size; } g_ffx_cas_sharpen_pass_permutations[2][5] = {
    {
        { g_ffx_cas_sharpen_pass_permutations_0_0, sizeof(g_ffx_cas_sharpen_pass_permutations_0_0) },
        { g_ffx_cas_sharpen_pass_permutations_0_1, sizeof(g_ffx_cas_sharpen_pass_permutations_0_1) },
        { g_ffx_cas_sharpen_pass_permutations_0_2, sizeof(g_ffx_cas_sharpen_pass_permutations_0_2) },
        { g_ffx_cas_sharpen_pass_permutations_0_3, sizeof(g_ffx_cas_sharpen_pass_permutations_0_3) },
        { g_ffx_cas_sharpen_pass_permutations_0_4, sizeof(g_ffx_cas_sharpen_pass_permutations_0_4) },
    },
    {
        { g_ffx_cas_sharpen_pass_permutations_1_0, sizeof(g_ffx_cas_sharpen_pass_permutations_1_0) },
        { g_ffx_cas_sharpen_pass_permutations_1_1, sizeof(g_ffx_cas_sharpen_pass_permutations_1_1) },
        { g_ffx_cas_sharpen_pass_permutations_1_2, sizeof(g_ffx_cas_sharpen_pass_permutations_1_2) },
        { g_ffx_cas_sharpen_pass_permutations_1_3, sizeof(g_ffx_cas_sharpen_pass_permutations_1_3) },
        { g_ffx_cas_sharpen_pass_permutations_1_4, sizeof(g_ffx_cas_sharpen_pass_permutations_1_4) },
    },
};
