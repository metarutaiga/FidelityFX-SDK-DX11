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
#include "ffx_fsr3upscaler_shaderblobs.h"
#include "fsr3upscaler/ffx_fsr3upscaler_private.h"

#include "permutations/ffx_fsr3upscaler_autogen_reactive_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_accumulate_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_luma_pyramid_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_prepare_reactivity_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_prepare_inputs_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_shading_change_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_rcas_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_shading_change_pyramid_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_luma_instability_pass_permutations.h"
#include "permutations/ffx_fsr3upscaler_debug_view_pass_permutations.h"

#include "permutations/ffx_fsr3upscaler_autogen_reactive_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_accumulate_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_luma_pyramid_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_prepare_reactivity_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_prepare_inputs_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_shading_change_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_rcas_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_shading_change_pyramid_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_luma_instability_pass_16bit_permutations.h"
#include "permutations/ffx_fsr3upscaler_debug_view_pass_16bit_permutations.h"

#include <string.h> // for memset

static FfxShaderBlob fsr3UpscalerGetPrepareReactivityPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                         Type  Format         Dim      HLSL Bind  Count
    // -------------------------------------- ---------- ------- ----------- -------------- ------
    // r_reconstructed_previous_nearest_depth    texture    uint          2d             t0      1 
    // r_dilated_motion_vectors                  texture  float2          2d             t1      1 
    // r_dilated_depth                           texture   float          2d             t2      1 
    // r_reactive_mask                           texture   float          2d             t3      1 
    // r_transparency_and_composition_mask       texture   float          2d             t4      1 
    // r_accumulation                            texture   float          2d             t5      1 
    // r_shading_change                          texture   float          2d             t6      1 
    // r_current_luma                            texture   float          2d             t7      1 
    // r_input_exposure                          texture  float2          2d             t8      1 
    // rw_dilated_reactive_masks                     UAV  unorm4          2d             u0      1 
    // rw_new_locks                                  UAV   unorm          2d             u1      1 
    // rw_accumulation                               UAV   float          2d             u2      1 
    // cbFSR3Upscaler                            cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_reconstructed_previous_nearest_depth", "r_dilated_motion_vectors", "r_dilated_depth", "r_reactive_mask", "r_transparency_and_composition_mask", "r_accumulation", "r_shading_change", "r_current_luma", "r_input_exposure" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_dilated_reactive_masks", "rw_new_locks", "rw_accumulation" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_prepare_reactivity_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_prepare_reactivity_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_prepare_reactivity_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_prepare_reactivity_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetShadingChangePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_spd_mips                        texture  float2          2d             t0      1 
    // rw_shading_change                     UAV   float          2d             u0      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_spd_mips" };
    static const uint32_t boundSRVTextures[] = { 0 };
    static const uint32_t boundSRVTextureCounts[] = { 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0 };
    static const char* boundUAVTextureNames[] = { "rw_shading_change" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_shading_change_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_shading_change_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_shading_change_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_shading_change_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetPrepareInputsPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                          Type  Format         Dim      HLSL Bind  Count
    // --------------------------------------- ---------- ------- ----------- -------------- ------
    // r_input_motion_vectors                     texture  float4          2d             t0      1 
    // r_input_depth                              texture   float          2d             t1      1 
    // r_input_color_jittered                     texture  float4          2d             t2      1 
    // rw_dilated_motion_vectors                      UAV  float2          2d             u0      1 
    // rw_dilated_depth                               UAV   float          2d             u1      1 
    // rw_reconstructed_previous_nearest_depth        UAV    uint          2d             u2      1 
    // rw_farthest_depth                              UAV   float          2d             u3      1 
    // rw_current_luma                                UAV   float          2d             u4      1 
    // cbFSR3Upscaler                             cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_motion_vectors", "r_input_depth", "r_input_color_jittered" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_dilated_motion_vectors", "rw_dilated_depth", "rw_reconstructed_previous_nearest_depth", "rw_farthest_depth", "rw_current_luma" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_prepare_inputs_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_prepare_inputs_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_prepare_inputs_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_prepare_inputs_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetAccumulatePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_exposure                  texture  float2          2d             t0      1 
    // r_dilated_reactive_masks          texture  unorm4          2d             t1      1 
    // r_input_motion_vectors            texture  float4          2d             t2      1 
    // r_internal_upscaled_color         texture  float4          2d             t3      1 
    // r_farthest_depth_mip1             texture   float          2d             t5      1 
    // r_luma_instability                texture   float          2d             t7      1 
    // r_input_color_jittered            texture  float4          2d             t8      1 
    // rw_internal_upscaled_color            UAV  float4          2d             u0      1 
    // rw_upscaled_output                    UAV  float4          2d             u1      1 
    // rw_new_locks                          UAV   unorm          2d             u2      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_exposure", "r_dilated_reactive_masks", "r_input_motion_vectors", "r_internal_upscaled_color", "r_farthest_depth_mip1", "r_luma_instability", "r_input_color_jittered" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3, 5, 7, 8 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_internal_upscaled_color", "rw_upscaled_output", "rw_new_locks" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_accumulate_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_accumulate_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_accumulate_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_accumulate_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetRCASPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_exposure                  texture  float2          2d             t0      1 
    // r_rcas_input                      texture  float4          2d             t1      1 
    // rw_upscaled_output                    UAV  float4          2d             u0      1 
    // cbRCAS                            cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_exposure", "r_rcas_input" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_upscaled_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_rcas_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_rcas_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_rcas_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_rcas_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetLumaPyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_current_luma                    texture   float          2d             t0      1 
    // r_farthest_depth                  texture   float          2d             t1      1 
    // rw_spd_global_atomic                  UAV    uint          2d             u0      1 
    // rw_frame_info                         UAV  float4          2d             u1      1 
    // rw_spd_mip5                           UAV  float2          2d             u7      1 
    // rw_farthest_depth_mip1                UAV   float          2d             u8      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    // cbSPD                             cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler", "cbSPD" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0, 0 };
    static const char* boundSRVTextureNames[] = { "r_current_luma", "r_farthest_depth" };
    static const uint32_t boundSRVTextures[] = { 0, 1 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_spd_global_atomic", "rw_frame_info", "rw_spd_mip5", "rw_farthest_depth_mip1" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 7, 8 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_luma_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_luma_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_luma_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_luma_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetAutogenReactivePassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_opaque_only               texture  float4          2d             t0      1 
    // r_input_color_jittered            texture  float4          2d             t1      1 
    // rw_output_autoreactive                UAV   float          2d             u0      1 
    // cbGenerateReactive                cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbGenerateReactive" };
    static const uint32_t boundConstantBuffers[] = { 1 };
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
        is16bit ? g_ffx_fsr3upscaler_autogen_reactive_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_autogen_reactive_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_autogen_reactive_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_autogen_reactive_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetShadingChangePyramidPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_current_luma                    texture   float          2d             t0      1 
    // r_previous_luma                   texture   float          2d             t1      1 
    // r_dilated_motion_vectors          texture  float2          2d             t2      1 
    // r_input_exposure                  texture  float2          2d             t3      1 
    // rw_spd_global_atomic                  UAV    uint          2d             u0      1 
    // rw_spd_mip0                           UAV  float2          2d             u1      1 
    // rw_spd_mip1                           UAV  float2          2d             u2      1 
    // rw_spd_mip2                           UAV  float2          2d             u3      1 
    // rw_spd_mip3                           UAV  float2          2d             u4      1 
    // rw_spd_mip4                           UAV  float2          2d             u5      1 
    // rw_spd_mip5                           UAV  float2          2d             u6      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    // cbSPD                             cbuffer      NA          NA            cb1      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler", "cbSPD" };
    static const uint32_t boundConstantBuffers[] = { 0, 1 };
    static const uint32_t boundConstantBufferCounts[] = { 1, 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0, 0 };
    static const char* boundSRVTextureNames[] = { "r_current_luma", "r_previous_luma", "r_dilated_motion_vectors", "r_input_exposure" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_spd_global_atomic", "rw_spd_mip0", "rw_spd_mip1", "rw_spd_mip2", "rw_spd_mip3", "rw_spd_mip4", "rw_spd_mip5" };
    static const uint32_t boundUAVTextures[] = { 0, 1, 2, 3, 4, 5, 6 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1, 1, 1, 1, 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0, 0, 0, 0, 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_shading_change_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_shading_change_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_shading_change_pyramid_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_shading_change_pyramid_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetLumaInstabilityPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_input_exposure                  texture  float2          2d             t0      1 
    // r_dilated_reactive_masks          texture  unorm4          2d             t1      1 
    // r_dilated_motion_vectors          texture  float2          2d             t2      1 
    // r_luma_history                    texture  float4          2d             t4      1 
    // r_current_luma                    texture   float          2d             t6      1 
    // rw_luma_history                       UAV  float4          2d             u0      1 
    // rw_luma_instability                   UAV   float          2d             u1      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_input_exposure", "r_dilated_reactive_masks", "r_dilated_motion_vectors", "r_luma_history", "r_current_luma" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 4, 6 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_luma_history", "rw_luma_instability" };
    static const uint32_t boundUAVTextures[] = { 0, 1 };
    static const uint32_t boundUAVTextureCounts[] = { 1, 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0, 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_luma_instability_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_luma_instability_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_luma_instability_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_luma_instability_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

static FfxShaderBlob fsr3UpscalerGetDebugViewPassPermutationBlobByIndex(uint32_t permutationOptions, bool isWave64, bool is16bit)
{
    int REPROJECT_USE_LANCZOS_TYPE = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_USE_LANCZOS_TYPE);
    int HDR_COLOR_INPUT = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_HDR_COLOR_INPUT);
    int LOW_RESOLUTION_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_LOW_RES_MOTION_VECTORS);
    int JITTERED_MOTION_VECTORS = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_JITTER_MOTION_VECTORS);
    int INVERTED_DEPTH = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_DEPTH_INVERTED);
    int APPLY_SHARPENING = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ENABLE_SHARPENING);

    // Resource Bindings:
    //
    // Name                                 Type  Format         Dim      HLSL Bind  Count
    // ------------------------------ ---------- ------- ----------- -------------- ------
    // r_dilated_reactive_masks          texture  unorm4          2d             t0      1 
    // r_dilated_motion_vectors          texture  float2          2d             t1      1 
    // r_dilated_depth                   texture   float          2d             t2      1 
    // r_internal_upscaled_color         texture  float4          2d             t3      1 
    // rw_upscaled_output                    UAV  float4          2d             u0      1 
    // cbFSR3Upscaler                    cbuffer      NA          NA            cb0      1 
    static const char* boundConstantBufferNames[] = { "cbFSR3Upscaler" };
    static const uint32_t boundConstantBuffers[] = { 0 };
    static const uint32_t boundConstantBufferCounts[] = { 1 };
    static const uint32_t boundConstantBufferSpaces[] = { 0 };
    static const char* boundSRVTextureNames[] = { "r_dilated_reactive_masks", "r_dilated_motion_vectors", "r_dilated_depth", "r_internal_upscaled_color" };
    static const uint32_t boundSRVTextures[] = { 0, 1, 2, 3 };
    static const uint32_t boundSRVTextureCounts[] = { 1, 1, 1, 1 };
    static const uint32_t boundSRVTextureSpaces[] = { 0, 0, 0, 0 };
    static const char* boundUAVTextureNames[] = { "rw_upscaled_output" };
    static const uint32_t boundUAVTextures[] = { 0 };
    static const uint32_t boundUAVTextureCounts[] = { 1 };
    static const uint32_t boundUAVTextureSpaces[] = { 0 };

    FfxShaderBlob blob = {
        is16bit ? g_ffx_fsr3upscaler_debug_view_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data
                : g_ffx_fsr3upscaler_debug_view_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].data,
        is16bit ? g_ffx_fsr3upscaler_debug_view_pass_16bit_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size
                : g_ffx_fsr3upscaler_debug_view_pass_permutations[REPROJECT_USE_LANCZOS_TYPE][HDR_COLOR_INPUT][LOW_RESOLUTION_MOTION_VECTORS][JITTERED_MOTION_VECTORS][INVERTED_DEPTH][APPLY_SHARPENING].size,
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

FfxErrorCode fsr3UpscalerGetPermutationBlobByIndex(
    FfxFsr3UpscalerPass passId,
    uint32_t permutationOptions,
    FfxShaderBlob* outBlob) {

    bool isWave64 = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_FORCE_WAVE64);
    bool is16bit = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_ALLOW_FP16);

    switch (passId) {

        case FFX_FSR3UPSCALER_PASS_PREPARE_INPUTS:
        {
            FfxShaderBlob blob = fsr3UpscalerGetPrepareInputsPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_PREPARE_REACTIVITY:
        {
            FfxShaderBlob blob = fsr3UpscalerGetPrepareReactivityPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_SHADING_CHANGE:
        {
            FfxShaderBlob blob = fsr3UpscalerGetShadingChangePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_ACCUMULATE:
        case FFX_FSR3UPSCALER_PASS_ACCUMULATE_SHARPEN:
        {
            FfxShaderBlob blob = fsr3UpscalerGetAccumulatePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_RCAS:
        {
            FfxShaderBlob blob = fsr3UpscalerGetRCASPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_LUMA_PYRAMID:
        {
            FfxShaderBlob blob = fsr3UpscalerGetLumaPyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_GENERATE_REACTIVE:
        {
            FfxShaderBlob blob = fsr3UpscalerGetAutogenReactivePassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_SHADING_CHANGE_PYRAMID:
        {
            FfxShaderBlob blob = fsr3UpscalerGetShadingChangePyramidPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_LUMA_INSTABILITY:
        {
            FfxShaderBlob blob = fsr3UpscalerGetLumaInstabilityPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
            memcpy(outBlob, &blob, sizeof(FfxShaderBlob));
            return FFX_OK;
        }

        case FFX_FSR3UPSCALER_PASS_DEBUG_VIEW:
        {
            FfxShaderBlob blob = fsr3UpscalerGetDebugViewPassPermutationBlobByIndex(permutationOptions, isWave64, is16bit);
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

FfxErrorCode fsr3UpscalerIsWave64(uint32_t permutationOptions, bool& isWave64)
{
    isWave64 = FFX_CONTAINS_FLAG(permutationOptions, FSR3UPSCALER_SHADER_PERMUTATION_FORCE_WAVE64);
    return FFX_OK;
}
