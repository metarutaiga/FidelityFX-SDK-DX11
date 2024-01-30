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

#include "ffx_blur_pass_permutations_3_0.h"
#include "ffx_blur_pass_permutations_3_1.h"
#include "ffx_blur_pass_permutations_3_2.h"
#include "ffx_blur_pass_permutations_5_0.h"
#include "ffx_blur_pass_permutations_5_1.h"
#include "ffx_blur_pass_permutations_5_2.h"
#include "ffx_blur_pass_permutations_7_0.h"
#include "ffx_blur_pass_permutations_7_1.h"
#include "ffx_blur_pass_permutations_7_2.h"
#include "ffx_blur_pass_permutations_9_0.h"
#include "ffx_blur_pass_permutations_9_1.h"
#include "ffx_blur_pass_permutations_9_2.h"
#include "ffx_blur_pass_permutations_11_0.h"
#include "ffx_blur_pass_permutations_11_1.h"
#include "ffx_blur_pass_permutations_11_2.h"
#include "ffx_blur_pass_permutations_13_0.h"
#include "ffx_blur_pass_permutations_13_1.h"
#include "ffx_blur_pass_permutations_13_2.h"
#include "ffx_blur_pass_permutations_15_0.h"
#include "ffx_blur_pass_permutations_15_1.h"
#include "ffx_blur_pass_permutations_15_2.h"
#include "ffx_blur_pass_permutations_17_0.h"
#include "ffx_blur_pass_permutations_17_1.h"
#include "ffx_blur_pass_permutations_17_2.h"
#include "ffx_blur_pass_permutations_19_0.h"
#include "ffx_blur_pass_permutations_19_1.h"
#include "ffx_blur_pass_permutations_19_2.h"
#include "ffx_blur_pass_permutations_21_0.h"
#include "ffx_blur_pass_permutations_21_1.h"
#include "ffx_blur_pass_permutations_21_2.h"

static const struct { const uint8_t* data; uint32_t size; } g_ffx_blur_pass_permutations[10][3] = {
    {
        { g_ffx_blur_pass_permutations_3_0, sizeof(g_ffx_blur_pass_permutations_3_0) },
        { g_ffx_blur_pass_permutations_3_1, sizeof(g_ffx_blur_pass_permutations_3_1) },
        { g_ffx_blur_pass_permutations_3_2, sizeof(g_ffx_blur_pass_permutations_3_2) },
    },
    {
        { g_ffx_blur_pass_permutations_5_0, sizeof(g_ffx_blur_pass_permutations_5_0) },
        { g_ffx_blur_pass_permutations_5_1, sizeof(g_ffx_blur_pass_permutations_5_1) },
        { g_ffx_blur_pass_permutations_5_2, sizeof(g_ffx_blur_pass_permutations_5_2) },
    },
    {
        { g_ffx_blur_pass_permutations_7_0, sizeof(g_ffx_blur_pass_permutations_7_0) },
        { g_ffx_blur_pass_permutations_7_1, sizeof(g_ffx_blur_pass_permutations_7_1) },
        { g_ffx_blur_pass_permutations_7_2, sizeof(g_ffx_blur_pass_permutations_7_2) },
    },
    {
        { g_ffx_blur_pass_permutations_9_0, sizeof(g_ffx_blur_pass_permutations_9_0) },
        { g_ffx_blur_pass_permutations_9_1, sizeof(g_ffx_blur_pass_permutations_9_1) },
        { g_ffx_blur_pass_permutations_9_2, sizeof(g_ffx_blur_pass_permutations_9_2) },
    },
    {
        { g_ffx_blur_pass_permutations_11_0, sizeof(g_ffx_blur_pass_permutations_11_0) },
        { g_ffx_blur_pass_permutations_11_1, sizeof(g_ffx_blur_pass_permutations_11_1) },
        { g_ffx_blur_pass_permutations_11_2, sizeof(g_ffx_blur_pass_permutations_11_2) },
    },
    {
        { g_ffx_blur_pass_permutations_13_0, sizeof(g_ffx_blur_pass_permutations_13_0) },
        { g_ffx_blur_pass_permutations_13_1, sizeof(g_ffx_blur_pass_permutations_13_1) },
        { g_ffx_blur_pass_permutations_13_2, sizeof(g_ffx_blur_pass_permutations_13_2) },
    },
    {
        { g_ffx_blur_pass_permutations_15_0, sizeof(g_ffx_blur_pass_permutations_15_0) },
        { g_ffx_blur_pass_permutations_15_1, sizeof(g_ffx_blur_pass_permutations_15_1) },
        { g_ffx_blur_pass_permutations_15_2, sizeof(g_ffx_blur_pass_permutations_15_2) },
    },
    {
        { g_ffx_blur_pass_permutations_17_0, sizeof(g_ffx_blur_pass_permutations_17_0) },
        { g_ffx_blur_pass_permutations_17_1, sizeof(g_ffx_blur_pass_permutations_17_1) },
        { g_ffx_blur_pass_permutations_17_2, sizeof(g_ffx_blur_pass_permutations_17_2) },
    },
    {
        { g_ffx_blur_pass_permutations_19_0, sizeof(g_ffx_blur_pass_permutations_19_0) },
        { g_ffx_blur_pass_permutations_19_1, sizeof(g_ffx_blur_pass_permutations_19_1) },
        { g_ffx_blur_pass_permutations_19_2, sizeof(g_ffx_blur_pass_permutations_19_2) },
    },
    {
        { g_ffx_blur_pass_permutations_21_0, sizeof(g_ffx_blur_pass_permutations_21_0) },
        { g_ffx_blur_pass_permutations_21_1, sizeof(g_ffx_blur_pass_permutations_21_1) },
        { g_ffx_blur_pass_permutations_21_2, sizeof(g_ffx_blur_pass_permutations_21_2) },
    },
};
