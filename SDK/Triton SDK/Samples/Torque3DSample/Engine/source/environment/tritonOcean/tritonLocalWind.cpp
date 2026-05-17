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

#include "environment/tritonOcean/tritonLocalWind.h"

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
#include "console/engineAPI.h"

Signal<void()> TritonLocalWind::smLocalWindUpdateSignal;

IMPLEMENT_CO_NETOBJECT_V1(TritonLocalWind);

ConsoleDocClass( TritonLocalWind,
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
TritonLocalWind::TritonLocalWind() : mWindSpeed(5.0f)
{
    // Flag this object so that it will always
    // be sent across the network to clients
    mNetFlags.set( Ghostable | ScopeAlways );

    // Set it as a "static" object
    mTypeMask |= EnvironmentObjectType;
}

TritonLocalWind::~TritonLocalWind()
{
}

//-----------------------------------------------------------------------------
// Object Editing
//-----------------------------------------------------------------------------
void TritonLocalWind::initPersistFields()
{
    addGroup( "Wind" );
    addField( "speed", TypeF32, Offset( mWindSpeed, TritonLocalWind ),
              "Set the wind speed, affects wave height.");
    endGroup( "Wind" );

    // SceneObject already handles exposing the transform
    Parent::initPersistFields();
}

void TritonLocalWind::inspectPostApply()
{
    Parent::inspectPostApply();

    setMaskBits(UpdateMask);
}

bool TritonLocalWind::onAdd()
{
    if ( !Parent::onAdd() )
        return false;

    // Set up a 1x1x1 bounding box
    mObjBox.set( Point3F( -0.5f, -0.5f, -0.5f ),
                 Point3F(  0.5f,  0.5f,  0.5f ) );

    resetWorldBox();

    // Add this object to the scene
    addToScene();

    mWind.SetLocalization(Triton::Vector3(getPosition().x, getPosition().y, getPosition().z), Triton::Vector3(getScale().x, getScale().y, getScale().z));
    Point3F euler = getTransform().toEuler();
    mWind.SetWind(mWindSpeed, euler.z-1.57079633);
    mWind.SetFetchLength(60000);
    smLocalWindUpdateSignal.trigger();

    return true;
}

void TritonLocalWind::onRemove()
{
    // Remove this object from the scene
    removeFromScene();
    smLocalWindUpdateSignal.trigger();
    Parent::onRemove();
}

void TritonLocalWind::setTransform(const MatrixF & mat)
{
    // Let SceneObject handle all of the matrix manipulation
    Parent::setTransform( mat );

    // Dirty our network mask so that the new transform gets
    // transmitted to the client object
    setMaskBits( TransformMask );
}

U32 TritonLocalWind::packUpdate( NetConnection *conn, U32 mask, BitStream *stream )
{
    // Allow the Parent to get a crack at writing its info
    U32 retMask = Parent::packUpdate( conn, mask, stream );

    // Write our transform information
    if ( stream->writeFlag( mask & TransformMask ) ) {
        mathWrite(*stream, getTransform());
        mathWrite(*stream, getScale());
    }
    if( stream->writeFlag( mask & UpdateMask ) ) {
        stream->write(mWindSpeed);
    }

    return retMask;
}

void TritonLocalWind::unpackUpdate(NetConnection *conn, BitStream *stream)
{
    // Let the Parent read any info it sent
    Parent::unpackUpdate(conn, stream);

    if ( stream->readFlag() ) { // TransformMask
        mathRead(*stream, &mObjToWorld);
        mathRead(*stream, &mObjScale);

        setTransform( mObjToWorld );
        mWind.SetLocalization(Triton::Vector3(getPosition().x, getPosition().y, getPosition().z), Triton::Vector3(getScale().x, getScale().y, getScale().z));
        Point3F euler = getTransform().toEuler();
        mWind.SetWind(mWindSpeed, euler.z-1.57079633);
        smLocalWindUpdateSignal.trigger();
    }
    if( stream->readFlag() ) { // UpdateMask
        stream->read(&mWindSpeed);
        mWind.SetLocalization(Triton::Vector3(getPosition().x, getPosition().y, getPosition().z), Triton::Vector3(getScale().x, getScale().y, getScale().z));
        Point3F euler = getTransform().toEuler();
        mWind.SetWind(mWindSpeed, euler.z-1.57079633);
        smLocalWindUpdateSignal.trigger();
    }
}

void TritonLocalWind::prepRenderImage( SceneRenderState *state )
{
    if(!isSelected())
        return;

    ObjectRenderInst *ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
    ri->renderDelegate.bind( this, &TritonLocalWind::render );
    ri->type = RenderPassManager::RIT_Editor;
    ri->translucentSort = true;
    ri->defaultKey = 1;
    state->getRenderPass()->addInst( ri );
}

void TritonLocalWind::render( ObjectRenderInst *ri, SceneRenderState *state, BaseMatInstance *overrideMat )
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

    drawer->drawCube(desc, getScale(), getPosition(), ColorI(0,255,0,80));
    Point3F euler = getTransform().toEuler();
    F32 x = cos(euler.z) * (mWindSpeed*10) + getPosition().x;
    F32 y = sin(euler.z) * (mWindSpeed*10) + getPosition().y;
    drawer->drawArrow(desc, getPosition(), Point3F(x,y,getPosition().z), ColorI(0,0,255,80));
}

const Triton::WindFetch& TritonLocalWind::getWindFetch()
{
    return mWind;
}

F32 TritonLocalWind::getWindSpeed()
{
    return mWindSpeed;
}

void TritonLocalWind::setWindSpeed(F32 speed)
{
    mWindSpeed = speed;
    setMaskBits(UpdateMask);
}

DefineEngineMethod(TritonLocalWind, setWindSpeed, void, (F32 speed), (0), "Sets the wind speed.")
{
    object->setWindSpeed(speed);
}

DefineEngineMethod(TritonLocalWind, getWindSpeed, F32, (), (0), "Gets the wind speed.")
{
    return object->getWindSpeed();
}
