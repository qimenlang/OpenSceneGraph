
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
#include <osg/Fog>

#include <iostream>

#include "SkyBox.h"
#include "TritonDrawable.h"

int main(int argc, char** argv)
{
    // use an ArgumentParser object to manage the program arguments.
    osg::ArgumentParser arguments(&argc,argv);

    arguments.getApplicationUsage()->setApplicationName(arguments.getApplicationName());
    arguments.getApplicationUsage()->setDescription(arguments.getApplicationName()+" is simple OpenSceneGraph example demonstrating integration with Triton SDK.");
    arguments.getApplicationUsage()->setCommandLineUsage(arguments.getApplicationName()+" [options] modelfile ...");

    osgViewer::Viewer viewer(arguments);
    viewer.setUpViewInWindow(20, 30, 1024, 768);

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

    // load the data
    osg::ref_ptr<osg::Node> loadedModel = osgDB::readNodeFiles(arguments);
    if (!loadedModel) {
        std::cout << arguments.getApplicationName() <<": No model loaded" << std::endl;
    }

    // any option left unread are converted into errors to write out later.
    arguments.reportRemainingOptionsAsUnrecognized();

    // report any errors if they have occurred when parsing the program arguments.
    if (arguments.errors()) {
        arguments.writeErrorMessages(std::cout);
        return 1;
    }

    //Create fog
    osg::ref_ptr<osg::Fog> fog = new osg::Fog;
    fog->setMode(osg::Fog::EXP);
    fog->setDensity(1E-9);
    fog->setColor(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));

    osg::ref_ptr< osg::Group > scene = new osg::Group;
    // Attach external model and fog it
    if (loadedModel.valid()) {
        loadedModel->getOrCreateStateSet()->setAttributeAndModes(fog.get());
        scene->addChild(loadedModel);
    }

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
    osg::Node* skyBox = createSkyBox(environmentMap);
    skyBox->getOrCreateStateSet()->setAttributeAndModes(fog.get());
    scene->addChild( skyBox );

    // Create Triton Waters
    osg::Geode * geode = new osg::Geode;
    scene->addChild( geode );
    geode->addDrawable( new TritonDrawable( environmentMap, fog ) );

    // Set the same Light as Triton OpenGL example ...
    osg::Vec4 position(0, 1.0 / sqrt(2.0), 1.0 / sqrt(2.0), 0);
    osg::Vec4 ambient( osg::Vec3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0) * 1.5, 1.0 );
    osg::Vec4 diffuse( osg::Vec3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0) * 1.5, 1.0 );

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

    viewer.realize();

    return viewer.run();

}
