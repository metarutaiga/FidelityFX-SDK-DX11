@echo off
rem This file is part of the FidelityFX SDK.
rem
rem Copyright (C) 2024 Advanced Micro Devices, Inc.
rem 
rem Permission is hereby granted, free of charge, to any person obtaining a copy
rem of this software and associated documentation files(the "Software"), to deal
rem in the Software without restriction, including without limitation the rights
rem to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
rem copies of the Software, and to permit persons to whom the Software is
rem furnished to do so, subject to the following conditions :
rem
rem The above copyright notice and this permission notice shall be included in
rem all copies or substantial portions of the Software.
rem
rem THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
rem IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
rem FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
rem AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
rem LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
rem OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
rem THE SOFTWARE.

for %%a in (_compute_luminance_pyramid_,_compute_optical_flow_advanced_) do (
  for %%b in (pass,pass_v5) do (
    if exist ../../hlsl/opticalflow/ffx_opticalflow%%a%%b.hlsl (
      for %%c in (_,_16bit_) do (
        call :header %%a %%b %%c
        for %%d in (0,1) do (
          call :hlsl %%a %%b %%c %%d
        )
      )
    )
  )
)
goto :eof

:hlsl
setlocal
set file=intel_opticalflow%1%2%3permutations_%4.hlsl
if exist %file% goto :eof
echo %file%

call ..\permutations\ffx_license.bat %file%
echo #define FFX_GPU 1 >>%file%
if /i %3==_ echo #define FFX_HALF 0 >>%file%
if /i %3==_16bit_ echo #define FFX_HALF 1 >>%file%
echo #define FFX_HLSL 1 >>%file%
echo.>>%file%
echo #define FFX_OPTICALFLOW_OPTION_HDR_COLOR_INPUT %4 >>%file%
echo.>>%file%
echo #include "ffx_wave_intel.h">>%file%
echo #include "ffx_common_types.h">>%file%
echo #define FFX_CONCAT(a, b) a ## b>>%file%
echo #define MapThreads FFX_CONCAT(MapThreads_, __LINE__)>>%file%
echo #define NumWorkGroups FFX_CONCAT(NumWorkGroups_, __LINE__)>>%file%
echo FfxUInt32 NumWorkGroups_90();>>%file%
echo FfxUInt32 NumWorkGroups_147()>>%file%
echo {>>%file%
echo     IntelExt_Init();>>%file%
echo     return NumWorkGroups_90();>>%file%
echo }>>%file%
echo FfxUInt32 NumWorkGroups_155()>>%file%
echo {>>%file%
echo     IntelExt_Init();>>%file%
echo     return NumWorkGroups_90();>>%file%
echo }>>%file%
echo void MapThreads_219(in FfxInt32x2 iGroupId, in FfxInt32 iLocalIndex, out FfxInt32x2 iSearchId, out FfxInt32x2 iPxPos, out FfxInt32 iLaneToBlockId);>>%file%
echo void MapThreads_232(in FfxInt32x2 iGroupId, in FfxInt32 iLocalIndex, out FfxInt32x2 iSearchId, out FfxInt32x2 iPxPos, out FfxInt32 iLaneToBlockId)>>%file%
echo {>>%file%
echo     IntelExt_Init();>>%file%
echo     MapThreads_219(iGroupId, iLocalIndex, iSearchId, iPxPos, iLaneToBlockId);>>%file%
echo }>>%file%
echo #include "../../hlsl/opticalflow/ffx_opticalflow%1%2.hlsl">>%file%
goto :eof

:header
setlocal
set file=intel_opticalflow%1%2%3permutations.h
if exist %file% goto :eof
echo %file%

call ..\permutations\ffx_license.bat %file%
echo typedef unsigned char BYTE;>>%file%
echo.>>%file%
for %%c in (0,1) do (
  echo #include "intel_opticalflow%1%2%3permutations_%%c.h">>%file%
)
echo.>>%file%
echo static const struct { const uint8_t* data; uint32_t size; } g_intel_opticalflow%1%2%3permutations[2] = {>>%file%
for %%c in (0,1) do (
  echo     {>>%file%
  echo         g_intel_opticalflow%1%2%3permutations_%%c, sizeof^(g_intel_opticalflow%1%2%3permutations_%%c^)>>%file%
  echo     },>>%file%
)
echo };>>%file%
goto :eof
