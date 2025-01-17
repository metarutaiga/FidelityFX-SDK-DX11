FidelifyFX for DX11
===

Task
---
- [x] Support for Shader Model 5.0
- [x] Patch GroupMemoryBarrier to GroupMemoryBarrierWithGroupSync
- [x] 16bit SPD for FSR3 Frame Interpolation
- [x] 16bit SPD for FSR3 Optical Flow
- [x] AMD Wave Intrinsics
- [x] Intel Wave Intrinsics
- [x] NVIDIA Wave Intrinsics

Patch
---
FidelityFX\gpu\ffx_core_hlsl.h
```diff
+#if FFX_HLSL_SM < 60
 #define FFX_GROUP_MEMORY_BARRIER GroupMemoryBarrier
+#else
+#define FFX_GROUP_MEMORY_BARRIER GroupMemoryBarrierWithGroupSync
+#endif

 /// A define for abstracting compute atomic additions between shading languages.
 ///
 /// @ingroup HLSLCore
 #define FFX_ATOMIC_ADD(x, y) InterlockedAdd(x, y)
+
+/// A define for abstracting compute atomic minimums between shading languages.
+///
+/// @ingroup HLSLCore
+#define FFX_ATOMIC_MIN(x, y) InterlockedMin(x, y)
```

FidelityFX\gpu\frameinterpolation\ffx_frameinterpolation_common.h
```diff
     vfElement.bInPainted      = false;
+    vfElement.fVelocity       = 0.0f;
+    vfElement.bNegOutside     = false;
+    vfElement.bPosOutside     = false;
 }
```

FidelityFX\gpu\frameinterpolation\ffx_frameinterpolation_game_motion_vector_field.h
```diff
-                bWriteSecondary &= IsOnScreen(iSamplePos, RenderSize());
+                bWriteSecondary = bWriteSecondary & IsOnScreen(iSamplePos, RenderSize());

                 if (bWriteSecondary)
                 {
                     const FfxUInt32 uExistingVectorFieldEntry = UpdateGameMotionVectorFieldEx(iSamplePos, packedVectorSecondary);

                     uNumPrimaryHits += PackedVectorFieldEntryIsPrimary(uExistingVectorFieldEntry);
-                    bWriteSecondary &= (uNumPrimaryHits <= 3);
+                    bWriteSecondary = bWriteSecondary & (uNumPrimaryHits <= 3);
```

FidelityFX\gpu\opticalflow\ffx_opticalflow_compute_optical_flow_v5.h
```diff
+#if defined(FFX_SPD_NO_WAVE_OPERATIONS)
+    FfxInt32 waveId = iLocalIndex >> 5u;
+    FFX_ATOMIC_ADD(sWaveSad[waveId], blockSadSum);
+    FFX_GROUP_MEMORY_BARRIER();
+    blockSadSum = sWaveSad[waveId] + sWaveSad[waveId ^ 1];
+#else
     blockSadSum = WaveActiveSum(blockSadSum);

     if (WaveGetLaneCount() == 32)
     {
         FfxInt32 waveId = iLocalIndex >> 5u;
         if (WaveIsFirstLane())
         {
             sWaveSad[waveId] = blockSadSum;
         }
         FFX_GROUP_MEMORY_BARRIER();
         blockSadSum += sWaveSad[waveId ^ 1];
     }
+#endif
```
```diff
+#if defined(FFX_SPD_NO_WAVE_OPERATIONS)
+    FfxInt32 waveId = iLocalIndex >> 5u;
+    FFX_ATOMIC_MIN(sWaveMin[waveId], min0123);
+    FFX_GROUP_MEMORY_BARRIER();
+    min0123 = ffxMin(sWaveMin[waveId], sWaveMin[waveId ^ 1]);
+#else
     min0123 = WaveActiveMin(min0123);

     if (WaveGetLaneCount() == 32)
     {
         FfxInt32 waveId = iLocalIndex >> 5u;

         if (WaveIsFirstLane())
         {
             sWaveMin[waveId] = min0123;
         }
         FFX_GROUP_MEMORY_BARRIER();
         min0123 = ffxMin(min0123, sWaveMin[waveId ^ 1]);
     }
+#endif
```
```diff
     sadMapBuffer[3][iSearchId.y][iSearchId.x] = (qsad.w << 16) | EncodeSearchCoord(FfxInt32x2(iSearchId.x * 4 + 3, iSearchId.y));
+#if defined(FFX_SPD_NO_WAVE_OPERATIONS)
+    sWaveSad[0] = 0;
+    sWaveSad[1] = 0;
+    sWaveMin[0] = 0xffffffffu;
+    sWaveMin[1] = 0xffffffffu;
+#endif
     FFX_GROUP_MEMORY_BARRIER();
```
```diff
-        if ((iSearchId.y & 0b111) == 0 && (iSearchId.x & 0b1) == 0)
+        if ((iSearchId.y & 7) == 0 && (iSearchId.x & 1) == 0)
```
```diff
-                pixels[iSearchId.y & 0b111][iSearchId.x & 0b1] = packedLuma_4blocks;
+                pixels[iSearchId.y & 7][iSearchId.x & 1] = packedLuma_4blocks;
```
