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
#include "environment/tritonOcean/tritonWake.h"

#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "core/stream/bitStream.h"
#include "materials/sceneData.h"
#include "gfx/gfxDebugEvent.h"
#include "gfx/primBuilder.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "renderInstance/renderPassManager.h"
#include "console/consoleTypes.h"

IMPLEMENT_CO_NETOBJECT_V1(TritonWake);

ConsoleDocClass( TritonWake,
                 "@brief An example scene object which renders using a callback.\n\n"
                 "This class implements a basic SceneObject that can exist in the world at a "
                 "3D position and render itself. Note that RenderObjectExample handles its own "
                 "rendering by submitting itself as an ObjectRenderInst (see "
                 "renderInstance\renderPassmanager.h) along with a delegate for its render() "
                 "function. However, the preffered rendering method in the engine is to submit "
                 "a MeshRenderInst along with a Material, vertex buffer, primitive buffer, and "
                 "transform and allow the RenderMeshMgr handle the actual rendering. You can "
                 "see this implemented in RenderMeshExample.\n\n"
                 "See the C++ code for implementation details.\n\n"
                 "@ingroup Examples\n" );

//-----------------------------------------------------------------------------
// Object setup and teardown
//-----------------------------------------------------------------------------
TritonWake::TritonWake() : mWake(NULL)
{
    // Flag this object so that it will always
    // be sent across the network to clients
    mNetFlags.set( Ghostable | ScopeAlways );

    // Set it as a "static" object
    mTypeMask |= EnvironmentObjectType;
}

TritonWake::~TritonWake()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void TritonWake::initPersistFields()
{
    /*addGroup( "Wind" );
       addField( "speed", TypeF32, Offset( mWindSpeed, TritonWake ),
       "Set the wind speed, affects wave height.");
       addField( "direction", TypeF32, Offset( mWindDirection, TritonWake ),
       "Set the wind direction.");
       addField( "radius", TypeF32, Offset( mWindRadius, TritonWake ),
       "Set the radius of the area affected by wind.");
    endGroup( "Wind" );*/

    // SceneObject already handles exposing the transform
    Parent::initPersistFields();
}

void TritonWake::inspectPostApply()
{
    Parent::inspectPostApply();

    setMaskBits(UpdateMask);
}

bool TritonWake::onAdd()
{
    if ( !Parent::onAdd() )
        return false;

    // Set up a 1x1x1 bounding box
    mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                 Point3F(  0.5f,  0.5f,  0.5f ) );

    resetWorldBox();

    // Add this object to the scene
    addToScene();

    TritonOcean::getWakeUpdateSignal().notify(this, &TritonWake::updateWake);

    return true;
}

void TritonWake::onRemove()
{
    TritonOcean::getWakeUpdateSignal().remove(this, &TritonWake::updateWake);

    // Remove this object from the scene
    removeFromScene();

    Parent::onRemove();
}

void TritonWake::setTransform(const MatrixF & mat)
{
    // Let SceneObject handle all of the matrix manipulation
    Parent::setTransform( mat );

    // Dirty our network mask so that the new transform gets
    // transmitted to the client object
    setMaskBits( TransformMask );
}

U32 TritonWake::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
    // Allow the Parent to get a crack at writing its info
    U32 retMask = Parent::packUpdate( conn, mask, stream );

    // Write our transform information
    if ( stream->writeFlag( mask & TransformMask ) ) {
        mathWrite(*stream, getTransform());
        mathWrite(*stream, getScale());
    }
    if( stream->writeFlag( mask & UpdateMask ) ) {
    }

    return retMask;
}

void TritonWake::unpackUpdate(NetConnection *conn, BitStream *stream)
{
    // Let the Parent read any info it sent
    Parent::unpackUpdate(conn, stream);

    if ( stream->readFlag() ) { // TransformMask
        mathRead(*stream, &mObjToWorld);
        mathRead(*stream, &mObjScale);

        setTransform( mObjToWorld );
    }
    if( stream->readFlag() ) { // UpdateMask
    }
}

void TritonWake::updateWake(Triton::Ocean* ocean)
{
    if(!mWake) {
        //mWake = new Triton::WakeGenerator(ocean, false, 0.0, 9.144, 4.0);
        mWake->SetParameters(mWakeParams);
    } else {
        F32 time = Sim::getCurrentTime() / 1000.0;
        if(mPrevTime != time) {
            Triton::Vector3 position(getPosition().x, getPosition().y, getPosition().z);
            double deltaX = position.x - mPrevPos.x;
            double deltaY = position.y - mPrevPos.y;
            double angle = atan2(deltaX, deltaY);
            double dist = sqrt(pow(deltaX,2) + pow(deltaY,2));
            double diff = time - mPrevTime;
            double speed = (dist / diff);
            if( speed > 10 )
                speed = 10;
            mWake->Update(position,Triton::Vector3(sin(angle), cos(angle), 0), speed, time);
            mPrevPos = position;
            mPrevTime = time;
        }
    }
}

void TritonWake::prepRenderImage( SceneRenderState *state )
{
    if(!isSelected())
        return;

    ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
    ri->renderDelegate.bind( this, &TritonWake::render );
    ri->type = RenderPassManager::RIT_Editor;
    ri->translucentSort = true;
    ri->defaultKey = 1;
    state->getRenderPass()->addInst( ri );
}

void TritonWake::render( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
{
    if ( overrideMat )
        return;

    GFXTransformSaver saver;
    GFXDrawUtil *drawer = GFX->getDrawUtil();
    AssertFatal( drawer, "Got NULL GFXDrawUtil!" );
    GFXStateBlockDesc desc;
    desc.setCullMode( GFXCullNone );
    desc.setBlend( true );
    desc.setZReadWrite( true, false );

    drawer->drawSphere(desc, 0.5, getPosition(), ColorI(0,255,0,80));
}
