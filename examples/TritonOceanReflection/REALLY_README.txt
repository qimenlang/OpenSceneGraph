MAKE SURE THE ENVIRONMENT VARIABLE TRITON_PATH IS DEFINED to point to the directory containing the Triton SDK. 

Under Windows, this is set automatically by the SDK installer.

To build it, you must first set the OSG_DIR environment variable to the root of your OpenSceneGraph installation. Run cmake 2.6 or newer on CMakeLists.txt, and the makefiles for your compiler will be generated. Be sure to set your working directory to the binary directory of your OSG installation when running. This code is built for OSG 3.6.0.

If you build for 64-bit Windows, make sure any DLL dependencies (such as d3dcompiler47.dll) are the 64-bit and not 32-bit versions - the ones included here are 32-bit. If you load a 32-bit DLL from a 64-bit Windows application, it will just crash with no explanation of why.

As always, if you run into any trouble, don't hesitate to contact support@sundog-soft.com for assistance.
