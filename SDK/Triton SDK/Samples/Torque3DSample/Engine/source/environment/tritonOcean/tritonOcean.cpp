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

#include "environment/tritonOcean/tritonOcean.h"
#include "environment/tritonOcean/tritonLocalWind.h"

#include "math/mathIO.h"
#include "math/util/matrixSet.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "materials/baseMatInstance.h"
#include "materials/materialParameters.h"
#include "terrain/terrData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/gfxTransformSaver.h"
#include "gfx/D3D9/gfxD3D9Device.h"
#include "gfx/gfxOcclusionQuery.h"
#include "renderInstance/renderPassManager.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

Signal<void(Triton::Ocean* ocean)> TritonOcean::smWakeUpdateSignal;

IMPLEMENT_CO_NETOBJECT_V1(TritonOcean);

ConsoleDocClass( TritonOcean,
                 "@brief Represents a large body of water stretching to the horizon in all directions.\n\n"

                 "TritonOcean's position is defined only by height, the z element of position, "
                 "it is infinite in xy and depth.\n\n"

                 "@ingroup Water"
               );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
TritonOcean::TritonOcean() : mRenderInit(true), mEnvironment(NULL), mOcean(NULL), mResourceLoader(NULL), mTerrainTex(NULL), mMatrixSet(NULL),
    mRequireWindUpdate(false), mRequireWaterUpdate(false), mGlobalWindSpeed(3), mGlobalWindDirection(0), mWaveChoppiness(1.2f), mWaterVisibility(3),
    mReflectivity(0.5f), mRefractColor(0.0f, 0.2f, 0.3f), mHeightRayDirection(0,0,-1), mServerTime(0), mTimeCount(301), mTimeOffset(0)
{
    // Flag this object so that it will always
    // be sent across the network to clients
    mNetFlags.set( Ghostable | ScopeAlways );

    mWaterPos.set( 0,0,0 );
    mWaterPlane.set( mWaterPos, Point3F(0,0,1) );
    mDepthGradientTexName = "art/water/depthcolor_ramp";

    mMatrixSet = reinterpret_cast<MatrixSet *>(dMalloc_aligned(sizeof(MatrixSet), 16));
    constructInPlace(mMatrixSet);
}

TritonOcean::~TritonOcean()
{
    dFree_aligned(mMatrixSet);
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void TritonOcean::initPersistFields()
{
    // SceneObject already handles exposing the transform
    addGroup( "Global Wind" );
    addField( "globalWindSpeed", TypeF32, Offset( mGlobalWindSpeed, TritonOcean ),
              "Set the global wind speed, affects wave height.");
    addField( "globalWindDirection", TypeF32, Offset( mGlobalWindDirection, TritonOcean ),
              "Set the global wind direction.");
    endGroup( "Global Wind" );
    addGroup( "Water" );
    addField( "waveChoppiness", TypeF32, Offset( mWaveChoppiness, TritonOcean ),
              "Set the choppiness of the waves.");
    addField( "waterVisibility", TypeF32, Offset( mWaterVisibility, TritonOcean ),
              "Set the transparency of the water.");
    endGroup( "Water" );
    addGroup( "Reflections" );
    addField( "refractColor" , TypeColorF, Offset( mRefractColor, TritonOcean ), "Color of water in areas that are not purely reflective." );
    addField( "reflectivity", TypeF32, Offset( mReflectivity, TritonOcean ), "Overall scalar to the reflectivity of the water surface." );
    addField( "reflectPriority", TypeF32, Offset( mReflectorDesc.priority, TritonOcean ), "Affects the sort order of reflected objects." );
    addField( "reflectMaxRateMs", TypeS32, Offset( mReflectorDesc.maxRateMs, TritonOcean ), "Affects the sort time of reflected objects." );
    addField( "reflectDetailAdjust", TypeF32, Offset( mReflectorDesc.detailAdjust, TritonOcean ), "scale up or down the detail level for objects rendered in a reflection" );
    addField( "useOcclusionQuery", TypeBool, Offset( mReflectorDesc.useOcclusionQuery, TritonOcean ), "turn off reflection rendering when occluded (delayed)." );
    addField( "reflectTexSize", TypeS32, Offset( mReflectorDesc.texSize, TritonOcean ), "The texture size used for reflections (square)" );
    endGroup( "Reflections" );

    Parent::initPersistFields();
    removeField( "rotation" );
    removeField( "scale" );
}

void TritonOcean::inspectPostApply()
{
    Parent::inspectPostApply();

    setMaskBits(WindMask|WaterMask);
}

bool TritonOcean::onAdd()
{
    if ( !Parent::onAdd() )
        return false;

    initTriton();
    setGlobalBounds();
    resetWorldBox();
    addToScene();

    TritonLocalWind::getLocalWindUpdateSignal().notify(this, &TritonOcean::updateLocalWinds);

    mWaterFogData.plane.set( 0, 0, 1, -getPosition().z );

    if(isClientObject()) {
        mPlaneReflector.registerReflector( this, &mReflectorDesc );
    }

    return true;
}

void TritonOcean::onRemove()
{
    TritonLocalWind::getLocalWindUpdateSignal().remove(this, &TritonOcean::updateLocalWinds);
    removeFromScene();
    deinitTriton();
    if(isClientObject()) {
        mPlaneReflector.unregisterReflector();
    }
    Parent::onRemove();
}

void TritonOcean::setTransform(const MatrixF & mat)
{
    // Let SceneObject handle all of the matrix manipulation
    Parent::setTransform( mat );

    mWaterPos.set( 0,0,getPosition().z );
    mWaterPlane.set( mWaterPos, Point3F(0,0,1) );

    if(mEnvironment)
        mEnvironment->SetSeaLevel(mWaterPos.z);

    mPlaneReflector.refplane = mWaterPlane;

    // Dirty our network mask so that the new transform gets
    // transmitted to the client object
    setMaskBits( TransformMask );
}

U32 TritonOcean::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
    // Allow the Parent to get a crack at writing its info
    U32 retMask = Parent::packUpdate( conn, mask, stream );

    // Write our transform information
    if ( stream->writeFlag( mask & TransformMask ) ) {
        mathWrite(*stream, getTransform());
        mathWrite(*stream, getScale());
    }

    if( stream->writeFlag( mask & TimeMask ) ) {
        stream->write(mServerTime);
    }

    //Write world editor settings information
    if( stream->writeFlag( mask & WindMask ) ) {
        stream->write( mGlobalWindSpeed );
        stream->write( mGlobalWindDirection );
        mGlobalWind.SetWind(mGlobalWindSpeed, mGlobalWindDirection);
        mRequireWindUpdate = true;
    }

    if( stream->writeFlag( mask & WaterMask ) ) {
        stream->write( mWaveChoppiness );
        mOcean->SetChoppiness(mWaveChoppiness);
        stream->write( mWaterVisibility );
        stream->write( mReflectorDesc.priority );
        stream->writeInt( mReflectorDesc.maxRateMs, 32 );
        stream->write( mReflectorDesc.detailAdjust );
        stream->writeFlag( mReflectorDesc.useOcclusionQuery );
        stream->writeInt( mReflectorDesc.texSize, 32 );
        stream->write( mReflectivity );
        stream->write(sizeof(ColorF), mRefractColor );
    }

    return retMask;
}

void TritonOcean::unpackUpdate(NetConnection *conn, BitStream *stream)
{
    // Let the Parent read any info it sent
    Parent::unpackUpdate(conn, stream);

    if ( stream->readFlag() ) { // TransformMask
        mathRead(*stream, &mObjToWorld);
        mathRead(*stream, &mObjScale);
        setTransform( mObjToWorld );
    }

    if( stream->readFlag() ) {
        stream->read(&mServerTime);
        mTimeOffset = mServerTime - Sim::getCurrentTime();
        Con::printf("%d",mTimeOffset);
    }

    if( stream->readFlag() ) { // WindMask
        stream->read( &mGlobalWindSpeed );
        stream->read( &mGlobalWindDirection );
        mGlobalWind.SetWind(mGlobalWindSpeed, mGlobalWindDirection);
        mRequireWindUpdate = true;
    }

    if(stream->readFlag() ) { // WaterMask
        stream->read( &mWaveChoppiness );
        stream->read( &mWaterVisibility );
        stream->read( &mReflectorDesc.priority );
        mReflectorDesc.maxRateMs = stream->readInt( 32 );
        stream->read( &mReflectorDesc.detailAdjust );
        mReflectorDesc.useOcclusionQuery = stream->readFlag();
        mReflectorDesc.texSize = stream->readInt( 32 );
        stream->read( &mReflectivity );
        stream->read(sizeof(ColorF), &mRefractColor );
        mRequireWaterUpdate = true;
    }
}

void TritonOcean::initTriton()
{
    Triton::EnvironmentError err;
    mResourceLoader = new Triton::ResourceLoader("Resources");
    mEnvironment = new Triton::Environment();
    if(isClientObject()) {
        GFXTextureManager::addEventDelegate( this, &TritonOcean::_onTextureEvent );
        IDirect3DDevice9 *mD3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();
        err = mEnvironment->Initialize(Triton::FLAT_ZUP, Triton::DIRECTX_9, mResourceLoader, mD3DDevice, true);
    } else {
        err = mEnvironment->Initialize(Triton::FLAT_ZUP, Triton::NO_RENDERER, mResourceLoader);
        setProcessTick(true);
    }

    mEnvironment->SetLicenseCode("Your License Name","Your License Code");
    mEnvironment->SetSeaLevel(getPosition().z);
    mEnvironment->GetRandomNumberGenerator()->SetRandomSeed(1337);
    mGlobalWind.SetWind(mGlobalWindSpeed, mGlobalWindDirection);
    mOcean = Triton::Ocean::Create(mEnvironment, Triton::PIERSON_MOSKOWITZ, true, false);
    mOcean->SetChoppiness(mWaveChoppiness);
    if(isServerObject())
        mEnvironment->AddWindFetch(mGlobalWind);
}

void TritonOcean::deinitTriton()
{
    GFXTextureManager::removeEventDelegate( this, &TritonOcean::_onTextureEvent );
    if (mOcean) {
        delete mOcean;
        mOcean = NULL;
    }
    if (mEnvironment) {
        delete mEnvironment;
        mEnvironment = NULL;
    }
    if (mResourceLoader) {
        delete mResourceLoader;
        mResourceLoader = NULL;
    }
}

void TritonOcean::updateLocalWinds()
{
    mRequireWindUpdate = true;
}

void TritonOcean::updateWinds()
{
    mEnvironment->ClearWindFetches();
    mEnvironment->AddWindFetch(mGlobalWind);
    Vector<SceneObject*> localWinds;
    getContainer()->findObjectList(EnvironmentObjectType, &localWinds);
    for(int i = 0; i < localWinds.size(); i++) {
        TritonLocalWind* localWind = dynamic_cast<TritonLocalWind*>(localWinds[i]);
        if(localWind) {
            mEnvironment->AddWindFetch(localWind->getWindFetch());
        }
    }
}

void TritonOcean::prepRenderImage( SceneRenderState *state )
{
    PROFILE_SCOPE(TritonOcean_PrepRenderImage);

    if(!state->isDiffusePass())
        return;

    mUnderwater = isUnderwater( state->getCameraPosition() );

    mMatrixSet->setSceneView(GFX->getWorldMatrix());
    mMatrixSet->setSceneProjection(GFX->getProjectionMatrix());

    mPlaneReflector.refplane = mWaterPlane;
    mWaterFogData.plane = mWaterPlane;

    updateUnderwaterEffect( state );

    // Allocate an ObjectRenderInst so that we can submit it to the RenderPassManager
    ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
    // Now bind our rendering function so that it will get called
    ri->renderDelegate.bind( this, &TritonOcean::render );
    // Set our RenderInst as a standard object render
    ri->type = RenderPassManager::RIT_Water;
    // Submit our RenderInst to the RenderPassManager
    state->getRenderPass()->addInst( ri );
}

void TritonOcean::renderInit()
{
    const Vector<SceneObject*> terrains = getContainer()->getTerrains();
    //For now assume only one terrain
    if(terrains.size() > 0) {
        TerrainBlock* terrainBlock = dynamic_cast<TerrainBlock*>(terrains[0]);
        IDirect3DDevice9 *mD3DDevice = dynamic_cast<GFXD3D9Device *>(GFX)->getDevice();
        U32 size = terrainBlock->getBlockSize();
        Triton::Matrix4 heightMapMatrix;
        Point3F terrainPos = terrainBlock->getPosition();
        HRESULT hr = mD3DDevice->CreateTexture(size, size, 0, 0, D3DFMT_R32F, D3DPOOL_MANAGED, &mTerrainTex, NULL);
        if (SUCCEEDED(hr)) {
            D3DLOCKED_RECT rect;
            mTerrainTex->LockRect(0, &rect, NULL, 0);
            for (int row = 0; row < size; row++) {
                for (int col = 0; col < size; col++) {
                    float height = terrainBlock->getHeight(Point2I(col, row)) + terrainPos.z;
                    int idx = row * (rect.Pitch / sizeof(float)) + col;
                    float *ptr = (float *)rect.pBits;
                    ptr[idx] = height;
                }
            }
            mTerrainTex->UnlockRect(0);
            Triton::Matrix4 trans(1, 0, 0, 0,
                                  0, 1, 0, 0,
                                  0, 0, 1, 0,
                                  -(terrainPos.x), -(terrainPos.y), 0, 1);

            float s = 1.0f / terrainBlock->getWorldBlockSize();
            Triton::Matrix4 scale(s, 0, 0, 0,
                                  0, s, 0, 0,
                                  0, 0, s, 0,
                                  0, 0, 0, 1);

            heightMapMatrix = trans * scale;
        }
        mEnvironment->SetHeightMap((Triton::TextureHandle)mTerrainTex, heightMapMatrix);
        mRenderInit = false;
    }
    updateWinds();
}

void TritonOcean::updateSettings()
{
    if(mRequireWindUpdate) {
        updateWinds();
        mRequireWindUpdate = false;
    }
    if(mRequireWaterUpdate) {
        mOcean->SetChoppiness(mWaveChoppiness);
        Triton::Vector3 fogColor;
        double visibility;
        mEnvironment->GetBelowWaterVisibility(visibility, fogColor);
        mEnvironment->SetBelowWaterVisibility(mWaterVisibility, Triton::Vector3(mRefractColor.red, mRefractColor.green, mRefractColor.blue));
        mOcean->SetPlanarReflectionBlend(mReflectivity);
        mOcean->SetRefractionColor(Triton::Vector3(mRefractColor.red, mRefractColor.green, mRefractColor.blue));
        mRequireWaterUpdate = false;
    }
}

void TritonOcean::render( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
    if ( !mOcean && !mEnvironment && overrideMat )
        return;

    PROFILE_SCOPE(TritonOcean_Render);

    // Set up a GFX debug event (this helps with debugging rendering events in external tools)
    GFXDEBUGEVENT_SCOPE( TritonOcean_Render, ColorI::RED );

    if(mRenderInit)
        renderInit();

    updateSettings();

    GFXOcclusionQuery *query = mPlaneReflector.getOcclusionQuery();
    bool doQuery = (!mPlaneReflector.mQueryPending && query && mReflectorDesc.useOcclusionQuery);

    if(doQuery)
        query->begin();

    mMatrixSet->restoreSceneViewProjection();
    mMatrixSet->setWorld(getRenderTransform());

    // GFXTransformSaver is a handy helper class that restores
    // the current GFX matrices to their original values when
    // it goes out of scope at the end of the function
    GFXTransformSaver saver;

    //Set Triton Matrices
    MatrixF view = mMatrixSet->getWorldToCamera();
    MatrixF proj = GFX->getProjectionMatrix();
    view = view.transpose();
    proj = proj.transpose();
    matrixToDoubleArray(view, mMatModelView);
    matrixToDoubleArray(proj, mMatProjection);
    mEnvironment->SetCameraMatrix(mMatModelView);
    mEnvironment->SetProjectionMatrix(mMatProjection);

    //Set Triton Lighting
    LightInfo* sunLight = LIGHTMGR->getSpecialLight( LightManager::slSunLightType );
    Point3F sunLightPos = sunLight->getPosition();
    Triton::Vector3 tritonLightPos(sunLightPos.x,sunLightPos.y,sunLightPos.z);
    ColorF sunLightColor = sunLight->getColor();
    mEnvironment->SetDirectionalLight(tritonLightPos, Triton::Vector3(sunLightColor.red, sunLightColor.green, sunLightColor.blue));
    ColorF ambientLight = sunLight->getAmbient();
    mEnvironment->SetAmbientLight(Triton::Vector3(ambientLight.red, ambientLight.green, ambientLight.blue));

    //Reflection
    SceneData sd;
    sd.reflectTex = mPlaneReflector.reflectTex;
    GFXD3D9TextureObject *reflectTO = dynamic_cast<GFXD3D9TextureObject *>(sd.reflectTex);
    IDirect3DTexture9 *reflect = reflectTO->get2DTex();
    MatrixF matReflectCam, matTrans(true), matScale(true), matReflect(true);
    matReflect.setRow(1,Point4F(0,-1,0,0));
    matTrans.setColumn(3, Point3F(1,1,1));
    view.setRow(3,Point3F(0,0,0));
    matReflectCam = view * proj * matTrans.transpose();
    matReflectCam = matReflectCam.scale(Point3F(0.5,0.5,0.5));
    matReflectCam *= matReflect.transpose();
    mEnvironment->SetPlanarReflectionMap(reflect, Triton::Matrix3(matReflectCam(0,0), matReflectCam(0,1), matReflectCam(0,2), matReflectCam(1,0), matReflectCam(1,1), matReflectCam(1,2), matReflectCam(2,0), matReflectCam(2,1), matReflectCam(2,2)));

    //Update Wake Generators
    smWakeUpdateSignal.trigger(mOcean);

    //Draw Triton Ocean
    mOcean->Draw((Sim::getCurrentTime() + mTimeOffset) / 1000.0f);

    if ( doQuery )
        query->end();
}

void TritonOcean::processTick(const Move* move)
{
    //Server only
    updateSettings();
    //Update Wake Generators
    smWakeUpdateSignal.trigger(mOcean);
    mServerTime = Sim::getCurrentTime();
    if(mTimeCount > 300) {
        setMaskBits(TimeMask);
        mTimeCount = 0;
    } else {
        mTimeCount++;
    }
    mOcean->UpdateSimulation(mServerTime / 1000.0f);
}

void TritonOcean::getSurfaceHeightNormal(const Point2F &pos, F32 &height, Point3F &normal)
{
    if(mOcean) {
        Triton::Vector3 inPos(pos.x, pos.y, mEnvironment->GetSeaLevel()+100);
        mOcean->GetHeight(inPos, mHeightRayDirection, height, mHeightNormal, false, true, true);
        height += mEnvironment->GetSeaLevel();
        normal.set(mHeightNormal.x, mHeightNormal.y, mHeightNormal.z);
    } else {
        height = getPosition().z;
        normal.set(0,0,1);
    }
}

F32 TritonOcean::getSurfaceHeight(const Point2F &pos) const
{
    if(mOcean) {
        F32 height;
        Triton::Vector3 normal;
        Triton::Vector3 inPos(pos.x, pos.y, mEnvironment->GetSeaLevel()+100);
        mOcean->GetHeight(inPos, mHeightRayDirection, height, normal, false, true, true);
        return height + mEnvironment->GetSeaLevel();
    }
    return getPosition().z;
}

F32 TritonOcean::getWaterCoverage( const Box3F &testBox ) const
{
    Point2F center(testBox.getCenter().x, testBox.getCenter().y);
    F32 posZ = getSurfaceHeight(center);
    F32 coverage = 0.0f;
    if ( posZ > testBox.minExtents.z ) {
        if ( posZ < testBox.maxExtents.z )
            coverage = (posZ - testBox.minExtents.z) / (testBox.maxExtents.z - testBox.minExtents.z);
        else
            coverage = 1.0f;
    }
    return coverage;
}

bool TritonOcean::isUnderwater( const Point3F &pnt ) const
{
    F32 height = getSurfaceHeight(Point2F(pnt.x,pnt.y));
    F32 diff = pnt.z - height;
    return ( diff < 0.1 );
}

void TritonOcean::matrixToDoubleArray(const MatrixF& matrix, double out[])
{
    for(int i = 0; i < 16; i++) {
        out[i] = matrix[i];
    }
}

void TritonOcean::_onTextureEvent( GFXTexCallbackCode code )
{
    if(mOcean) {
        if(code == GFXZombify)
            mOcean->D3D9DeviceLost();
        else if(code == GFXResurrect)
            mOcean->D3D9DeviceReset();
    }
}

//Engine Methods

void TritonOcean::setGlobalWind(F32 speed, F32 direction)
{
    mGlobalWindSpeed = speed;
    mGlobalWindDirection = direction;
    setMaskBits(WindMask);
}

F32 TritonOcean::getGlobalWindSpeed()
{
    return mGlobalWindSpeed;
}

F32 TritonOcean::getGlobalWindDirection()
{
    return mGlobalWindDirection;
}

void TritonOcean::setChoppiness(F32 choppiness)
{
    mWaveChoppiness = choppiness;
    setMaskBits(WaterMask);
}

F32 TritonOcean::getChoppiness()
{
    return mWaveChoppiness;
}

DefineEngineMethod(TritonOcean, setGlobalWind, void, (F32 speed, F32 direction), (0), "Sets the global wind speed and direction.")
{
    object->setGlobalWind(speed, direction);
}

DefineEngineMethod(TritonOcean, setGlobalWindSpeed, void, (F32 speed), (0), "Sets the global wind speed.")
{
    object->setGlobalWind(speed, object->getGlobalWindDirection());
}

DefineEngineMethod(TritonOcean, getGlobalWindDirection, void, (F32 direction), (0), "Sets the global wind direction.")
{
    object->setGlobalWind(object->getGlobalWindSpeed(), direction);
}

DefineEngineMethod(TritonOcean, getGlobalWindSpeed, F32, (), (0), "Gets the global wind speed.")
{
    return object->getGlobalWindSpeed();
}

DefineEngineMethod(TritonOcean, setGlobalWindDirection, F32, (), (0), "Gets the global wind direction.")
{
    return object->getGlobalWindDirection();
}

DefineEngineMethod(TritonOcean, setChoppiness, void, (F32 choppiness), (0), "Sets the wave choppiness of the ocean.")
{
    object->setChoppiness(choppiness);
}

DefineEngineMethod(TritonOcean, getChoppiness, F32, (), (0), "Gets the wave choppiness of the ocean.")
{
    return object->getChoppiness();
}
