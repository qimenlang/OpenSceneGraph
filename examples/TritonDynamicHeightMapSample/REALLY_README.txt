Triton dynamic height map demo (integrated as an OSG example).

Requirements:
- TRITON_PATH must point to the Triton SDK root (Resources/, Samples/, lib/).
- BUILD_OSG_EXAMPLES=ON and Triton found at configure time (same as TritonOcean).

Runtime:
- Loads %TRITON_PATH%/Samples/terrain.ive and skybox textures from Samples/.
- Executable: example_TritonDynamicHeightMapSample (Debug: TritonDynamicHeightMapSampled.exe).
- Run from the build output directory or ensure Triton DLLs are on PATH.

See examples/TritonOcean for the same CMake/Triton integration pattern.
