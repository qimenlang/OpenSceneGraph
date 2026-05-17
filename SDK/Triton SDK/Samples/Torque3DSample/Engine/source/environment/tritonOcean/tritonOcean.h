//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _TRITONOCEAN_H_
#define _TRITONOCEAN_H_

#ifndef _WATEROBJECT_H_
#include "environment/waterObject.h"
#endif
#ifndef _GFXDEVICE_H_
#include "gfx/gfxDevice.h"
#endif
#ifndef _REFLECTOR_H_
#include "scene/reflector.h"
#endif
#ifndef _SCENEDATA_H_
#include "materials/sceneData.h"
#endif
#ifndef _FOGSTRUCTS_H_
#include "scene/fogStructs.h"
#endif

#include "Triton.h"

class BaseMatInstance;
class MatrixSet;
class TerrainBlock;
struct IDirect3DTexture9;

//-----------------------------------------------------------------------------
// This class implements a basic SceneObject that can exist in the world at a
// 3D position and render itself. Note that RenderObjectExample handles its own
// rendering by submitting itself as an ObjectRenderInst (see
// renderInstance\renderPassmanager.h) along with a delegate for its render()
// function. However, the preffered rendering method in the engine is to submit
// a MeshRenderInst along with a Material, vertex buffer, primitive buffer, and
// transform and allow the RenderMeshMgr handle the actual rendering. You can
// see this implemented in RenderMeshExample.
//-----------------------------------------------------------------------------

class TritonOcean : public WaterObject
{
    typedef WaterObject Parent;

    // Networking masks
    // We need to implement at least one of these to allow
    // the client version of the object to receive updates
    // from the server version (like if it has been moved
    // or edited)
    enum MaskBits {
        TransformMask = Parent::NextFreeMask << 0,
        WindMask = Parent::NextFreeMask << 1,
        WaterMask = Parent::NextFreeMask << 2,
        TimeMask = Parent::NextFreeMask << 3,
        NextFreeMask  = Parent::NextFreeMask << 4
    };

    //Triton
    bool mRenderInit;
    Triton::ResourceLoader *mResourceLoader;
    Triton::Environment *mEnvironment;
    Triton::Ocean *mOcean;
    IDirect3DTexture9* mTerrainTex;

    //Matrices
    MatrixSet *mMatrixSet;
    double mMatModelView[16], mMatProjection[16];

    //Reflection
    PlaneReflector mPlaneReflector;
    ReflectorDesc mReflectorDesc;
    Point3F mWaterPos;
    PlaneF mWaterPlane;
    F32 mReflectivity;
    ColorF mRefractColor;

    //Wind
    F32 mGlobalWindSpeed;
    F32 mGlobalWindDirection;
    Triton::WindFetch mGlobalWind;
    bool mRequireWindUpdate;

    //Water
    F32 mWaveChoppiness;
    F32 mWaterVisibility;
    bool mRequireWaterUpdate;
    U32 mServerTime;
    U32 mTimeCount;
    U32 mTimeOffset;

    //Wake
    static Signal<void(Triton::Ocean* ocean)> smWakeUpdateSignal;

    //Water Height
    Triton::Vector3 mHeightRayDirection;
    Triton::Vector3 mHeightNormal;

public:
    TritonOcean();
    virtual ~TritonOcean();

    // Declare this object as a ConsoleObject so that we can
    // instantiate it into the world and network it
    DECLARE_CONOBJECT(TritonOcean);

    //--------------------------------------------------------------------------
    // Object Editing
    // Since there is always a server and a client object in Torque and we
    // actually edit the server object we need to implement some basic
    // networking functions
    //--------------------------------------------------------------------------
    // Set up any fields that we want to be editable (like position)
    static void initPersistFields();

    // Handle when we are added to the scene and removed from the scene
    bool onAdd();
    void onRemove();

    // Override this so that we can dirty the network flag when it is called
    void setTransform( const MatrixF &mat );

    // This function handles sending the relevant data from the server
    // object to the client object
    U32 packUpdate( NetConnection *conn, U32 mask, BitStream *stream );
    // This function handles receiving relevant data from the server
    // object and applying it to the client object
    void unpackUpdate( NetConnection *conn, BitStream *stream );

    virtual void inspectPostApply();

    void initTriton();
    void deinitTriton();

    void matrixToDoubleArray(const MatrixF& matrix, double out[]);

    // This is the function that allows this object to submit itself for rendering
    void prepRenderImage( SceneRenderState *state );
    // This is the function that actually gets called to do the rendering
    void render( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat );
    void renderInit();
    void updateSettings();

    void processTick(const Move* move);
    void updateLocalWinds();
    void updateWinds();
    static Signal<void(Triton::Ocean* ocean)>& getWakeUpdateSignal() {
        return smWakeUpdateSignal;
    }

    void getSurfaceHeightNormal(const Point2F &pos, F32 &height, Point3F &normal);
    F32 getSurfaceHeight(const Point2F &pos) const;

    bool isUnderwater( const Point3F &pnt ) const;
    F32 getWaterCoverage( const Box3F &testBox ) const;

    //engine methods
    F32 getGlobalWindSpeed();
    F32 getGlobalWindDirection();
    F32 getChoppiness();
    void setGlobalWind(F32 speed, F32 direction);
    void setChoppiness(F32 choppiness);

    // Required for cleaning up textures in Triton
    // Without this, Triton will crash
    // @see GFXTextureManager::addEventDelegate
    void _onTextureEvent( GFXTexCallbackCode code );
};

#endif // _TRITONOCEAN_H_