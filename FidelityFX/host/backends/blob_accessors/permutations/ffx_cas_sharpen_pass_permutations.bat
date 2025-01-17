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

for %%a in (_,_16bit_) do (
  call :header %%a
  for %%b in (0,1) do (
    for %%c in (0,1,2,3,4) do (
      call :hlsl %%a %%b %%c
    )
  )
)
goto :eof

:hlsl
setlocal
set file=ffx_cas_sharpen_pass%1permutations_%2_%3.hlsl
if exist %file% goto :eof
echo %file%

call ffx_license.bat %file%
echo #define FFX_GPU 1 >>%file%
if /i %1==_ echo #define FFX_HALF 0 >>%file%
if /i %1==_16bit_ echo #define FFX_HALF 1 >>%file%
echo #define FFX_HLSL 1 >>%file%
echo.>>%file%
echo #define FFX_CAS_OPTION_SHARPEN_ONLY %2 >>%file%
echo #define FFX_CAS_COLOR_SPACE_CONVERSION %3 >>%file%
echo.>>%file%
echo #include "../../hlsl/cas/ffx_cas_sharpen_pass.hlsl">>%file%
goto :eof

:header
setlocal
set file=ffx_cas_sharpen_pass%1permutations.h
if exist %file% goto :eof
echo %file%

call ffx_license %file%
echo typedef unsigned char BYTE;>>%file%
echo.>>%file%
for %%b in (0,1) do (
  for %%c in (0,1,2,3,4) do (
    echo #include "ffx_cas_sharpen_pass%1permutations_%%b_%%c.h">>%file%
  )
)
echo.>>%file%
echo static const struct { const uint8_t* data; uint32_t size; } g_ffx_cas_sharpen_pass%1permutations[2][5] = {>>%file%
for %%b in (0,1) do (
  echo     {>>%file%
  for %%c in (0,1,2,3,4) do (
    echo         {>>%file%
    echo             g_ffx_cas_sharpen_pass%1permutations_%%b_%%c, sizeof^(g_ffx_cas_sharpen_pass%1permutations_%%b_%%c^)>>%file%
    echo         },>>%file%
  )
  echo     },>>%file%
)
echo };>>%file%
goto :eof
