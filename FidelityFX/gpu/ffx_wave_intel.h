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

#include "extension/IntelExtensions.hlsl"

float4 QuadReadAcrossX(float4 val)
{
    float4 res;
    res.x = IntelExt_QuadReadAcrossX(val.x);
    res.y = IntelExt_QuadReadAcrossX(val.y);
    res.z = IntelExt_QuadReadAcrossX(val.z);
    res.w = IntelExt_QuadReadAcrossX(val.w);
    return res;
}

float4 QuadReadAcrossY(float4 val)
{
    float4 res;
    res.x = IntelExt_QuadReadAcrossY(val.x);
    res.y = IntelExt_QuadReadAcrossY(val.y);
    res.z = IntelExt_QuadReadAcrossY(val.z);
    res.w = IntelExt_QuadReadAcrossY(val.w);
    return res;
}

float4 QuadReadAcrossDiagonal(float4 val)
{
    float4 res;
    res.x = IntelExt_QuadReadAcrossDiagonal(val.x);
    res.y = IntelExt_QuadReadAcrossDiagonal(val.y);
    res.z = IntelExt_QuadReadAcrossDiagonal(val.z);
    res.w = IntelExt_QuadReadAcrossDiagonal(val.w);
    return res;
}

uint ffxWaveMin(uint val)
{
    return IntelExt_WaveActiveMin(val);
}

uint ffxWaveSum(uint val)
{
    return IntelExt_WaveActiveSum(val);
}

uint ffxWaveLaneCount()
{
    return 32;
}

bool ffxWaveIsFirstLane()
{
    return IntelExt_WaveGetLaneIndex() == 0;
}
