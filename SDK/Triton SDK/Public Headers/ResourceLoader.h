// Copyright (c) 2010-2013  Sundog Software, LLC All rights reserved worldwide.

/**
    \file ResourceLoader.h
    \brief A class for loading Triton's resources from mass storage, which you may extend.
 */

#ifndef TRITON_RESOURCE_LOADER_H
#define TRITON_RESOURCE_LOADER_H

#ifdef SWIG
%module TritonResourceLoader
#define TRITONAPI
SWIG_CSBODY_PROXY(public, public, SWIGTYPE)
SWIG_CSBODY_TYPEWRAPPER(public, public, public, SWIGTYPE)
%{
#include "ResourceLoader.h"
using namespace Triton;
%}
#endif

#if defined(WIN32) || defined(WIN64)
#pragma warning (disable: 4786)
#endif

#include "TritonCommon.h"
#include <string>

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** This class is used whenever Triton needs to load textures, data files, or shaders from mass storage; you may
   extend this class to override our default use of POSIX filesystem calls with your own resource management if you wish.
   If you have your own system of packed files, you can include Triton's resources directory into it and implement your
   own ResourceLoader to access our resources within your pack files.
 */
class ResourceLoader : public MemObject
{
public:
    /** Constructor.
    \param resourceDirPath The path to Triton's resources folder. Avoid using relative paths if at all possible.
    \param useAddDllDirectory Only applicable to Windows; controls whether we attempt to add the resources/dll
                              directory to the DLL search path using the Windows AddDLLDirectory function instead
                              of SetDLLDirectory. AddDllDirectory is less destructive to the exisiting DLL search 
                              path, but is not well supported on older systems.
    */
    ResourceLoader(const char *resourceDirPath, bool useAddDllDirectory = false);

    /// Virtual destructor; frees the memory of the resource path string.
    virtual ~ResourceLoader();

    /** Sets the path to the Triton resources folder, which will be pre-pended to all resource filenames
        passed into LoadResource(). This method also calls the Win32 function AddDllDirectory in order to
        add the dll subdirectory to the application's DLL search path. This should be a fully qualified path
        and not a relative one if at all possible.
        \param path The path to Triton's Resources folder; avoid using relative paths.
        \param useAddDllDirectory On Windows, controls whether we attempt to use the AddDllDirectory function
                                  instead of SetDLLDirectory in order to add our runtime dependencies into the
                                  DLL search path.
    */
    virtual void TRITONAPI SetResourceDirPath(const char *path, bool useAddDllDirectory = false);

    /// Retrieves the path set by SetResourceDirPath().
    virtual const char * TRITONAPI GetResourceDirPath() const {
        return resourcePath;
    }

    /** Load a resource from mass storage; the default implementation uses the POSIX functions fopen(), fread(), and fclose()
       to do this, but you may override this method to load resources however you wish. The caller is responsible for calling
       FreeResource() when it's done consuming the resource data in order to free its memory.

       \param pathName The path to the desired resource, relative to the location of the resources folder previously specified
       in SetResourceDirPath().
       \param data A reference to a char * that will return the resource's data upon a successful load.
       \param dataLen A reference to an unsigned int that will return the number of bytes loaded upon a successful load.
       \param text True if the resource is a text file, such as a shader. If true, a terminating null character will be appended
       to the resulting data and the file will be opened in text mode.
       \return True if the resource was located and loaded successfully, false otherwise.

       \sa SetResourceDirPath
     */
    virtual bool TRITONAPI LoadResource(const char *pathName, char*& data, unsigned int& dataLen, bool text);

    /** Frees the resource data memory that was returned from LoadResource(). The data pointer will be invalid following
       this call. */
    virtual void TRITONAPI FreeResource(char *data);

protected:
    char *resourcePath;
};

class ScopedResource
{
public:
    ScopedResource(const char* pathName, ResourceLoader* _loader);
    ~ScopedResource();
    char* Data(void) const;
private:
    ResourceLoader* loader;
    char *data;
    unsigned int dataLen;
};
}

#pragma pack(pop)

#endif
