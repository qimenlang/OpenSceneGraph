MAKE SURE THE ENVIRONMENT VARIABLE TRITON_PATH IS DEFINED to point to the directory containing the Triton SDK. 

Under Windows, this is set automatically by the SDK installer.

To build it, you must first set the OSG_DIR environment variable to the root of your OpenSceneGraph installation. Run cmake 2.6 or newer on CMakeLists.txt, and the makefiles for your compiler will be generated. Be sure to set your working directory to the binary directory of your OSG installation when running. This code is built for OSG 3.6.0.

The application will try to load a file terrain.ive inside the samples folder, to illustrate the dynamic creation of height maps passed to Triton for smooth coastal blending. You'll need to modify main.cpp to point to your own terrain model if you wish.

In the lower left corner will be an inset view of the height map created for your scene. It will cover an area 512x512 surrounding the camera is it's implemented in this example. As you get to within 512 meters of a coastline, you should see smooth blending of the water and coastline, assuming your terrain includes bathymetry data. Even if it doesn't, the height map will be used to ensure water does not appear where terrain exists.

This sample also includes an example of setting up logarithmic depth buffers. See the comments at the top of TritonDrawable.h for further guidance.

If you build for 64-bit Windows, make sure any DLL dependencies (such as d3dcompiler47.dll) are the 64-bit and not 32-bit versions - the ones included here are 32-bit. If you load a 32-bit DLL from a 64-bit Windows application, it will just crash with no explanation of why.

As always, if you run into any trouble, don't hesitate to contact support@sundog-soft.com for assistance.
