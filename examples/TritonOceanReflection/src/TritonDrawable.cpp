// Triton OpenSceneGraph Sample Project
// Illustrates integration of Triton with an OpenSceneGraph application

// Copyright (c) 2011-2012 Sundog Software LLC. All rights reserved worldwide.

// The Triton.h header includes everything you need

#define _CRT_SECURE_NO_WARNINGS

#include "TritonDrawable.h"

#include <osg/GL>
#include <osg/GLExtensions>
#include <osg/State>

namespace {

GLuint shaderHandleToProgram(Triton::ShaderHandle handle)
{
#if defined(_WIN32) || defined(_WIN64)
    return static_cast<GLuint>(reinterpret_cast<uintptr_t>(handle));
#else
    return static_cast<GLuint>(handle);
#endif
}

}

// Create the Triton objects at startup, once we have a valid GL context in place
TritonDrawable::TritonDrawable( bool geocentric, osg::TextureCubeMap * cubeMap, osg::RefMatrix * cubeMapProjection,
                                osg::Texture2D * planarReflectionMap, osg::RefMatrix * planarReflectionProjection )
    :_resourceLoader(0)
    ,_environment(0)
    ,_ocean(0)
    ,_cubeMap(cubeMap)
    ,_cubeMapProjection(cubeMapProjection)
    ,_planarReflectionMap( planarReflectionMap )
    ,_planarReflectionProjection( planarReflectionProjection )
    ,_geocentric(geocentric)
    ,_ship(0)
    ,_shipPosition( 0,0,0 )
    ,_shipDirection( 0,0,0 )
    ,_shipVelocity( 0.f )
    ,_rotorWash(0)
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseVertexBufferObjects(false);
    setUseDisplayList(false);

    for (int i = 0; i < 2; ++i) {
        _userRotorCenterLoc[i] = -1;
    }
}

// Clean up our resources
TritonDrawable::~TritonDrawable()
{
    Cleanup();
}

void TritonDrawable::UpdateShipPos( double x, double y, double z, double dx, double dy, double dz, double velocity )
{
    _shipPosition.set(x, y, z);
    _shipDirection.set(dx, dy, dz);
    _shipVelocity = velocity;
}

void TritonDrawable::Setup( )
{
    // We use the default resource loader that just loads files from disk. You'll need
    // to redistribute the resources folder if using this. You can also extend the
    // _resourceLoader class to hook into your own resource manager if you wish.
    // Update the path below to where you installed SilverLining's resources folder.
    const char *tritonPath = getenv("TRITON_PATH");
    if (!tritonPath) {
        printf("Can't find Triton; set the TRITON_PATH environment variable ");
        printf("to point to the directory containing the SDK.\n");
        exit(0);
    }

    std::string resPath(tritonPath);
#ifdef _WIN32
    resPath += "\\Resources\\";
#else
    resPath += "/Resources/";
#endif
    _resourceLoader = new Triton::ResourceLoader(resPath.c_str());

    // Create an _environment for the water, with a flat-Earth coordinate system with Z
    // pointing up and using an OpenGL 2.0 capable context.
    _environment = new Triton::Environment();
    Triton::EnvironmentError err = _environment->Initialize(_geocentric ? Triton::WGS84_ZUP : Triton::FLAT_ZUP, Triton::OPENGL_2_0, _resourceLoader);
    if (err != Triton::SUCCEEDED) {
        osg::notify( osg::FATAL ) << "Failed to initialize Triton - is the resource path passed in to the ResourceLoader constructor valid?" << std::endl;
    }

    // Substitute your own license name and code, otherwise the app will terminate after
    // 5 minutes. Visit www.sundog-soft.com to purchase a license if you're so inclined.
    _environment->SetLicenseCode("Your license name", "Your license code");

    // Set up wind of 5 m/s blowing North
    Triton::WindFetch wf;
    wf.SetWind(5.0, 0.0);
    _environment->AddWindFetch(wf);

    // Set visibility
//    _environment->SetAboveWaterVisibility( 30000, Triton::Vector3( 0.5, 0.5, 0.5 ) );

    // Finally, create the _ocean object using the _environment we've created.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    _ocean = Triton::Ocean::Create(_environment, Triton::JONSWAP);

    if( !_ship && _ocean ) {
        Triton::WakeGeneratorParameters params;
        params.bowSprayOffset = 110.0;
        params.bowWaveOffset = 110.0;
        params.beamWidth = 20.0;
        params.sternWaveOffset = 0;
        params.propWashOffset = 50;
        params.sprayEffects = true;
        params.length = 110.0;
        params.propWash = true;
        params.numHullSprays = 0;
        _ship = new Triton::WakeGenerator( _ocean, params);
    }

    if (_ocean) {
        _rotorWash = new Triton::RotorWash(_ocean, 30.0, false, false);
    }

    // Set up an update callback so we can update the FFT model form the update thread
    if (_ocean) {
        setUpdateCallback(new TritonUpdateCallback(_ocean));
    }
}

// Clean up our resources
void TritonDrawable::Cleanup()
{
    if( _ship ) {
        delete _ship;
        _ship = NULL;
    }

    if (_rotorWash) {
        delete _rotorWash;
        _rotorWash = NULL;
    }

    if (_ocean) {
        delete _ocean;
        _ocean = NULL;
    }

    if (_environment) {
        delete _environment;
        _environment = NULL;
    }

    if (_resourceLoader) {
        delete _resourceLoader;
        _resourceLoader = NULL;
    }
}

void TritonDrawable::applyUserRotorCenterUniform(osg::State& state) const
{
    if (!_ocean || !_rotorWash) {
        return;
    }

    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
    if (!ext) {
        return;
    }

    Triton::Vector3 center = _rotorWash->GetPosition();
    if (_environment && _environment->IsGeocentric()) {
        const double* camPos = _environment->GetCameraPosition();
        center = Triton::Vector3(
            center.x - camPos[0],
            center.y - camPos[1],
            center.z - camPos[2]);
    }
    const Triton::Shaders programs[2] = { Triton::WATER_SURFACE, Triton::WATER_SURFACE_PATCH };

    for (int i = 0; i < 2; ++i) {
        const GLuint program = shaderHandleToProgram(_ocean->GetShaderObject(programs[i]));
        if (program == 0) {
            continue;
        }

        if (_userRotorCenterLoc[i] < 0) {
            _userRotorCenterLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorCenter");
        }

        if (_userRotorCenterLoc[i] < 0) {
            continue;
        }

        ext->glUseProgram(program);
        ext->glUniform3f(_userRotorCenterLoc[i],
            static_cast<GLfloat>(center.x),
            static_cast<GLfloat>(center.y),
            static_cast<GLfloat>(center.z));
    }

    ext->glUseProgram(0);
}


// Draw Triton _ocean
void TritonDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    osg::State & state = *renderInfo.getState();
    osg::ref_ptr< osg::StateSet > stateSet = new osg::StateSet();
    state.captureCurrentState( *stateSet );

    state.disableAllVertexArrays();
    state.dirtyAllVertexArrays();

    if (!_ocean) {
        const_cast< TritonDrawable *>( this )->Setup();
    }

    // Pass the final view and projection matrices into Triton.
    if (_environment) {
        _environment->SetCameraMatrix(renderInfo.getCurrentCamera()->getViewMatrix().ptr());
        _environment->SetProjectionMatrix( state.getProjectionMatrix().ptr() );
    }

    state.dirtyAllVertexArrays();

    // Now light and draw the ocean:
    if (_environment) {
        // The sun position is roughly where it is in our skybox texture:

        // Since this is a simple example we will just assume that Sun is the light from View light source
        osg::Light *light = renderInfo.getView() ? renderInfo.getView()->getLight() : NULL;
        // This is the light attached to View so there are no transformations above..
        // But in general case you would need to accumulate all transforms above the light into this matrix
        osg::Matrix lightLocalToWorldMatrix = osg::Matrix::identity();

        // If you don't know where the sun lightsource is attached and don't know its local to world matrix you may use
        // following elaborate scheme to grab the light source while drawing Triton ocean:
        // - Install cull callback to catch CullVisitor and record pointer to its associated RenderStage
        //   I was hoping RenderStage can be found from renderInfo in drawImplementation but I didn't figure how ...
        // - When TritonDrawable::drawImplementation is called all lights will be already applied to OpenGL
        //   then just find proper infinite directional light by scanning renderStage->PositionalStateContainer.
        // - Note that we canot scan for the lights inside cull because they may not be traversed before Triton drawable
        // - When you found interesting light source that can work as Sun, read its modelview matrix and lighting params
        //   Multiply light position by ( modelview * inverse camera view ) and pass this to Triton with lighting colors

        if ( light && light->getPosition().w() == 0 ) {
            osg::Vec4 ambient = light->getAmbient();
            osg::Vec4 diffuse = light->getDiffuse();
            osg::Vec4 position = light->getPosition();
            // Compute light position/direction in the world
            position = position * lightLocalToWorldMatrix;

            // Diffuse direction and color
            _environment->SetDirectionalLight( Triton::Vector3( position[0], position[1], position[2] ),
                                               Triton::Vector3( diffuse[0],  diffuse[1],  diffuse[2] ) );

            // Ambient color based on the zenith color in the cube map
            _environment->SetAmbientLight( Triton::Vector3( ambient[0], ambient[1], ambient[2] ) );

        }

        // Build transform from world orientation (Triton) space to native cube map space
        // Our SkyBox cube map has Yup face. So to use it as Zup cubemap we need to rotate it by 90 degrees around X axis

        osg::Matrix m =  *_cubeMapProjection * osg::Matrix::rotate( -osg::PI_2, osg::X_AXIS );

        Triton::Matrix3 transformFromYUpToZUpCubeMapCoords( m(0,0), m(0,1), m(0,2),
                m(1,0), m(1,1), m(1,2),
                m(2,0), m(2,1), m(2,2) );

        // Grab the cube map from our sky box and give it to Triton to use as an _environment map
        // GLenum texture = renderInfo.getState()->getLastAppliedTextureAttribute( _stage, osg::StateAttribute::TEXTURE );
        _environment->SetEnvironmentMap( (Triton::TextureHandle)
                                         _cubeMap->getTextureObject( state.getContextID() )->id(), transformFromYUpToZUpCubeMapCoords );

#if 1
        osg::Matrix & p = *_planarReflectionProjection;

        Triton::Matrix3 planarProjection( p(0,0), p(0,1), p(0,2),
                                          p(1,0), p(1,1), p(1,2),
                                          p(2,0), p(2,1), p(2,2) );

        _environment->SetPlanarReflectionMap( (Triton::TextureHandle)
                                              _planarReflectionMap->getTextureObject( state.getContextID() )->id(),
                                              planarProjection, 0.125  );
#endif

        if( _ship )
            _ship->Update
            ( Triton::Vector3( _shipPosition[0],_shipPosition[1],_shipPosition[2] ),
              Triton::Vector3( _shipDirection[0],_shipDirection[1],_shipDirection[2]),
              _shipVelocity, renderInfo.getView()->getFrameStamp()->getSimulationTime() );

        const double simTime = renderInfo.getView()->getFrameStamp()->getSimulationTime();

        if (_rotorWash && _rotorWashSource.valid()) {
            osg::NodePathList paths = _rotorWashSource->getParentalNodePaths();
            if (!paths.empty()) {
                const osg::Matrixd worldMat = osg::computeLocalToWorld(paths.front());

                const osg::Vec3d rotorWorld = osg::Vec3d(0.0, 0.0, 0.0) * worldMat;

                osg::Vec3d downWorld = osg::Matrixd::transform3x3(osg::Vec3d(0.0, 0.0, -1.0), worldMat);
                if (downWorld.length2() > 0.0) {
                    downWorld.normalize();
                } else {
                    downWorld.set(0.0, 0.0, -1.0);
                }
                _rotorWash->Update(
                    Triton::Vector3(rotorWorld.x(), rotorWorld.y(), rotorWorld.z()),
                    Triton::Vector3(downWorld.x(), downWorld.y(), downWorld.z()),
                    100.0,
                    simTime);
            }
        }

        applyUserRotorCenterUniform(state);

        // Draw the _ocean for the current time sample
        if (_ocean) {
            _ocean->Draw( simTime );
        }

        state.haveAppliedTextureAttribute(6, osg::StateAttribute::TEXTURE);
    }

//    state.dirtyAllAttributes();
//    state.dirtyAllModes();
    state.dirtyAllVertexArrays();

    /* Deprecated OSG code follows:
    state.dirtyColorPointer();
    state.dirtySecondaryColorPointer();
    state.dirtyTexCoordPointersAboveAndIncluding( 0 );
    state.dirtyVertexAttribPointersAboveAndIncluding( 0 );
    */

    state.setLastAppliedProgramObject( 0 );
    state.applyAttribute( new osg::Program() );

//    state.apply( stateSet );
}


