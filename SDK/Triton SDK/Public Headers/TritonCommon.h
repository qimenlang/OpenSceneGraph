// Copyright (c) 2011-2014 Sundog Software LLC. All rights reserved worldwide.

/*! \file TritonCommon.h
    \brief Common typedefs and defines used within Triton.
*/

#ifndef TRITONCOMMON_H
#define TRITONCOMMON_H

#include "MemAlloc.h"
#include <stdlib.h>
#include <string>
#include <limits.h>

#define TRITON_PI 3.14159265
#define TRITON_TWOPI (TRITON_PI * 2.0)
#define TRITON_SQRT2 1.414213562373095

#define TRITON_PIF 3.14159265f
#define TRITON_TWOPIF (TRITON_PIF * 2.0f)
#define TRITON_SQRT2F 1.414213562373095f

#define RADIANS(x) ( (x) * (TRITON_PI / 180.0) )
#define DEGREES(x) ( (x) * (180.0 / TRITON_PI ) )

#define TRITON_MIN(a,b) (((a) < (b)) ? (a) : (b))

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct _D3DXMACRO;
struct IDirect3DDevice9;

namespace Triton
{

class Environment;
class Stream;

#if _WIN32
#if (_MSC_VER >= 1900)
#pragma warning (disable: 4302 4311 4312)
#endif
/** A renderer-agnostic handle for a shader. */
typedef void * ShaderHandle;
/** A renderer-agnostic handle for a texture. */
typedef void * TextureHandle;
/** A renderer-agnostic handle for a decal. */
typedef void * DecalHandle;
/** A renderer-agnostic handle for a buffer*/
typedef void * BufferHandle;
#else
/** A renderer-agnostic handle for a shader. */
typedef int ShaderHandle;
/** A renderer-agnostic handle for a texture. */
typedef int TextureHandle;
/** A renderer-agnostic handle for a decal. */
typedef void * DecalHandle;
/** A renderer-agnostic handle for a buffer*/
typedef void * BufferHandle;
#endif

/** A collection of static utility methods used by Triton. */
class Utils
{
public:

    /** Allows the client to plug in a routine which will be called to
        print/log DebugMsg() calls when "enable-debug-messages" is set,
        instead of using the internal default logging behavior. */
    static void TRITONAPI SetDebugMsgCB( void (*debug_msg_cb)( const char * ));

    /** Prints out an error message to the debugger's output window only if the
        setting enable-debug-messages is set to 'yes' in resources/Triton.config */
    static void TRITONAPI DebugMsg(const char *debugMessage);

    /** Clears any existing OpenGL error codes so we may test for new ones. */
    static void TRITONAPI ClearGLErrors(Stream* stream = 0);

    /** Prints out (and clears) any existing GL errors using Utils::DebugMsg
        \return True if no error code was present. */
    static bool TRITONAPI PrintGLErrors(const char *file, int line, Stream* stream = 0);

    /** Returns the resources subdirectory where our FFT implementation DLL's live
        for this specific build's compiler version and platform. */
    static TRITON_STRING TRITONAPI GetDLLPath() {
        TRITON_STRING path;
#ifdef _WIN32

#if _MSC_VER >= 1950
        path += "vc145\\";
#elif _MSC_VER >= 1930
        path += "vc143\\";
#elif _MSC_VER >= 1900
        path += "vc14\\";
#elif _MSC_VER >= 1800
        path += "vc12\\";
#elif _MSC_VER >= 1700
        path += "vc11\\";
#elif _MSC_VER >= 1600
        path += "vc10\\";
#elif _MSC_VER >= 1500
        path += "vc9\\";
#elif _MSC_VER >= 1400
        path += "vc8\\";
#else
        path += "vc7\\";
#endif

#if _M_X64
        path += "x64\\";
#elif _M_ARM64
        path += "arm64\\";
#else
        path += "win32\\";
#endif
#else
#if ( __WORDSIZE == 64 )
        path += "linux/x64/";
#else
        path += "linux/x86/";
#endif
#endif

        /*
        #ifdef _WIN32
        #if (DIRECTX9_FOUND == 0 && DIRECTX11_FOUND == 0)
        path += "NODX\\";
        #endif
        #endif
        */
        return path;
    }

    static void TRITONAPI Sprintf(char *string, size_t sizeInBytes, const char *format, float val);
    static void TRITONAPI Sprintf(char *string, size_t sizeInBytes, const char *format, double val);
    static void TRITONAPI Sprintf(char *string, size_t sizeInBytes,const char *format, int val);

    /** Retrieves the suffix on the FFT implementation DLL's for this specific
        build flavor's runtime library. */
    static TRITON_STRING TRITONAPI GetDLLSuffix();

    /** Retrieves the DLL extension. */
    static TRITON_STRING TRITONAPI GetDLLExtension() {
#ifdef _WIN32
        return ".dll";
#else
        return ".so";
#endif
    }

    /** Returns HLSL preprocessor defines for DirectX9, indicating the shader models avaialble to our shaders.
        \return NULL if minimum system requirements for DX9 are not present, a null-terminated D3DXMACRO array otherwise.*/
    static struct _D3DXMACRO * TRITONAPI GetDX9Macros(IDirect3DDevice9 *device, bool hdr, bool breakingWaves);

    /** Returns the filename for the main water shaders, which may be overridden via config. */
    static TRITON_STRING TRITONAPI GetWaterShaderFileName(const Environment *env, bool tessendorf, bool fragment, bool patch);

    /** Returns the filename for the particle shaders, which may be overridden via config. */
    static TRITON_STRING TRITONAPI GetParticleShaderFileName(const Environment *env, bool fragment);

    /** Returns the filename for the user-defined fragment shader functions (OpenGL only; see the Resources/user-functions.glsl file.) */
    static TRITON_STRING TRITONAPI GetUserShaderFileName();

    /** Returns the filename for the user-defined vertex shader functions (OpenGL only; see the Resources/user-vert-functions.glsl file.) */
    static TRITON_STRING TRITONAPI GetUserVertShaderFileName();

    /** Returns the filename for the volumetric deferred decal shaders, which may be overridden via config. */
    static TRITON_STRING TRITONAPI GetDecalShaderFileName(const Environment *env, bool fragment);

    /** Returns the filename for the god ray shaders, which may be overridden via config. */
    static TRITON_STRING TRITONAPI GetGodRayShaderFileName(const Environment *env, bool fragment);
};

// A collection of DirectX11 resources that make up a texture that might be bound to a compute shader.
typedef struct DX11Texture_S {
    ID3D11Texture2D *tex2D;
    ID3D11Texture2D *staging;
    ID3D11ShaderResourceView *srv;
    ID3D11UnorderedAccessView *uav;
} DX11Texture;
}

#endif
