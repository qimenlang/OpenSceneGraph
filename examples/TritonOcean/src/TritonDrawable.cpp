// Triton OpenSceneGraph Sample Project
// Illustrates integration of Triton with an OpenSceneGraph application

// Copyright (c) 2011-2012 Sundog Software LLC. All rights reserved worldwide.

// The Triton.h header includes everything you need

#include "TritonDrawable.h"


// Create the Triton objects at startup, once we have a valid GL context in place
TritonDrawable::TritonDrawable( osg::TextureCubeMap * _environmentMap, osg::Fog* fog )
    :_resourceLoader(0)
    ,_environment(0)
    ,_ocean(0)
    ,_cubeMap(_environmentMap)
    ,_fog(fog)
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseVertexBufferObjects(false);
    setUseDisplayList(false);

    _aboveWaterFogColor = Triton::Vector3(1.0f, 1.0f, 1.0f);
    _aboveWaterVisibility = 1E9;
    _belowWaterFogColor = Triton::Vector3(84.f / 255.f, 135.f / 255.f, 172.f / 255.f);
    _belowWaterVisibility = 1000.0;
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

    // Set up wind of 10 m/s blowing North
    Triton::WindFetch wf;
    wf.SetWind(10.0, 0.0);
    _environment->AddWindFetch(wf);

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
    }
}

// Clean up our resources
void TritonDrawable::Cleanup()
{
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

        // Draw the _ocean for the current time sample
        if (_ocean) {
            _ocean->Draw( renderInfo.getView()->getFrameStamp()->getSimulationTime() );
        }
    }

    state.dirtyAllVertexArrays();
}


