// Triton OpenSceneGraph Sample Project
// Illustrates integration of Triton with an OpenSceneGraph application

// Copyright (c) 2011-2012 Sundog Software LLC. All rights reserved worldwide.

// The Triton.h header includes everything you need

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
TritonDrawable::TritonDrawable( osg::TextureCubeMap * _environmentMap, osg::Fog* fog )
    :_resourceLoader(0)
    ,_environment(0)
    ,_ocean(0)
    ,_rotorWash(0)
    ,_cubeMap(_environmentMap)
    ,_fog(fog)
    ,_userRotorRadius(45.0f)
    ,_userRotorAmplitude(0.8f)
    ,_userRotorWaveCount(4.0f)
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseVertexBufferObjects(false);
    setUseDisplayList(false);

    _aboveWaterFogColor = Triton::Vector3(1.0f, 1.0f, 1.0f);
    _aboveWaterVisibility = 1E9;
    _belowWaterFogColor = Triton::Vector3(84.f / 255.f, 135.f / 255.f, 172.f / 255.f);
    _belowWaterVisibility = 1000.0;

    for (int i = 0; i < 2; ++i) {
        _userRotorCenterLoc[i] = -1;
        _userRotorRadiusLoc[i] = -1;
        _userRotorAmplitudeLoc[i] = -1;
        _userRotorWaveCountLoc[i] = -1;
        _userRotorEnabledLoc[i] = -1;
    }
}

// Clean up our resources
TritonDrawable::~TritonDrawable()
{
    Cleanup();
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
    std::cout << "Triton path: " << resPath << std::endl;
#ifdef _WIN32
    resPath += "\\Resources\\";
#else
    resPath += "/Resources/";
#endif
    _resourceLoader = new Triton::ResourceLoader(resPath.c_str());

    // Create an _environment for the water, with a flat-Earth coordinate system with Y
    // pointing up and using an OpenGL 2.0 capable context.
    _environment = new Triton::Environment();
    Triton::EnvironmentError err = _environment->Initialize(Triton::FLAT_ZUP, Triton::OPENGL_2_0, _resourceLoader);
    if (err != Triton::SUCCEEDED) {
        osg::notify( osg::FATAL ) << "Failed to initialize Triton - is the resource path passed in to the ResourceLoader constructor valid?" << std::endl;
    }

    // Substitute your own license name and code, otherwise the app will terminate after
    // 5 minutes. Visit www.sundog-soft.com to purchase a license if you're so inclined.
    _environment->SetLicenseCode("Your license name", "Your license code");

    // Calm open-ocean FFT: Beaufort 0 = calm, 1 = light air (see Environment::SimulateSeaState).
    // Replaces manual WindFetch; clears existing wind and sets sea state from Beaufort scale.
    const double beaufortScale = 1.0;
    const double windDirectionRadians = 0.0;
    _environment->SimulateSeaState(beaufortScale, windDirectionRadians);
    _environment->ClearSwells();

    // Set visibility
    _environment->SetAboveWaterVisibility(_aboveWaterVisibility, _aboveWaterFogColor);
    _environment->SetBelowWaterVisibility(_belowWaterVisibility, _belowWaterFogColor);

    // Finally, create the _ocean object using the _environment we've created.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    _ocean = Triton::Ocean::Create(_environment, Triton::JONSWAP);

    // Set up an update callback so we can update the FFT model form the update thread
    // Also enable underwater "god rays"
    if (_ocean) {
        _ocean->EnableGodRays(true);
        setUpdateCallback(new TritonUpdateCallback(_ocean));
        _rotorWash = new Triton::RotorWash(_ocean, 30.0, false, false);
    }
}

// Clean up our resources
void TritonDrawable::Cleanup()
{
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

void TritonDrawable::applyUserRotorDisplaceUniforms(osg::State& state) const
{
    if (!_ocean) {
        return;
    }

    osg::GLExtensions* ext = state.get<osg::GLExtensions>();
    if (!ext) {
        return;
    }

    const Triton::Shaders programs[2] = { Triton::WATER_SURFACE, Triton::WATER_SURFACE_PATCH };
    bool enabled = false;
    Triton::Vector3 center(0.0, 0.0, 0.0);

    if (_rotorWash) {
        center = _rotorWash->GetPosition();
        enabled = true;
    }

    for (int i = 0; i < 2; ++i) {
        const GLuint program = shaderHandleToProgram(_ocean->GetShaderObject(programs[i]));
        if (program == 0) {
            continue;
        }

        if (_userRotorCenterLoc[i] < 0) {
            _userRotorCenterLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorCenter");
            _userRotorRadiusLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorRadius");
            _userRotorAmplitudeLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorAmplitude");
            _userRotorWaveCountLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorWaveCount");
            _userRotorEnabledLoc[i] = ext->glGetUniformLocation(program, "trit_userRotorEnabled");
        }

        if (_userRotorCenterLoc[i] < 0) {
            continue;
        }

        ext->glUseProgram(program);

        ext->glUniform3f(_userRotorCenterLoc[i],
            static_cast<GLfloat>(center.x),
            static_cast<GLfloat>(center.y),
            static_cast<GLfloat>(center.z));

        if (_userRotorRadiusLoc[i] >= 0) {
            ext->glUniform1f(_userRotorRadiusLoc[i], _userRotorRadius);
        }
        if (_userRotorAmplitudeLoc[i] >= 0) {
            ext->glUniform1f(_userRotorAmplitudeLoc[i], _userRotorAmplitude);
        }
        if (_userRotorWaveCountLoc[i] >= 0) {
            ext->glUniform1f(_userRotorWaveCountLoc[i], _userRotorWaveCount);
        }
        if (_userRotorEnabledLoc[i] >= 0) {
            ext->glUniform1f(_userRotorEnabledLoc[i], enabled ? 1.0f : 0.0f);
        }
    }

    ext->glUseProgram(0);
}


// Draw Triton _ocean
void TritonDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    osg::State & state = *renderInfo.getState();


    state.disableAllVertexArrays();

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
        // - When you found interesting ligt source that can work as Sun, read its modelview matrix and lighting params
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

        // Ensure scene fog is consistent with the fog applied to the water
        if (_fog) {
            if (_ocean->IsCameraAboveWater()) {
                _fog->setColor(osg::Vec4(_aboveWaterFogColor.x, _aboveWaterFogColor.y, _aboveWaterFogColor.z, 1.0));
                _fog->setDensity(3.912 / _aboveWaterVisibility);
            } else {
                _fog->setColor(osg::Vec4(_belowWaterFogColor.x, _belowWaterFogColor.y, _belowWaterFogColor.z, 1.0));
                _fog->setDensity(3.912 / _belowWaterVisibility);
            }
        }

        // Build transform from our cube map orientation space to native Triton orientation
        // See worldToCubeMap function used in SkyBox to orient sky texture so that sky is up and earth is down
        osg::Matrix m = osg::Matrix::rotate( osg::PI_2, osg::X_AXIS ); // = worldToCubeMap

        Triton::Matrix3 transformFromYUpToZUpCubeMapCoords( m(0,0), m(0,1), m(0,2),
                m(1,0), m(1,1), m(1,2),
                m(2,0), m(2,1), m(2,2) );

        // Grab the cube map from our sky box and give it to Triton to use as an _environment map
        // GLenum texture = renderInfo.getState()->getLastAppliedTextureAttribute( _stage, osg::StateAttribute::TEXTURE );
        if (_cubeMap) {
            _environment->SetEnvironmentMap( (Triton::TextureHandle)
                                             _cubeMap->getTextureObject( state.getContextID() )->id(), transformFromYUpToZUpCubeMapCoords );
        }

        if (_ocean) {
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

            applyUserRotorDisplaceUniforms(state);
            _ocean->Draw(simTime);
        }
    }

    state.dirtyAllVertexArrays();
}

