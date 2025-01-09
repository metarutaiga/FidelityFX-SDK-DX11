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

#include <FidelityFX/host/ffx_util.h>
#include "ffx_fsr2_shaderblobs.h"
#include "fsr2/ffx_fsr2_private.h"

#include "permutations/ffx_fsr2_autogen_reactive_pass_permutations.h"
#include "permutations/ffx_fsr2_accumulate_pass_permutations.h"
#include "permutations/ffx_fsr2_compute_luminance_pyramid_pass_permutations.h"
#include "permutations/ffx_fsr2_depth_clip_pass_permutations.h"
#include "permutations/ffx_fsr2_lock_pass_permutations.h"
#include "permutations/ffx_fsr2_reconstruct_previous_depth_pass_permutations.h"
#include "permutations/ffx_fsr2_rcas_pass_permutations.h"
#include "permutations/ffx_fsr2_tcr_autogen_pass_permutations.h"

#include "permutations/ffx_fsr2_autogen_reactive_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_accumulate_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_compute_luminance_pyramid_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_depth_clip_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_lock_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_reconstruct_previous_depth_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_rcas_pass_16bit_permutations.h"
#include "permutations/ffx_fsr2_tcr_autogen_pass_16bit_permutations.h"

#include <string.h> // for memset

static FfxShaderBlob fsr2GetTcrAutogenPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                      Type  Format         Dim      HLSL Bind  Count
    // ----------------------------------- ---------- ------- ----------- -------------- ------
    // r_input_opaque_only                    texture  float4          2d             t0      1 
    // r_input_color_jittered                 texture  float4          2d             t1      1 
    // r_input_motion_vectors                 texture  float4          2d             t2      1 
    // r_reactive_mask                        texture   float          2d             t4      1 
    // r_transparency_and_composition_mask    texture   float          2d             t5      1 
    // r_input_prev_color_pre_alpha           texture  float3          2d            t46      1 
    // r_input_prev_color_post_alpha          texture  float3          2d            t47      1 
    // rw_output_autoreactive                 UAV       float          2d             u0      1 
    // rw_output_autocomposition              UAV       float          2d             u1      1 
    // rw_output_prev_color_pre_alpha         UAV      float3          2d             u2      1 
    // rw_output_prev_color_post_alpha        UAV      float3          2d             u3      1 
    // cbFSR2                                 cbuffer      NA          NA            cb0      1 
    // cbGenerateReactive                     cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2", "cbGenerateReactive"};
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0, 0 };
    static const char* boundSRVTextureNames[] = { "r_input_opaque_only", "r_input_color_jittered", "r_input_motion_vectors", "r_reactive_mask", "r_transparency_and_composition_mask", "r_input_prev_color_pre_alpha", "r_input_prev_color_post_alpha" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 4, 5, 46, 47 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_output_autoreactive", "rw_output_autocomposition", "rw_output_prev_color_pre_alpha", "rw_output_prev_color_post_alpha" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_tcr_autogen_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_tcr_autogen_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_tcr_autogen_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_tcr_autogen_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetDepthClipPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    // r_reconstructed_previous_nearest_depth    texture    uint          2d             t0      1 
    // r_dilated_motion_vectors                  texture  float2          2d             t1      1 
    // r_dilatedDepth                            texture   float          2d             t2      1 
    // r_reactive_mask                           texture   float          2d             t3      1 
    // r_transparency_and_composition_mask       texture   float          2d             t4      1 
    // r_previous_dilated_motion_vectors         texture  float2          2d             t5      1 
    // r_input_motion_vectors                    texture  float4          2d             t6      1 
    // r_input_color_jittered                    texture  float4          2d             t7      1 
    // r_input_exposure                          texture  float2          2d             t9      1 
    // rw_dilated_reactive_masks                     UAV  unorm2          2d             u0      1 
    // rw_prepared_input_color                       UAV  float4          2d             u1      1 
    // cbFSR2                                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_reconstructed_previous_nearest_depth", "r_dilated_motion_vectors", "r_dilatedDepth", "r_reactive_mask", "r_transparency_and_composition_mask", "r_previous_dilated_motion_vectors", "r_input_motion_vectors", "r_input_color_jittered", "r_input_exposure" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 9 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_dilated_reactive_masks", "rw_prepared_input_color" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_depth_clip_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_depth_clip_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_depth_clip_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_depth_clip_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetReconstructPreviousDepthPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                          Type  Format         Dim      HLSL Bind  Count
    // --------------------------------------- ---------- ------- ----------- -------------- ------
    // r_input_motion_vectors                     texture  float4          2d             t0      1 
    // r_input_depth                              texture   float          2d             t1      1 
    // r_input_color_jittered                     texture  float4          2d             t2      1 
    // r_input_exposure                           texture  float2          2d             t3      1 
    // rw_reconstructed_previous_nearest_depth        UAV    uint          2d             u0      1 
    // rw_dilated_motion_vectors                      UAV  float2          2d             u1      1 
    // rw_dilatedDepth                                UAV   float          2d             u2      1 
    // rw_lock_input_luma                             UAV   float          2d             u3      1 
    // cbFSR2                                     cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_motion_vectors", "r_input_depth", "r_input_color_jittered","r_input_exposure" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_reconstructed_previous_nearest_depth", "rw_dilated_motion_vectors", "rw_dilatedDepth", "rw_lock_input_luma" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_reconstruct_previous_depth_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_reconstruct_previous_depth_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_reconstruct_previous_depth_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_reconstruct_previous_depth_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetLockPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                          Type  Format         Dim      HLSL Bind  Count
    // --------------------------------------- ---------- ------- ----------- -------------- ------
    // r_lock_input_luma                          texture   float          2d             t0      1 
    // rw_new_locks                                   UAV   unorm          2d             u0      1 
    // rw_reconstructed_previous_nearest_depth        UAV    uint          2d             u1      1 
    // cbFSR2                                     cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_lock_input_luma" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = { "rw_new_locks", "rw_reconstructed_previous_nearest_depth" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_lock_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_lock_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_lock_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_lock_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetAccumulatePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_exposure                  texture  float2          2d             t0      1 
    // r_dilated_reactive_masks          texture  unorm2          2d             t1      1 
    // r_input_motion_vectors            texture  float4          2d             t2      1 
    // r_internal_upscaled_color         texture  float4          2d             t3      1 
    // r_lock_status                     texture  unorm2          2d             t4      1 
    // r_prepared_input_color            texture  float4          2d             t5      1 
    // r_imgMips                         texture   float          2d             t8      1 
    // r_luma_history                    texture  unorm4          2d            t10      1 
    // rw_internal_upscaled_color            UAV  float4          2d             u0      1 
    // rw_lock_status                        UAV  unorm2          2d             u1      1 
    // rw_upscaled_output                    UAV  float4          2d             u2      1 
    // rw_new_locks                          UAV   unorm          2d             u3      1 
    // rw_luma_history                       UAV  float4          2d             u4      1 
    // cbFSR2                            cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_exposure", "r_dilated_reactive_masks", "r_input_motion_vectors", "r_internal_upscaled_color", "r_lock_status", "r_prepared_input_color", "r_imgMips", "r_luma_history" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5, 8, 10 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_internal_upscaled_color", "rw_lock_status", "rw_upscaled_output", "rw_new_locks", "rw_luma_history" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_accumulate_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_accumulate_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_accumulate_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_accumulate_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetRCASPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_exposure                  texture  float2          2d             t0      1 
    // r_rcas_input                      texture  float4          2d             t1      1 
    // rw_upscaled_output                    UAV  float4          2d             u0      1 
    // cbFSR2                            cbuffer      NA          NA            cb0      1 
    // cbRCAS                            cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2", "cbRCAS" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0, 0 };
    static const char* boundSRVTextureNames[] = { "r_input_exposure", "r_rcas_input" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_upscaled_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_rcas_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_rcas_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_rcas_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_rcas_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetComputeLuminancePyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_color_jittered            texture  float4          2d             t0      1 
    // rw_spd_global_atomic                  UAV    uint          2d             u0      1 
    // rw_img_mip_shading_change             UAV   float          2d             u1      1 
    // rw_img_mip_5                          UAV   float          2d             u2      1 
    // rw_auto_exposure                      UAV  float2          2d             u3      1 
    // cbFSR2                            cbuffer      NA          NA            cb0      1 
    // cbSPD                             cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR2", "cbSPD" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0, 0 };
    static const char* boundSRVTextureNames[] = { "r_input_color_jittered" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = {  "rw_spd_global_atomic", "rw_img_mip_shading_change", "rw_img_mip_5", "rw_auto_exposure" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_compute_luminance_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_compute_luminance_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_compute_luminance_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_compute_luminance_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

static FfxShaderBlob fsr2GetAutogenReactivePassPermutationBlobByIndex(
    uint32_t permutationOptions,
    bool isWave64,
    bool is16bit) {
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_opaque_only               texture  float4          2d             t0      1 
    // r_input_color_jittered            texture  float4          2d             t1      1 
    // rw_output_autoreactive                UAV   float          2d             u0      1 
    // cbGenerateReactive                cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbGenerateReactive" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_opaque_only", "r_input_color_jittered" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_output_autoreactive" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr2_autogen_reactive_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr2_autogen_reactive_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr2_autogen_reactive_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr2_autogen_reactive_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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
        boundConstantBufferSpaces,
        boundSRVTextureNames,
        boundSRVTextures,
        boundSRVTextureCounts,
        boundSRVTextureSpaces,
        boundUAVTextureNames,
        boundUAVTextures,
        boundUAVTextureCounts,
        boundUAVTextureSpaces,
    };

    return blob;
}

FfxErrorCode fsr2GetPermutationBlobByIndex(
    FfxFsr2Pass passId,
    uint32_t permutationOptions,
    FfxShaderBlob* outBlob) {

    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_FSR2_PASS_DEPTH_CLIP:
        {
            FfxShaderBlob blob = fsr2GetDepthClipPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_RECONSTRUCT_PREVIOUS_DEPTH:
        {
            FfxShaderBlob blob = fsr2GetReconstructPreviousDepthPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_LOCK:
        {
            FfxShaderBlob blob = fsr2GetLockPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_ACCUMULATE:
        case FFX_FSR2_PASS_ACCUMULATE_SHARPEN:
        {
            FfxShaderBlob blob = fsr2GetAccumulatePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_RCAS:
        {
            FfxShaderBlob blob = fsr2GetRCASPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_COMPUTE_LUMINANCE_PYRAMID:
        {
            FfxShaderBlob blob = fsr2GetComputeLuminancePyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_GENERATE_REACTIVE:
        {
            FfxShaderBlob blob = fsr2GetAutogenReactivePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR2_PASS_TCR_AUTOGENERATE:
        {
            FfxShaderBlob blob = fsr2GetTcrAutogenPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
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

FfxErrorCode fsr2IsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFX_CONTAINS_FLAG(permutationOptions, FSR2_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFX_OK;
}
