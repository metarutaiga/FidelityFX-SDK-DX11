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

typedef unsigned char BYTE;

#include "ffx_fsr1_easu_pass_16bit_permutations_0_0_0_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_0_0_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_0_1_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_0_1_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_1_0_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_1_0_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_1_1_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_0_1_1_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_0_0_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_0_0_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_0_1_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_0_1_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_1_0_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_1_0_1.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_1_1_0.h"
#include "ffx_fsr1_easu_pass_16bit_permutations_1_1_1_1.h"

static const struct { const uint8_t* data; uint32_t size; } g_ffx_fsr1_easu_pass_16bit_permutations[2][2][2][2] = {
    {
        {
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_0_0_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_0_0_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_0_0_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_0_0_1) },
            },
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_0_1_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_0_1_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_0_1_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_0_1_1) },
            },
        },
        {
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_1_0_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_1_0_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_1_0_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_1_0_1) },
            },
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_1_1_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_1_1_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_0_1_1_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_0_1_1_1) },
            },
        },
    },
    {
        {
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_0_0_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_0_0_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_0_0_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_0_0_1) },
            },
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_0_1_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_0_1_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_0_1_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_0_1_1) },
            },
        },
        {
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_1_0_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_1_0_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_1_0_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_1_0_1) },
            },
            {
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_1_1_0, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_1_1_0) },
                { g_ffx_fsr1_easu_pass_16bit_permutations_1_1_1_1, sizeof(g_ffx_fsr1_easu_pass_16bit_permutations_1_1_1_1) },
            },
        },
    },
};
