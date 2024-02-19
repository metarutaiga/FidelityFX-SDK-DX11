@echo off
rem This file is part of the FidelityFX SDK.
rem 
rem Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
rem 
rem Permission is hereby granted, free of charge, to any person obtaining a copy
rem of this software and associated documentation files (the "Software"), to deal
rem in the Software without restriction, including without limitation the rights
rem to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
rem copies of the Software, and to permit persons to whom the Software is
rem furnished to do so, subject to the following conditions:
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

for %%a in (_easu_,_rcas_) do (
  for %%b in (_,_16bit_) do (
    call :header %%a %%b
    for %%c in (0,1) do (
      for %%d in (0,1) do (
        for %%e in (0,1) do (
          for %%f in (0,1) do (
            call :hlsl %%a %%b %%c %%d %%e %%f
          )
        )
      )
    )
  )
)
goto :eof

:hlsl
setlocal
set file=ffx_fsr1%1pass%2permutations_%3_%4_%5_%6.hlsl
if exist %file% goto :eof
echo %file%

call ffx_license.bat %file%
echo #define FFX_GPU 1 >>%file%
if /i %2==_ echo #define FFX_HALF 0 >>%file%
if /i %2==_16bit_ echo #define FFX_HALF 1 >>%file%
echo #define FFX_HLSL 1 >>%file%
echo.>>%file%
echo #define FFX_FSR1_OPTION_APPLY_RCAS %3 >>%file%
echo #define FFX_FSR1_OPTION_RCAS_PASSTHROUGH_ALPHA %4 >>%file%
echo #define FFX_FSR1_OPTION_RCAS_DENOISE %5 >>%file%
echo #define FFX_FSR1_OPTION_SRGB_CONVERSIONS %6 >>%file%
echo.>>%file%
echo #include "../../hlsl/fsr1/ffx_fsr1%1pass.hlsl">>%file%
goto :eof

:header
setlocal
set file=ffx_fsr1%1pass%2permutations.h
if exist %file% goto :eof
echo %file%

call ffx_license.bat %file%
echo typedef unsigned char BYTE;>>%file%
echo.>>%file%
for %%c in (0,1) do (
  for %%d in (0,1) do (
    for %%e in (0,1) do (
      for %%f in (0,1) do (
        echo #include "ffx_fsr1%1pass%2permutations_%%c_%%d_%%e_%%f.h">>%file%
      )
    )
  )
)
echo.>>%file%
echo static const struct { const uint8_t* data; uint32_t size; } g_ffx_fsr1%1pass%2permutations[2][2][2][2] = {>>%file%
for %%c in (0,1) do (
  echo     {>>%file%
  for %%d in (0,1) do (
    echo         {>>%file%
    for %%e in (0,1) do (
      echo             {>>%file%
      for %%f in (0,1) do (
        echo                 {>>%file%
        echo                     g_ffx_fsr1%1pass%2permutations_%%c_%%d_%%e_%%f, sizeof^(g_ffx_fsr1%1pass%2permutations_%%c_%%d_%%e_%%f^)>>%file%
        echo                 },>>%file%
      )
      echo             },>>%file%
    )
    echo         },>>%file%
  )
  echo     },>>%file%
)
echo };>>%file%
goto :eof
