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

#define NV_SHADER_EXTN_SLOT u15
#include "extension/nvHLSLExtns.h"

float4 QuadReadAcrossX(float4 val)
{
    float4 res;
    res.x = asfloat(NvShflXor(asint(val.x), 0xffffffffu, 1));
    res.y = asfloat(NvShflXor(asint(val.y), 0xffffffffu, 1));
    res.z = asfloat(NvShflXor(asint(val.z), 0xffffffffu, 1));
    res.w = asfloat(NvShflXor(asint(val.w), 0xffffffffu, 1));
    return res;
}

float4 QuadReadAcrossY(float4 val)
{
    float4 res;
    res.x = asfloat(NvShflXor(asint(val.x), 0xffffffffu, 2));
    res.y = asfloat(NvShflXor(asint(val.y), 0xffffffffu, 2));
    res.z = asfloat(NvShflXor(asint(val.z), 0xffffffffu, 2));
    res.w = asfloat(NvShflXor(asint(val.w), 0xffffffffu, 2));
    return res;
}

float4 QuadReadAcrossDiagonal(float4 val)
{
    float4 res;
    res.x = asfloat(NvShflXor(asint(val.x), 0xffffffffu, 3));
    res.y = asfloat(NvShflXor(asint(val.y), 0xffffffffu, 3));
    res.z = asfloat(NvShflXor(asint(val.z), 0xffffffffu, 3));
    res.w = asfloat(NvShflXor(asint(val.w), 0xffffffffu, 3));
    return res;
}

uint ffxWaveMin(uint val)
{
    val = min(val, NvShflXor(val, 0xfffffffu, 1));
    val = min(val, NvShflXor(val, 0xfffffffu, 2));
    val = min(val, NvShflXor(val, 0xfffffffu, 4));
    val = min(val, NvShflXor(val, 0xfffffffu, 8));
    val = min(val, NvShflXor(val, 0xfffffffu, 16));
    return val;
}

uint ffxWaveSum(uint val)
{
    val += NvShflXor(val, 0xfffffffu, 1);
    val += NvShflXor(val, 0xfffffffu, 2);
    val += NvShflXor(val, 0xfffffffu, 4);
    val += NvShflXor(val, 0xfffffffu, 8);
    val += NvShflXor(val, 0xfffffffu, 16);
    return val;
}

uint ffxWaveLaneCount()
{
    return 32;
}

bool ffxWaveIsFirstLane()
{
    return NvGetLaneId() == 0;
}
