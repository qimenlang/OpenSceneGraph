// Triton OpenSceneGraph Sample Project
// Illustrates integration of Triton with an OpenSceneGraph application

// Copyright (c) 2011-2016 Sundog Software LLC. All rights reserved worldwide.

// The Triton.h header includes everything you need

#ifdef WIN32
#include <Windows.h>
#endif

#include <osg/GLExtensions>

#include "TritonDrawable.h"

// Create the Triton objects at startup, once we have a valid GL context in place
TritonDrawable::TritonDrawable( osg::TextureCubeMap * environmentMap, osg::Texture2D* heightMap, osg::Camera* heightMapCamera )
    :_resourceLoader(0)
    ,_environment(0)
    ,_ocean(0)
    ,_cubeMap(environmentMap)
    ,_heightMap(heightMap)
    ,_heightMapCamera(heightMapCamera)
{
    setDataVariance(osg::Object::DYNAMIC);
    setUseVertexBufferObjects(false);
    setUseDisplayList(false);
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

    // Set underwater visibility - this controls how transparent the water gets near shore.
    _environment->SetBelowWaterVisibility(20.0,Triton::Vector3(0.5,0.5,0.9));

    // Finally, create the _ocean object using the _environment we've created.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    _ocean = Triton::Ocean::Create(_environment, Triton::JONSWAP);

    // Set up an update callback so we can update the FFT model form the update thread
    if (_ocean) {
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

#ifdef USE_LOG_DEPTH_BUFFER

static void setUniformf(osg::GLExtensions* ext, GLuint program_id, const std::string& uniform_name, GLfloat v1)
{
    if (!program_id) {
        return;
    }
    ext->glUseProgram(program_id);
    GLuint location =  ext->glGetUniformLocation(program_id, uniform_name.c_str());
    if (-1 != location) {
        ext->glUniform1f(location, v1);
    }
    ext->glUseProgram(0);
}

static void SetFCoef(osg::GLExtensions* ext, Triton::ShaderHandle shader, float fcoef)
{
    ext->glUseProgram((GLuint)shader);
    GLuint loc = ext->glGetUniformLocation((GLuint)shader, "Fcoef");
    if (loc != -1) {
        ext->glUniform1f(loc, fcoef);
    }
    ext->glUseProgram(0);
}
#endif

// Draw Triton _ocean
void TritonDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    osg::State & state = *renderInfo.getState();

    state.disableAllVertexArrays();

    if (!_ocean) {
        static bool once = false;
        if (!once) {
            const_cast< TritonDrawable *>( this )->Setup();
            once = true;
        }
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
                                               Triton::Vector3(diffuse[0], diffuse[1], diffuse[2] ) );

            // Ambient color based on the zenith color in the cube map
            _environment->SetAmbientLight( Triton::Vector3( ambient[0], ambient[1], ambient[2] ) );

        }

        // Build transform from our cube map orientation space to native Triton orientation
        // See worldToCubeMap function used in SkyBox to orient sky texture so that sky is up and earth is down
        osg::Matrix m = osg::Matrix::rotate( osg::PI_2, osg::X_AXIS ); // = worldToCubeMap

        Triton::Matrix3 transformFromYUpToZUpCubeMapCoords( m(0,0), m(0,1), m(0,2),
                m(1,0), m(1,1), m(1,2),
                m(2,0), m(2,1), m(2,2) );

        if (_heightMap && _heightMapCamera) {
            osg::Texture::TextureObject* textureObject = _heightMap->getTextureObject(renderInfo.getContextID());
            if (textureObject) {
                const osg::Matrixd bias(0.5, 0.0, 0.0, 0.0,
                                        0.0, 0.5, 0.0, 0.0,
                                        0.0, 0.0, 0.5, 0.0,
                                        0.5, 0.5, 0.5, 1.0);

                osg::Matrixd HeightMapMatrix = _heightMapCamera->getViewMatrix() * _heightMapCamera->getProjectionMatrix() * bias;

                const Triton::Matrix4 worldToTextureCoords(HeightMapMatrix.ptr());
                _environment->SetHeightMap((Triton::TextureHandle)textureObject->id(),worldToTextureCoords);
            }
        }

        // Draw the _ocean for the current time sample
        if (_ocean) {

#ifdef USE_LOG_DEPTH_BUFFER
            osg::GLExtensions* ext = renderInfo.getState()->get<osg::GLExtensions>();
            double fovy, ar, nearplane, farplane;
            renderInfo.getCurrentCamera()->getProjectionMatrixAsPerspective(fovy, ar, nearplane, farplane);
            float Fcoef = 2.0f / log2(farplane + 1.0f);
            SetFCoef(ext, _ocean->GetShaderObject(Triton::WATER_SURFACE), Fcoef);
            SetFCoef(ext, _ocean->GetShaderObject(Triton::WATER_SURFACE_PATCH), Fcoef);
            SetFCoef(ext, _ocean->GetShaderObject(Triton::WAKE_SPRAY_PARTICLES), Fcoef);
            SetFCoef(ext, _ocean->GetShaderObject(Triton::SPRAY_PARTICLES), Fcoef);
#endif

            _ocean->Draw( renderInfo.getView()->getFrameStamp()->getSimulationTime() );
        }
    }

    state.dirtyAllVertexArrays();
}


