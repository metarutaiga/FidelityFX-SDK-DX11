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


#define FFX_CPU
#include <FidelityFX/host/ffx_util.h>
#include "ffx_frameinterpolation_shaderblobs.h"
#include "frameinterpolation/ffx_frameinterpolation_private.h"

#include "permutations/ffx_frameinterpolation_disocclusion_mask_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_reconstruct_previous_depth_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_setup_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_game_motion_vector_field_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_optical_flow_vector_field_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_Compute_Game_Vector_Field_Inpainting_Pyramid_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_reconstruct_and_dilate_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_Compute_Inpainting_Pyramid_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_Inpainting_pass_permutations.h"
#include "permutations/ffx_frameinterpolation_debug_view_pass_permutations.h"

#include "permutations/ffx_frameinterpolation_disocclusion_mask_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_reconstruct_previous_depth_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_setup_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_game_motion_vector_field_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_optical_flow_vector_field_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_Compute_Game_Vector_Field_Inpainting_Pyramid_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_reconstruct_and_dilate_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_Compute_Inpainting_Pyramid_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_Inpainting_pass_16bit_permutations.h"
#include "permutations/ffx_frameinterpolation_debug_view_pass_16bit_permutations.h"

#include <string.h> // for memset

static FfxShaderBlob FrameInterpolationGetReconstructAndDilatePermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                        Type  Format         Dim      HLSL Bind  Count
    // ------------------------------------- ---------- ------- ----------- -------------- ------
    // r_input_motion_vectors                   texture  float4          2d             t0      1 
    // r_input_depth                            texture   float          2d             t1      1 
    // rw_reconstructed_depth_previous_frame        UAV    uint          2d             u0      1 
    // rw_dilated_motion_vectors                    UAV  float2          2d             u1      1 
    // rw_dilated_depth                             UAV   float          2d             u2      1 
    // cbFI                                     cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_input_motion_vectors", "r_input_depth" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_reconstructed_depth_previous_frame", "rw_dilated_motion_vectors", "rw_dilated_depth" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_setup_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_setup_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_setup_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_setup_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetSetupPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                        Type  Format         Dim      HLSL Bind  Count
    // ------------------------------------- ---------- ------- ----------- -------------- ------
    // r_optical_flow_scd                       texture    uint          2d             t0      1 
    // rw_game_motion_vector_field_x                UAV    uint          2d             u0      1 
    // rw_game_motion_vector_field_y                UAV    uint          2d             u1      1 
    // rw_optical_flow_motion_vector_field_x        UAV    uint          2d             u2      1 
    // rw_optical_flow_motion_vector_field_y        UAV    uint          2d             u3      1 
    // rw_disocclusion_mask                         UAV  float2          2d             u4      1 
    // rw_counters                                  UAV    uint          2d             u5      1 
    // cbFI                                     cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow_scd" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "rw_game_motion_vector_field_x", "rw_game_motion_vector_field_y", "rw_optical_flow_motion_vector_field_x", "rw_optical_flow_motion_vector_field_y", "rw_disocclusion_mask", "rw_counters" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4, 5 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_setup_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_setup_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_setup_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_setup_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetGameMotionVectorFieldPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                  Type  Format         Dim      HLSL Bind  Count
    // ------------------------------- ---------- ------- ----------- -------------- ------
    // r_dilated_motion_vectors           texture  float2          2d             t0      1 
    // r_dilated_depth                    texture   float          2d             t1      1 
    // r_previous_interpolation_source    texture  float4          2d             t2      1 
    // rw_game_motion_vector_field_x          UAV    uint          2d             u0      1 
    // rw_game_motion_vector_field_y          UAV    uint          2d             u1      1 
    // cbFI                               cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_dilated_motion_vectors", "r_dilated_depth", "r_previous_interpolation_source" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_game_motion_vector_field_x", "rw_game_motion_vector_field_y" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_game_motion_vector_field_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_game_motion_vector_field_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_game_motion_vector_field_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_game_motion_vector_field_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetOpticalFlowVectorFieldPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                        Type  Format         Dim      HLSL Bind  Count
    // ------------------------------------- ---------- ------- ----------- -------------- ------
    // r_optical_flow                           texture   sint2          2d             t0      1 
    // r_previous_interpolation_source          texture  float4          2d             t3      1 
    // r_current_interpolation_source           texture  float4          2d             t4      1 
    // rw_optical_flow_motion_vector_field_x        UAV    uint          2d             u0      1 
    // rw_optical_flow_motion_vector_field_y        UAV    uint          2d             u1      1 
    // cbFI                                     cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow", "r_previous_interpolation_source", "r_current_interpolation_source" };
    static const uint32_t boundSRVTextures[] = { 0, 3, 4 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_optical_flow_motion_vector_field_x", "rw_optical_flow_motion_vector_field_y" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_optical_flow_vector_field_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_optical_flow_vector_field_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_optical_flow_vector_field_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_optical_flow_vector_field_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetReconstructPrevDepthPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                            Type  Format         Dim      HLSL Bind  Count
    // ----------------------------------------- ---------- ------- ----------- -------------- ------
    // r_dilated_motion_vectors                     texture  float2          2d             t0      1 
    // r_dilated_depth                              texture   float          2d             t1      1 
    // rw_reconstructed_depth_interpolated_frame        UAV    uint          2d             u0      1 
    // cbFI                                         cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_dilated_motion_vectors", "r_dilated_depth" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_reconstructed_depth_interpolated_frame" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_reconstruct_previous_depth_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_reconstruct_previous_depth_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_reconstruct_previous_depth_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_reconstruct_previous_depth_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetDisocclusionMaskPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                           Type  Format         Dim      HLSL Bind  Count
    // ---------------------------------------- ---------- ------- ----------- -------------- ------
    // r_game_motion_vector_field_x                texture    uint          2d             t0      1 
    // r_game_motion_vector_field_y                texture    uint          2d             t1      1 
    // r_reconstructed_depth_previous_frame        texture    uint          2d             t2      1 
    // r_dilated_depth                             texture   float          2d             t3      1 
    // r_reconstructed_depth_interpolated_frame    texture    uint          2d             t4      1 
    // r_inpainting_pyramid                        texture  float4          2d             t5      1 
    // rw_disocclusion_mask                            UAV  float2          2d             u0      1 
    // cbFI                                        cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_game_motion_vector_field_x", "r_game_motion_vector_field_y", "r_reconstructed_depth_previous_frame", "r_dilated_depth", "r_reconstructed_depth_interpolated_frame", "r_inpainting_pyramid" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_disocclusion_mask" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_disocclusion_mask_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_disocclusion_mask_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_disocclusion_mask_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_disocclusion_mask_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetComputeInpaintingPyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_output                          texture  float4          2d             t0      1 
    // rw_counters                           UAV    uint          2d             u0      1 
    // rw_inpainting_pyramid0                UAV  float4          2d             u1      1 
    // rw_inpainting_pyramid1                UAV  float4          2d             u2      1 
    // rw_inpainting_pyramid2                UAV  float4          2d             u3      1 
    // rw_inpainting_pyramid3                UAV  float4          2d             u4      1 
    // rw_inpainting_pyramid4                UAV  float4          2d             u5      1 
    // rw_inpainting_pyramid5                UAV  float4          2d             u6      1 
    // rw_inpainting_pyramid6                UAV  float4          2d             u7      1 
    // rw_inpainting_pyramid7                UAV  float4          2d             u8      1 
    // rw_inpainting_pyramid8                UAV  float4          2d             u9      1 
    // rw_inpainting_pyramid9                UAV  float4          2d            u10      1 
    // rw_inpainting_pyramid10               UAV  float4          2d            u11      1 
    // rw_inpainting_pyramid11               UAV  float4          2d            u12      1 
    // cbFI                              cbuffer      NA          NA            cb0      1 
    // cbInpaintingPyramid               cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFI", "cbInpaintingPyramid" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const char* boundSRVTextureNames[] = { "r_output" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const char* boundUAVTextureNames[] = { "rw_counters", "rw_inpainting_pyramid0", "rw_inpainting_pyramid1", "rw_inpainting_pyramid2", "rw_inpainting_pyramid3", "rw_inpainting_pyramid4", "rw_inpainting_pyramid5", "rw_inpainting_pyramid6", "rw_inpainting_pyramid7", "rw_inpainting_pyramid8", "rw_inpainting_pyramid9", "rw_inpainting_pyramid10", "rw_inpainting_pyramid11" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_compute_inpainting_pyramid_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_compute_inpainting_pyramid_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_compute_inpainting_pyramid_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_compute_inpainting_pyramid_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetFiPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                       Type  Format         Dim      HLSL Bind  Count
    // ------------------------------------ ---------- ------- ----------- -------------- ------
    // r_game_motion_vector_field_x            texture    uint          2d             t0      1 
    // r_game_motion_vector_field_y            texture    uint          2d             t1      1 
    // r_optical_flow_motion_vector_field_x    texture    uint          2d             t2      1 
    // r_optical_flow_motion_vector_field_y    texture    uint          2d             t3      1 
    // r_previous_interpolation_source         texture  float4          2d             t4      1 
    // r_current_interpolation_source          texture  float4          2d             t5      1 
    // r_disocclusion_mask                     texture  float4          2d             t6      1 
    // r_inpainting_pyramid                    texture  float4          2d             t7      1 
    // r_counters                              texture    uint          2d             t8      1 
    // rw_output                                   UAV  float4          2d             u0      1 
    // cbFI                                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_game_motion_vector_field_x", "r_game_motion_vector_field_y", "r_optical_flow_motion_vector_field_x", "r_optical_flow_motion_vector_field_y", "r_previous_interpolation_source", "r_current_interpolation_source", "r_disocclusion_mask", "r_inpainting_pyramid", "r_counters" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetComputeGameVectorFieldInpaintingPyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_game_motion_vector_field_x      texture    uint          2d             t0      1 
    // r_game_motion_vector_field_y      texture    uint          2d             t1      1 
    // rw_counters                           UAV    uint          2d             u0      1 
    // rw_inpainting_pyramid0                UAV  float4          2d             u1      1 
    // rw_inpainting_pyramid1                UAV  float4          2d             u2      1 
    // rw_inpainting_pyramid2                UAV  float4          2d             u3      1 
    // rw_inpainting_pyramid3                UAV  float4          2d             u4      1 
    // rw_inpainting_pyramid4                UAV  float4          2d             u5      1 
    // rw_inpainting_pyramid5                UAV  float4          2d             u6      1 
    // rw_inpainting_pyramid6                UAV  float4          2d             u7      1 
    // rw_inpainting_pyramid7                UAV  float4          2d             u8      1 
    // rw_inpainting_pyramid8                UAV  float4          2d             u9      1 
    // rw_inpainting_pyramid9                UAV  float4          2d            u10      1 
    // rw_inpainting_pyramid10               UAV  float4          2d            u11      1 
    // rw_inpainting_pyramid11               UAV  float4          2d            u12      1 
    // cbFI                              cbuffer      NA          NA            cb0      1 
    // cbInpaintingPyramid               cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFI", "cbInpaintingPyramid" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const char* boundSRVTextureNames[] = { "r_game_motion_vector_field_x", "r_game_motion_vector_field_y" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_counters", "rw_inpainting_pyramid0", "rw_inpainting_pyramid1", "rw_inpainting_pyramid2", "rw_inpainting_pyramid3", "rw_inpainting_pyramid4", "rw_inpainting_pyramid5", "rw_inpainting_pyramid6", "rw_inpainting_pyramid7", "rw_inpainting_pyramid8", "rw_inpainting_pyramid9", "rw_inpainting_pyramid10", "rw_inpainting_pyramid11" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetInpaintingPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_optical_flow_scd                texture    uint          2d             t0      1 
    // r_inpainting_pyramid              texture  float4          2d             t1      1 
    // r_present_backbuffer              texture  float4          2d             t2      1 
    // r_current_interpolation_source    texture  float4          2d             t3      1 
    // rw_output                             UAV  float4          2d             u0      1 
    // cbFI                              cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_optical_flow_scd", "r_inpainting_pyramid", "r_present_backbuffer", "r_current_interpolation_source" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_inpainting_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_inpainting_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_inpainting_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_inpainting_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

static FfxShaderBlob FrameInterpolationGetDebugViewPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    bool INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_DEPTH_INVERTED);

    // Resource Bindings:
    //
    // Name                                       Type  Format         Dim      HLSL Bind  Count
    // ------------------------------------ ---------- ------- ----------- -------------- ------
    // r_game_motion_vector_field_x            texture    uint          2d             t0      1 
    // r_game_motion_vector_field_y            texture    uint          2d             t1      1 
    // r_optical_flow_motion_vector_field_x    texture    uint          2d             t2      1 
    // r_optical_flow_motion_vector_field_y    texture    uint          2d             t3      1 
    // r_disocclusion_mask                     texture  float4          2d             t4      1 
    // r_present_backbuffer                    texture  float4          2d             t5      1 
    // r_inpainting_pyramid                    texture  float4          2d             t6      1 
    // r_current_interpolation_source          texture  float4          2d             t7      1 
    // rw_output                                   UAV  float4          2d             u0      1 
    // cbFI                                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFI" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const char* boundSRVTextureNames[] = { "r_game_motion_vector_field_x", "r_game_motion_vector_field_y", "r_optical_flow_motion_vector_field_x", "r_optical_flow_motion_vector_field_y", "r_disocclusion_mask", "r_present_backbuffer", "r_inpainting_pyramid", "r_current_interpolation_source" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    static const char* boundUAVTextureNames[] = { "rw_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_frameinterpolation_debug_view_pass_16bit_permutations[INVERTED_DEPTH].data
                : g_ffx_frameinterpolation_debug_view_pass_permutations[INVERTED_DEPTH].data,
        is16bit ? g_ffx_frameinterpolation_debug_view_pass_16bit_permutations[INVERTED_DEPTH].size
                : g_ffx_frameinterpolation_debug_view_pass_permutations[INVERTED_DEPTH].size,
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
        0,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        0,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        0,
    };

    return blob;
}

FfxErrorCode frameInterpolationGetPermutationBlobByIndex(FfxFrameInterpolationPass passId,
    FfxBindStage stageId,
    uint32_t permutationOptions, FfxShaderBlob* outBlob)
{

    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit  = FFX_CONTAINS_FLAG(permutationOptions, FRAMEINTERPOLATION_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_AND_DILATE:
        {
            FfxShaderBlob blob = FrameInterpolationGetReconstructAndDilatePermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_SETUP:
        {
            FfxShaderBlob blob = FrameInterpolationGetSetupPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_RECONSTRUCT_PREV_DEPTH:
        {
            FfxShaderBlob blob = FrameInterpolationGetReconstructPrevDepthPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_GAME_MOTION_VECTOR_FIELD:
        {
            FfxShaderBlob blob = FrameInterpolationGetGameMotionVectorFieldPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_OPTICAL_FLOW_VECTOR_FIELD:
        {
            FfxShaderBlob blob = FrameInterpolationGetOpticalFlowVectorFieldPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_DISOCCLUSION_MASK:
        {
            FfxShaderBlob blob = FrameInterpolationGetDisocclusionMaskPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_GAME_VECTOR_FIELD_INPAINTING_PYRAMID:
        {
            FfxShaderBlob blob = FrameInterpolationGetComputeGameVectorFieldInpaintingPyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_INPAINTING_PYRAMID:
        {
            FfxShaderBlob blob = FrameInterpolationGetComputeInpaintingPyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_INTERPOLATION:
        {
            FfxShaderBlob blob = FrameInterpolationGetFiPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_INPAINTING:
        {
            FfxShaderBlob blob = FrameInterpolationGetInpaintingPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FRAMEINTERPOLATION_PASS_DEBUG_VIEW:
        {
            FfxShaderBlob blob = FrameInterpolationGetDebugViewPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
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
