#define _CRT_SECURE_NO_WARNINGS

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
#include <osgDB/WriteFile>
#include <osg/MatrixTransform>
#include <osg/FrontFace>
#include <osg/Material>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Shape>

#include <iostream>

#include "SkyBox.h"
#include "PlanarReflection.h"
#include "TritonDrawable.h"

#define WATER_MASK ( 0x80000000 )
#define FALLBACK_MODEL "boat.3ds"


osg::Camera * CreateTextureQuadOverlay( osg::Texture * texture, float x, float y, float w, float h )
{
    osg::Camera * camera = new osg::Camera;

    // set up the background color and clear mask.
    camera->setClearMask( GL_DEPTH_BUFFER_BIT );

    // set viewport
    camera->setProjectionMatrixAsOrtho2D( 0, 1, 0, 1 );
    camera->setViewMatrix( osg::Matrix::identity() );
    camera->setReferenceFrame( osg::Transform::ABSOLUTE_RF );

    osg::Geode * geode = new osg::Geode();
    camera->addChild( geode );

    osg::Geometry * background = osg::createTexturedQuadGeometry( osg::Vec3( x,y,0 ),osg::Vec3( w,0,0 ), osg::Vec3( 0,h,0 ) );
    geode->addDrawable( background );

    osg::Vec4Array* colors = new osg::Vec4Array;
    colors->push_back( osg::Vec4( 1,1,1,0.3 ) );
    background->setColorArray( colors );
    background->setColorBinding( osg::Geometry::BIND_OVERALL );
    background->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );
    background->getOrCreateStateSet()->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    osg::Geometry * quad = osg::createTexturedQuadGeometry( osg::Vec3( x,y,0 ),osg::Vec3( w,0,0 ), osg::Vec3( 0,h,0 ) );
    geode->addDrawable( quad );
    quad->getOrCreateStateSet()->setTextureAttributeAndModes( 0, texture );
    quad->getOrCreateStateSet()->setMode( GL_BLEND, osg::StateAttribute::ON );


    // set the camera to render before the main camera.
    camera->setRenderOrder(osg::Camera::POST_RENDER);

    return camera;
}

osg::Node * LoadModel( osg::ArgumentParser & arguments )
{
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

    // load the data
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) {
        std::cout << arguments.getApplicationName() << ": No external model loaded - Loading default " << FALLBACK_MODEL << " model. " << std::endl;
        loadedModel = osgDB::readNodeFile( resPath + FALLBACK_MODEL );
        if( !loadedModel )
            std::cout << arguments.getApplicationName() << ": Failure loading default " << FALLBACK_MODEL << "model. " << std::endl;
    }

    // Our fallback boat model has its origin located too high - translate it
    if( loadedModel && loadedModel->getName().rfind( "boat.3ds" ) != std::string::npos ) {
        osg::ref_ptr<osg::Material> material = new osg::Material;
        material->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.3, 0.3, 0.3, 1.0));
        material->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.5, 0.5, 0.5, 1.0));
        loadedModel->getOrCreateStateSet()->setAttribute(material.get(), osg::StateAttribute::OVERRIDE|osg::StateAttribute::ON);
        osg::ref_ptr< osg::MatrixTransform > transform = new osg::MatrixTransform;
        transform->setMatrix( osg::Matrix::rotate( TRITON_PI, osg::Z_AXIS ) * osg::Matrix::translate( 0, 0, 45 ) * osg::Matrix::scale(0.3048, 0.3048, 0.3048));
        transform->addChild( loadedModel );
        loadedModel = transform;
    }

    return loadedModel.release();
}

osg::Node * CreateWorldGraph( osg::Node * loadedModel, const osg::Matrix & localToWorld )
{
    osg::ref_ptr< osg::MatrixTransform > world = new osg::MatrixTransform;
    world->setMatrix( localToWorld );
    world->addChild( loadedModel );
    return world.release();
}

osg::Node * CreateMirroredWorldGraph( osg::Node * loadedModel, const osg::Matrix & localToWorld )
{
    osg::ref_ptr< osg::MatrixTransform > world = new osg::MatrixTransform;
    world->setMatrix( localToWorld );

    // Add transform inverting height coordinates
    osg::ref_ptr< osg::MatrixTransform > mirror = new osg::MatrixTransform;
    world->addChild( mirror );
    mirror->setMatrix( osg::Matrix::scale( 1,1,-1) );
    // Make sure fron faces will be not culled out with changed orientation
    mirror->getOrCreateStateSet()->setAttributeAndModes( new osg::FrontFace( osg::FrontFace::CLOCKWISE ) );

    // Add clip node to cut off parts of the scene below the water surface
    osg::ref_ptr< osg::ClipNode > clipNode = new osg::ClipNode();
    // Set clip plane to cut off all whats below the water surface
    // Note that the w coordinate of the clip plane is 1.0 instead of 0 - this bumps the
    // clip plane a bit, in order to prevent a gap between the ship and
    // its reflection when waves are higher. You may need to fine-tune this for your own
    // application.
    clipNode->addClipPlane( new osg::ClipPlane( 0, osg::Plane( 0,0,1,1) ) );
    mirror->addChild( clipNode );

    // Finally add the model
    clipNode->addChild( loadedModel );
    return world.release();
}

// Set initial camera position little higher than sea level to avoid annoying ocean surface clipping at startup
void AdjustCameraHomePosition( osgViewer::Viewer & viewer, osg::Node * world, const osg::Matrix & localToWorld )
{
    osg::Vec3 center( 0,0,0 );
    osg::Vec3 viewVector( -500,-500,-200 );

    if( world ) {
        center = world->getBound().center();
//        if( center[2] < 0 ) center[2] = 0;

        float length = viewVector.normalize();
        viewVector *= osg::maximum( world->getBound().radius() * 2, length );
    }

    viewer.getCameraManipulator()->setHomePosition( center - viewVector, center, osg::Z_AXIS * localToWorld );
}


class MoveModelCallback: public osg::NodeCallback
{
public:
    MoveModelCallback( TritonDrawable * tritonDrawable, osg::RefMatrix* localToWorld ) {
        _tritonDrawable = tritonDrawable;
        _localToWorld = localToWorld;
        _shipX = _shipY = _shipZ = 0;
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv) {
        const double shipVelocity = 10.0;
        const double shipRadius = 500.0;

        double circumference = 2.0 * TRITON_PI * 2.0 * shipRadius;
        double halfShipT = (110.0 / circumference) * (2.0 * TRITON_PI);
        double circuitTime = circumference / shipVelocity;

        double nowS = (double)nv->getFrameStamp()->getSimulationTime();
        double t = fmod(nowS, circuitTime);
        t /= circuitTime;
        t *= (2.0 * TRITON_PI);

        _shipZ = 0;

        _shipX = shipRadius * cos(t);
        _shipY = shipRadius * sin(t);

        float shipTheta = t;

        osg::Vec3d wsShipPos = osg::Vec3d(_shipX, _shipY, _shipZ) * (*_localToWorld);

        _shipX = shipRadius * cos(t + halfShipT);
        _shipY = shipRadius * sin(t + halfShipT);

        osg::Vec3d wsBowPos = osg::Vec3d(_shipX, _shipY, _shipZ) * (*_localToWorld);
        osg::Vec3d direction = wsBowPos - wsShipPos;
        direction.normalize();

        // Let Triton know where the ship moves
        _tritonDrawable->UpdateShipPos( wsShipPos.x(), wsShipPos.y(), wsShipPos.z(),
                                        direction.x(), direction.y(), direction.z(), shipVelocity );

        // And move the ship
        osg::MatrixTransform * mt = dynamic_cast< osg::MatrixTransform * >( node );
        if( mt )
            mt->setMatrix(
                osg::Matrix::rotate( shipTheta, osg::Z_AXIS ) *
                osg::Matrix::translate( osg::Vec3( _shipX, _shipY, _shipZ ) ) );



        // note, callback is responsible for scenegraph traversal so
        // they must call traverse(node,nv) to ensure that the
        // scene graph subtree (and associated callbacks) are traversed.
        traverse(node,nv);
    }

protected:
    osg::ref_ptr< TritonDrawable > _tritonDrawable;
    osg::ref_ptr< osg::RefMatrix > _localToWorld;
    double                         _shipX, _shipY, _shipZ;
};



int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is an OpenSceneGraph example demonstrating use of planar reflections with Triton SDK.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] modelfile ...");
    arguments.getApplicationUsage()->addCommandLineOption("--geocentric","Use WGS_84 ellipsoid geocentric model. Default if flat model.");

    osgViewer::Viewer viewer(arguments);
    viewer.setUpViewInWindow(20, 30, 1024, 768);

    unsigned int helpType = 0;
    if ((helpType = arguments.readHelpType())) {
        arguments.getApplicationUsage()->write(std::cout, helpType);
        return 1;
    }

    bool geocentric = false;
    while( arguments.read( "--geocentric" ) )
        geocentric = true;

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

    // force multi-threaded for testing
    viewer.setThreadingModel(osgViewer::ViewerBase::CullThreadPerCameraDrawThreadPerContext);

    osg::ref_ptr< osg::RefMatrix > localToWorld = new osg::RefMatrix;
    osg::ref_ptr< osg::RefMatrix > worldToLocal = new osg::RefMatrix;

    if( geocentric ) {
        double lat = osg::PI_4;
        double lon = -osg::PI_4;
        double alt = 0;
        osg::ref_ptr< osg::EllipsoidModel > ellipsoidModel = new osg::EllipsoidModel;
        ellipsoidModel->computeLocalToWorldTransformFromLatLongHeight( lat, lon, alt, *localToWorld );
        worldToLocal->set( osg::Matrix::inverse( *localToWorld ) );
    }

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors()) {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    // Setup scene root. It will be a parent for world model and reflection camera which will have world model attached as well
    osg::ref_ptr< osg::Group > scene = new osg::Group;

    // Load external model
    osg::Node * loadedModel = LoadModel( arguments );

    osg::ref_ptr< osg::MatrixTransform > moveModelTransform = new osg::MatrixTransform;

    moveModelTransform->addChild( loadedModel );

    loadedModel = moveModelTransform;

    osg::Node * world = CreateWorldGraph( moveModelTransform, *localToWorld );
    scene->addChild( world );

    osg::Node * worldMirrored = CreateMirroredWorldGraph( loadedModel, *localToWorld );

    // Setup planar reflection camera
    PlanarReflection *planarReflectionCamera = new PlanarReflection( /*parent camera*/ viewer.getCamera() );
    scene->addChild( planarReflectionCamera );
    planarReflectionCamera->addChild( worldMirrored );

    // There are two options:
    // We may pass only loaded model to reflection camera
    // or
    // we may pass whole world including waters -
    // but then we need to make sure water will not get rendered to reflectionMap.
    // Thats why we set camera cull mask to skip rendering water surface to reflection map.
    planarReflectionCamera->setCullMask( ~WATER_MASK );

    // Environment map created by Skybox and passed to Triton to make reflections on waves
    osg::TextureCubeMap * environmentMap = NULL;

    // Set initial camera position little higher than sea level to avoid annoying ocean surface clipping at startup
    AdjustCameraHomePosition( viewer, world, *localToWorld );

    // Create Sky
    world->asGroup()->addChild( createSkyBox( environmentMap ) );

    // Create Triton Waters
    osg::Geode * ocean = new osg::Geode;
    scene->addChild( ocean );

    // Set water node mask to limit rendering in planar Reflection camera
    // If water surface was renderd in reflection camera it would occlude reflected terrain
    ocean->setNodeMask( WATER_MASK );

    // Additionaly add overlay showing rendered texture reflection in lower right corner
    scene->addChild( CreateTextureQuadOverlay( planarReflectionCamera->getTexture(), 0.65, 0.05, 0.3, 0.3 ) );

    // Set light position roughly matching sun location in environment cube map (this matching is done for flat model)
    osg::Vec4 position(-1.0 / sqrt(3.0), -1.0 / sqrt(3.0), 1.0 / sqrt(3.0), 0);
    osg::Vec4 ambient( osg::Vec3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0), 1.0 );
    osg::Vec4 diffuse( osg::Vec3(0.8, 0.8, 0.8), 1.0 );

    viewer.setLightingMode( osg::View::SKY_LIGHT );
    viewer.getLight()->setPosition( position );
    viewer.getLight()->setAmbient( ambient );
    viewer.getLight()->setDiffuse( diffuse );

    // Make sure ocean surface is not clipped by automatically computed near far
    viewer.getCamera()->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR );

    // Alternatively one may attach clampProjectionCallback to CullVisition and adjust
    // near and far to the distances from camera position to closest and furthest point on the ocean surface

    // Read near far and fov range
    double l,r,t,b,n,f;
    viewer.getCamera()->getProjectionMatrixAsFrustum( l,r,t,b,n,f );

    // Adjust near far to range 1..50 km and increase fov twice
    viewer.getCamera()->setProjectionMatrixAsFrustum( 2*l/n,2*r/n,2*t/n,2*b/n,n/n,50000 );

    viewer.setSceneData( scene );

    // Cube carrying rotor wash; downwash is emitted from its center
    osg::ref_ptr<osg::MatrixTransform> rotorCube = new osg::MatrixTransform;
    rotorCube->setMatrix(osg::Matrix::translate(0.0, 0.0, 8.0));

    osg::ref_ptr<osg::Geode> cubeGeode = new osg::Geode;
    osg::ref_ptr<osg::ShapeDrawable> cubeShape = new osg::ShapeDrawable(new osg::Box(osg::Vec3(), 1.0));
    cubeShape->setColor(osg::Vec4(0.8f, 0.2f, 0.2f, 1.0f));
    cubeGeode->addDrawable(cubeShape.get());

    osg::ref_ptr<osg::Material> cubeMaterial = new osg::Material;
    cubeMaterial->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.8f, 0.2f, 0.2f, 1.0f));
    cubeGeode->getOrCreateStateSet()->setAttributeAndModes(cubeMaterial.get());

    rotorCube->addChild(cubeGeode.get());
    scene->addChild(rotorCube.get());

    osg::ref_ptr<TritonDrawable> tritonDrawable =
        new TritonDrawable( geocentric,
                            environmentMap, worldToLocal,
                            planarReflectionCamera->getTexture(),
                            planarReflectionCamera->getTextureProjectionMatrix() );
    tritonDrawable->setRotorWashSourceNode(rotorCube.get());

    ocean->addDrawable( tritonDrawable );

    moveModelTransform->addUpdateCallback( new MoveModelCallback( tritonDrawable, localToWorld ) );

    viewer.realize();

    return viewer.run();
}
