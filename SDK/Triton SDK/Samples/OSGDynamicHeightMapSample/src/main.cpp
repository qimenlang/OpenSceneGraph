
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <osgGA/TrackballManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/SphericalManipulator>

#include <osgDB/ReadFile>

#include <iostream>
#include <math.h>

#include "SkyBox.h"
#include "TritonDrawable.h"

#define HEIGHT_MAP_AREA 20000

osg::ref_ptr<osg::Texture2D>    g_HeightMap;
osg::ref_ptr<osg::Camera>       g_HeightMapCamera;

osg::Camera* createHUDForHeightMapDisplay()
{
    osg::GraphicsContext::WindowingSystemInterface* wsi = osg::GraphicsContext::getWindowingSystemInterface();
    if (!wsi) {
        osg::notify(osg::NOTICE)<<"Error, no WindowSystemInterface available, cannot create windows."<<std::endl;
        return 0;
    }

    osg::Camera* hudCamera = new osg::Camera;

    unsigned int width, height;
    wsi->getScreenResolution(osg::GraphicsContext::ScreenIdentifier(0), width, height);

    hudCamera->setProjectionMatrix(osg::Matrix::ortho2D(0,width,0,height));
    hudCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    hudCamera->setViewMatrix(osg::Matrix::identity());
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER);
    hudCamera->setAllowEventFocus(false);

    osg::Vec3Array* vertices = new osg::Vec3Array;
    vertices->push_back(osg::Vec3(100,100,-0.1));
    vertices->push_back(osg::Vec3(400,100,-0.1));
    vertices->push_back(osg::Vec3(400,400,-0.1));
    vertices->push_back(osg::Vec3(100,400,-0.1));

    osg::Geometry* geometry = new osg::Geometry;
    geometry->setVertexArray(vertices);

    osg::Vec3Array* normals = new osg::Vec3Array;
    normals->push_back(osg::Vec3(0.0f,0.0f,1.0f));
    geometry->setNormalArray(normals);
    geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);

    osg::Vec2Array* uvs = new osg::Vec2Array;
    uvs->push_back(osg::Vec2(0,0));
    uvs->push_back(osg::Vec2(1,0));
    uvs->push_back(osg::Vec2(1,1));
    uvs->push_back(osg::Vec2(0,1));
    geometry->setTexCoordArray(0,uvs);
    geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0,g_HeightMap);

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back(osg::Vec4(1.f,1.f,1.f,1.f));
    geometry->setColorArray(colors);
    geometry->setColorBinding(osg::Geometry::BIND_OVERALL);

    geometry->addPrimitiveSet(new osg::DrawArrays(GL_QUADS,0,4));

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geometry);

    hudCamera->addChild(geode);

    return hudCamera;
}

class FollowCameraForHeightMapGenerationUpdateNodeCallback : public osg::NodeCallback
{
public:
    FollowCameraForHeightMapGenerationUpdateNodeCallback(osg::Camera* camera)
        : _camera(camera) {
    }
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
        osg::Vec3 eye;
        osg::Vec3 center;
        osg::Vec3 up;

        _camera->getViewMatrixAsLookAt(eye,center,up);

        osg::Vec3 d = center-eye;
        d.normalize();

        osg::Vec3 newEye = eye+d*(HEIGHT_MAP_AREA);

        osg::Vec3 heightMapCameraEye = osg::Vec3(newEye.x(),newEye.y(),2000);
        osg::Vec3 heightMapCameraUp = osg::Vec3(0,1,0);
        osg::Vec3 heightMapCameraCenter = osg::Vec3(newEye.x(),newEye.y(),0);

        osg::Camera* heightMapCamera = dynamic_cast<osg::Camera*>(node);
        if (!heightMapCamera) return;

        osg::Matrixd mx = osg::Matrix::inverse(_camera->getViewMatrix());
        osg::Quat q = mx.getRotate();

        heightMapCamera->setViewMatrixAsLookAt(
            heightMapCameraEye,
            heightMapCameraCenter,
            q*heightMapCameraUp);

    }
protected:
    osg::Camera*    _camera;
};

/** This shader assumes your terrain's vertices are in world coordinates
    and Z is "up". If this isn't true in your case, you may need to
    pass additional data into this shader in order to figure out the
    height above sea level at each vertex. If you have model matrices applied to your
    terrain, you may need to pass in the inverse view matrix in order to
    be able to derive world coordinates from vertex coordinates.

    If you are in a geocentric / ECEF system (ie, osgEarth,) here's some
    sample code for properly calculating the altitude above sea level for
    a given vertex:

    const char *HeightMapVertSource =
    {
      // Need to populate this with inverse of the height map camera's view matrix:
      "uniform mat4 viewMatrixInverse;\n"
      // Need to populate this with the axis lengths of your Earth model
      "uniform vec3 earthRadii;\n"
      "varying float height;\n"

      // Get altitude above or below MSL for a ECEF point.
      "float altFromECEF(in vec3 ecef)\n"
      "{\n"
      "   vec3 v = ecef / earthRadii;\n"
      "   vec3 unitSphereIntersection = normalize(v);\n"
      "   vec3 ellipsoidIntersection = unitSphereIntersection * earthRadii;\n"
      "   float alt = length(ecef) - length(ellipsoidIntersection);\n"
      "   return alt;\n"
      "}\n"

      "void main()\n"
      "{\n"
      "   vec4 worldVert = viewMatrixInverse * gl_ModelViewMatrix * gl_Vertex;\n"
      "   height = altFromECEF(worldVert.xyz);\n"
      "   gl_Position = ftransform();\n"
      "   gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n"
      "}\n"

    For even greater accuracy, you would divide the computed altitude by the cosine of the
    angle between the local "up" vector and the normalized vector pointing from the center
    of the Earth to your position. "Up" and "direction from the center of the Earth" aren't
    quite the same thing, and some small error results from this.
    */

const char *HeightMapVertSource = {
    "varying float height;\n"
    "void main()\n"
    "{\n"
    "   vec4 worldVert = gl_Vertex;\n"
    "   height = worldVert.z;\n"
    "   gl_Position = gl_ModelViewProjectionMatrix * worldVert;\n"
    "   gl_ClipVertex = gl_ModelViewMatrix * worldVert;\n"
    "}\n"
};

const char *HeightMapFragSource = {
    "varying float height;\n"
    "void main()\n"
    "{\n"
    "   gl_FragColor = vec4(height, height, height, 1.0);\n"
    "}\n"
};

const char *VertSource = {
#ifdef USE_LOG_DEPTH_BUFFER
    "uniform float Fcoef; \n"
    "varying float flogz; \n"
#endif
    "void main()\n"
    "{\n"
    "   vec4 worldVert = gl_Vertex;\n"
    "	vec4 clipVertex =  gl_ModelViewMatrix * worldVert;\n"
    "   gl_ClipVertex = clipVertex;\n"
    "   gl_Position = gl_ProjectionMatrix * clipVertex;\n"
    "	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;   \n"
#ifdef USE_LOG_DEPTH_BUFFER
    "   gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef - 1.0) * gl_Position.w;\n"
    // NOTE: In some terrains, multiplying in gl_Position.w results in sorting errors.
    // If things don't look right, try replacing the line above with:
    // "   gl_Position.z = (log2(max(1e-6, 1.0 + gl_Position.w)) * Fcoef - 1.0);\n"
    "   flogz = 1.0 + gl_Position.w;\n"
#endif
    "}\n"
};

const char *FragSource = {
#ifdef USE_LOG_DEPTH_BUFFER
    "uniform float Fcoef;\n"
    "varying float flogz;\n"
#endif
    "uniform sampler2D baseTexture;\n"
    "void main()\n"
    "{\n"
    "   vec4 color = texture2D( baseTexture, gl_TexCoord[0].xy );\n"
    "	gl_FragColor = color; \n"
#ifdef USE_LOG_DEPTH_BUFFER
    "   gl_FragDepth = log2(flogz) * Fcoef * 0.5;\n"
#endif
    "}\n"
};

osg::Camera* createHeightMapCamera(osg::Camera* mainCamera, osg::Node* model)
{
    g_HeightMap = new osg::Texture2D();
    g_HeightMap->setTextureSize(2048, 2048);
#if 1
    g_HeightMap->setInternalFormat(GL_LUMINANCE32F_ARB);
    g_HeightMap->setSourceFormat(GL_LUMINANCE);
#else
    g_HeightMap->setInternalFormat(GL_RGBA);
#endif
    g_HeightMap->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    g_HeightMap->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);

    g_HeightMap->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP_TO_EDGE);
    g_HeightMap->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP_TO_EDGE);

    // Create its camera and render to it
    g_HeightMapCamera = new osg::Camera;
    g_HeightMapCamera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    g_HeightMapCamera->setClearColor(osg::Vec4(-1000.0, -1000.0, -1000.0, 1.0f));
    g_HeightMapCamera->setViewport(0, 0, 2048, 2048);
    g_HeightMapCamera->setRenderOrder(osg::Camera::PRE_RENDER);
    g_HeightMapCamera->attach(osg::Camera::COLOR_BUFFER, g_HeightMap);
    g_HeightMapCamera->addChild(model);
    g_HeightMapCamera->setUpdateCallback(new FollowCameraForHeightMapGenerationUpdateNodeCallback(mainCamera));
    g_HeightMapCamera->setProjectionMatrix(osg::Matrix::ortho2D(-HEIGHT_MAP_AREA,HEIGHT_MAP_AREA,-HEIGHT_MAP_AREA,HEIGHT_MAP_AREA));
    g_HeightMapCamera->setCullingActive(false);
    g_HeightMapCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF_INHERIT_VIEWPOINT);
    g_HeightMapCamera->setImplicitBufferAttachmentMask(0, 0);
    g_HeightMapCamera->setRenderTargetImplementation( osg::Camera::FRAME_BUFFER_OBJECT );
    g_HeightMapCamera->getOrCreateStateSet()->setMode(GL_ALPHA_TEST,
            osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osg::Program> heightMapProgram = new osg::Program;
    heightMapProgram->addShader(new osg::Shader(osg::Shader::VERTEX, HeightMapVertSource));
    heightMapProgram->addShader(new osg::Shader(osg::Shader::FRAGMENT, HeightMapFragSource));

    osg::StateSet *stateSet = g_HeightMapCamera->getOrCreateStateSet();
    stateSet->setAttributeAndModes(heightMapProgram.get(),osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    return g_HeightMapCamera.get();
}

#ifdef USE_LOG_DEPTH_BUFFER
struct SetFarPlaneUniformCallback : public osg::Camera::DrawCallback {
    osg::ref_ptr<osg::Uniform>              _uniform;

    SetFarPlaneUniformCallback(osg::Uniform* uniform) {
        _uniform = uniform;
    }

    static double Log2(double n) {
        return log(n) / log(2.0);
    }

    void operator () (osg::RenderInfo& renderInfo) const {
        const osg::Matrix& proj = renderInfo.getCurrentCamera()->getProjectionMatrix();

        float vfov, ar, n, f;
        proj.getPerspective(vfov, ar, n, f);
        float fc = (float)(2.0f / Log2(f + 1.0f));
        _uniform->set(fc);

    }
};
#endif

int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is simple OpenSceneGraph example demonstrating integration with Triton SDK.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] modelfile ...");

    osgViewer::Viewer viewer(arguments);

    viewer.setUpViewInWindow(50, 50, 800, 600);

    unsigned int helpType = 0;
    if ((helpType = arguments.readHelpType())) {
        arguments.getApplicationUsage()->write(std::cout, helpType);
        return 1;
    }

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors()) {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    // set up the camera manipulators.
    {
        osg::ref_ptr<osgGA::KeySwitchMatrixManipulator> keyswitchManipulator = new osgGA::KeySwitchMatrixManipulator;

        keyswitchManipulator->addMatrixManipulator( '1', "Trackball", new osgGA::TrackballManipulator() );
        keyswitchManipulator->addMatrixManipulator( '2', "Flight", new osgGA::FlightManipulator() );
        keyswitchManipulator->addMatrixManipulator( '3', "Drive", new osgGA::DriveManipulator() );
        keyswitchManipulator->addMatrixManipulator( '4', "Terrain", new osgGA::TerrainManipulator() );
        keyswitchManipulator->addMatrixManipulator( '5', "Orbit", new osgGA::OrbitManipulator() );
        keyswitchManipulator->addMatrixManipulator( '6', "FirstPerson", new osgGA::FirstPersonManipulator() );
        keyswitchManipulator->addMatrixManipulator( '7', "Spherical", new osgGA::SphericalManipulator() );

        std::string pathfile;
        double animationSpeed = 1.0;
        while(arguments.read("--speed",animationSpeed) ) {}
        char keyForAnimationPath = '8';
        while (arguments.read("-p",pathfile)) {
            osgGA::AnimationPathManipulator* apm = new osgGA::AnimationPathManipulator(pathfile);
            if (apm || !apm->valid()) {
                apm->setTimeScale(animationSpeed);

                unsigned int num = keyswitchManipulator->getNumMatrixManipulators();
                keyswitchManipulator->addMatrixManipulator( keyForAnimationPath, "Path", apm );
                keyswitchManipulator->selectMatrixManipulator(num);
                ++keyForAnimationPath;
            }
        }

        viewer.setCameraManipulator( keyswitchManipulator.get() );
    }

    // add the state manipulator
    viewer.addEventHandler( new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()) );

    // add the thread model handler
    viewer.addEventHandler(new osgViewer::ThreadingHandler);

    // add the window size toggle handler
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);

    // add the stats handler
    viewer.addEventHandler(new osgViewer::StatsHandler);

    // add the help handler
    viewer.addEventHandler(new osgViewer::HelpHandler(arguments.getApplicationUsage()));

    // add the record camera path handler
    viewer.addEventHandler(new osgViewer::RecordCameraPathHandler);

    // add the LOD Scale handler
    viewer.addEventHandler(new osgViewer::LODScaleHandler);

    // add the screen capture handler
    viewer.addEventHandler(new osgViewer::ScreenCaptureHandler);

    // force multi-threading for testing purposes
    viewer.setThreadingModel(osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext);

#if 0
    // load the data
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) {
        std::cout << arguments.getApplicationName() <<": No model loaded" << std::endl;
    }
#else
    const char *tritonPath = getenv("TRITON_PATH");
    if (!tritonPath) {
        printf("Can't find Triton; set the TRITON_PATH environment variable ");
        printf("to point to the directory containing the SDK.\n");
        exit(0);
    }

    std::string resPath(tritonPath);
#ifdef _WIN32
    resPath += "\\Samples\\";
#else
    resPath += "/Samples/";
#endif

    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFile(resPath+"terrain.ive");
#endif

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors()) {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    osg::ref_ptr< osg::Group > scene = new osg::Group;
    // Attach external model
    if( loadedModel.valid() )
        scene->addChild( loadedModel );

    // Set initial camera postion little higher than sea level to avoid ocean surface clipping
    {
        osg::Vec3 center( 0,0,0 );
        osg::Vec3 viewVector( -20,-20,-10 );

        if( loadedModel.valid() ) {
            center = loadedModel->getBound().center();
            if( center[2] < 0 ) center[2] = 0;

            float length = viewVector.normalize();
            viewVector *= osg::maximum( loadedModel->getBound().radius() * 2, length );
        }

        viewer.getCameraManipulator()->setHomePosition( center - viewVector, center, osg::Z_AXIS );
    }

    viewer.setSceneData( scene );

    // Environment map created by Skybox and passed to Triton to make reflections on waves
    osg::TextureCubeMap * environmentMap = NULL;

    // Create Sky
    scene->addChild( createSkyBox( environmentMap ) );

    // Create and add the heightMap texture,
    // HeightMap camera, and the hud display
    // for the texture
    viewer.getSceneData()->asGroup()->addChild(createHeightMapCamera(viewer.getCamera(),loadedModel.get()));
    viewer.getSceneData()->asGroup()->addChild(createHUDForHeightMapDisplay());

    // Create Triton Waters
    osg::Geode * geode = new osg::Geode;
    scene->addChild( geode );
    geode->addDrawable( new TritonDrawable( environmentMap, g_HeightMap.get(), g_HeightMapCamera.get() ) );

    // Set the same Light as Triton OpenGL example ...
    osg::Vec4 position(0, 1.0 / sqrt(2.0), 1.0 / sqrt(2.0), 0);
    osg::Vec4 ambient( osg::Vec3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0) * 1.5, 1.0 );
    osg::Vec4 diffuse( osg::Vec3(1.0, 1.0, 1.0 ), 1.0);

    viewer.setLightingMode( osg::View::SKY_LIGHT );
    viewer.getLight()->setPosition( position );
    viewer.getLight()->setAmbient( ambient );
    viewer.getLight()->setDiffuse( diffuse );

    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(new osg::Shader(osg::Shader::VERTEX, VertSource));
    program->addShader(new osg::Shader(osg::Shader::FRAGMENT, FragSource));

    osg::StateSet *stateSet = loadedModel->getOrCreateStateSet();

#ifdef USE_LOG_DEPTH_BUFFER
    osg::ref_ptr<osg::Uniform> fCoefUniform = new osg::Uniform("Fcoef", 0.0f);
    stateSet->addUniform(fCoefUniform);
    viewer.getCamera()->setPreDrawCallback(new SetFarPlaneUniformCallback(fCoefUniform.get()));
#endif

    stateSet->setAttributeAndModes(program.get(),osg::StateAttribute::ON);

    viewer.realize();

    return viewer.run();

}
