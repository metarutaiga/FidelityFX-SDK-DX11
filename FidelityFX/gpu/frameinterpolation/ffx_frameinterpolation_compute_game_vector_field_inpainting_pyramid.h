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


#ifndef FFX_FRAMEINTERPOLATION_COMPUTE_GAME_VECTOR_FIELD_INPAINTING_PYRAMID_H
#define FFX_FRAMEINTERPOLATION_COMPUTE_GAME_VECTOR_FIELD_INPAINTING_PYRAMID_H

#if FFX_HALF
    #define FFX_SPD_PACKED_ONLY 1
#endif // FFX_HALF

#include "ffx_frameinterpolation_common.h"
//--------------------------------------------------------------------------------------
// Buffer definitions - global atomic counter
//--------------------------------------------------------------------------------------

FFX_GROUPSHARED FfxUInt32 spdCounter;

void SpdIncreaseAtomicCounter(FfxUInt32 slice)
{
    AtomicIncreaseCounter(FfxInt32x2(COUNTER_SPD, 0), spdCounter);
}

FfxUInt32 SpdGetAtomicCounter()
{
    return spdCounter;
}

void SpdResetAtomicCounter(FfxUInt32 slice)
{
    StoreCounter(FfxInt32x2(COUNTER_SPD, 0), 0);
}

#if FFX_HALF

FFX_GROUPSHARED FfxFloat16x2 spdIntermediateRG[16][16];
FFX_GROUPSHARED FfxFloat16x2 spdIntermediateBA[16][16];

FfxFloat16x4 SpdLoadSourceImageH(FfxInt32x2 tex, FfxUInt32 slice)
{
    VectorFieldEntry gameMv;
    FfxUInt32x2      packedGameFieldMv = LoadGameFieldMv(tex);
    UnpackVectorFieldEntries(packedGameFieldMv, gameMv);
    
    return FfxFloat16x4(gameMv.fMotionVector, gameMv.uHighPriorityFactor, gameMv.uLowPriorityFactor) * (DisplaySize().x > 0);
}

FfxFloat16x4 SpdLoadH(FfxInt32x2 tex, FfxUInt32 slice)
{
    return RWLoadInpaintingPyramid(tex, 5);
}

void SpdStoreH(FfxInt32x2 pix, FfxFloat16x4 outValue, FfxUInt32 index, FfxUInt32 slice)
{
    StoreInpaintingPyramid(pix, outValue, index);
}

FfxFloat16x4 SpdLoadIntermediateH(FfxUInt32 x, FfxUInt32 y)
{
    return FfxFloat16x4(
        spdIntermediateRG[x][y].x,
        spdIntermediateRG[x][y].y,
        spdIntermediateBA[x][y].x,
        spdIntermediateBA[x][y].y);
}

void SpdStoreIntermediateH(FfxUInt32 x, FfxUInt32 y, FfxFloat16x4 value)
{
    spdIntermediateRG[x][y] = value.xy;
    spdIntermediateBA[x][y] = value.zw;
}

FfxFloat16x4 SpdReduce4H(FfxFloat16x4 v0, FfxFloat16x4 v1, FfxFloat16x4 v2, FfxFloat16x4 v3)
{
    FfxFloat16x4 vec = FfxFloat16x4(0,0,0,0);

    FfxFloat16 fWeightSum = 0.0f;
#define ADD(SAMPLE) { \
        FfxFloat16 fWeight = (SAMPLE.z > 0.0f); \
        vec += SAMPLE * fWeight; \
        fWeightSum += fWeight; \
        }

    ADD(v0);
    ADD(v1);
    ADD(v2);
    ADD(v3);
#undef ADD

    vec /= (fWeightSum > FFX_FRAMEINTERPOLATION_EPSILON) ? fWeightSum : FfxFloat16(1.0f);

    return vec;
}

#else

FFX_GROUPSHARED FfxFloat32 spdIntermediateR[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateG[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateB[16][16];
FFX_GROUPSHARED FfxFloat32 spdIntermediateA[16][16];

FfxFloat32x4 SpdLoadSourceImage(FfxFloat32x2 tex, FfxUInt32 slice)
{
    VectorFieldEntry gameMv;
    FfxUInt32x2      packedGameFieldMv = LoadGameFieldMv(tex);
    UnpackVectorFieldEntries(packedGameFieldMv, gameMv);
    
    return FfxFloat32x4(gameMv.fMotionVector, gameMv.uHighPriorityFactor, gameMv.uLowPriorityFactor) * (DisplaySize().x > 0);
}

FfxFloat32x4 SpdLoad(FfxFloat32x2 tex, FfxUInt32 slice)
{
    return RWLoadInpaintingPyramid(tex, 5);
}

void SpdStore(FfxInt32x2 pix, FfxFloat32x4 outValue, FfxUInt32 index, FfxUInt32 slice)
{
    StoreInpaintingPyramid(pix, outValue, index);
}

FfxFloat32x4 SpdLoadIntermediate(FfxUInt32 x, FfxUInt32 y)
{
    return FfxFloat32x4(
        spdIntermediateR[x][y],
        spdIntermediateG[x][y],
        spdIntermediateB[x][y],
        spdIntermediateA[x][y]);
}

void SpdStoreIntermediate(FfxUInt32 x, FfxUInt32 y, FfxFloat32x4 value)
{
    spdIntermediateR[x][y] = value.x;
    spdIntermediateG[x][y] = value.y;
    spdIntermediateB[x][y] = value.z;
    spdIntermediateA[x][y] = value.w;
}

FfxFloat32x4 SpdReduce4(FfxFloat32x4 v0, FfxFloat32x4 v1, FfxFloat32x4 v2, FfxFloat32x4 v3)
{
    FfxFloat32x4 vec = FfxFloat32x4(0,0,0,0);

    FfxFloat32 fWeightSum = 0.0f;
#define ADD(SAMPLE) { \
        FfxFloat32 fWeight = (SAMPLE.z > 0.0f); \
        vec += SAMPLE * fWeight; \
        fWeightSum += fWeight; \
        }

    ADD(v0);
    ADD(v1);
    ADD(v2);
    ADD(v3);
#undef ADD

    vec /= (fWeightSum > FFX_FRAMEINTERPOLATION_EPSILON) ? fWeightSum : 1.0f;

    return vec;
}

#endif // FFX_HALF

#include "spd/ffx_spd.h"

void computeFrameinterpolationGameVectorFieldInpaintingPyramid(FfxInt32x3 iGroupId, FfxInt32 iLocalIndex)
{
#if FFX_HALF
    SpdDownsampleH(
        FfxUInt32x2(iGroupId.xy),
        FfxUInt32(iLocalIndex),
        FfxUInt32(NumMips()),
        FfxUInt32(NumWorkGroups()),
        FfxUInt32(iGroupId.z),
        FfxUInt32x2(WorkGroupOffset()));
#else
    SpdDownsample(
        FfxUInt32x2(iGroupId.xy),
        FfxUInt32(iLocalIndex),
        FfxUInt32(NumMips()),
        FfxUInt32(NumWorkGroups()),
        FfxUInt32(iGroupId.z),
        FfxUInt32x2(WorkGroupOffset()));
#endif // FFX_HALF
}

#endif // FFX_FRAMEINTERPOLATION_COMPUTE_GAME_VECTOR_FIELD_INPAINTING_PYRAMID_H
