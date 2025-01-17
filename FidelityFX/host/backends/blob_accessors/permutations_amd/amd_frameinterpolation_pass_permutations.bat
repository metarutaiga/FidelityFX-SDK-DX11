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

for %%a in (_compute_game_vector_field_inpainting_pyramid_,_compute_inpainting_pyramid_) do (
  for %%b in (_,_16bit_) do (
    call :header %%a %%b
    for %%c in (0,1) do (
      call :hlsl %%a %%b %%c
    )
  )
)
goto :eof

:hlsl
setlocal
set file=amd_frameinterpolation%1pass%2permutations_%3.hlsl
if exist %file% goto :eof
echo %file%

call ..\permutations\ffx_license.bat %file%
echo #define FFX_GPU 1 >>%file%
if /i %2==_ echo #define FFX_HALF 0 >>%file%
if /i %2==_16bit_ echo #define FFX_HALF 1 >>%file%
echo #define FFX_HLSL 1 >>%file%
echo.>>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF 0 >>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF 0 >>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF 1 >>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF 0 >>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_USE_LANCZOS_TYPE 2 >>%file%
echo.>>%file%
echo #define FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH %3 >>%file%
echo.>>%file%
echo #include "ffx_wave_amd.h">>%file%
echo #include "../../hlsl/frameinterpolation/ffx_frameinterpolation%1pass.hlsl">>%file%
goto :eof

:header
setlocal
set file=amd_frameinterpolation%1pass%2permutations.h
if exist %file% goto :eof
echo %file%

call ..\permutations\ffx_license.bat %file%
echo typedef unsigned char BYTE;>>%file%
echo.>>%file%
for %%c in (0,1) do (
  echo #include "amd_frameinterpolation%1pass%2permutations_%%c.h">>%file%
)
echo.>>%file%
echo static const struct { const uint8_t* data; uint32_t size; } g_amd_frameinterpolation%1pass%2permutations[2] = {>>%file%
for %%c in (0,1) do (
  echo     {>>%file%
  echo         g_amd_frameinterpolation%1pass%2permutations_%%c, sizeof^(g_amd_frameinterpolation%1pass%2permutations_%%c^)>>%file%
  echo     },>>%file%
)
echo };>>%file%
goto :eof
