/*
-----------------------------------------------------------------------------
Filename:    TutorialApplication.cpp
-----------------------------------------------------------------------------
This illustrates integrating the Triton Ocean SDK with the Ogre engine. It's based
on the Ogre Wiki tutorial framework classes.

If you have any problems, feel free to contact support@sundog-soft.com for a hand.
-----------------------------------------------------------------------------
*/

#include "TritonOgre.h"
#include "TritonApplication.h"

//-------------------------------------------------------------------------------------
TritonApplication::TritonApplication(void)
{
}
//-------------------------------------------------------------------------------------
TritonApplication::~TritonApplication(void)
{
    if (mRoot && mRenderSystemListener) {
        mRoot->getRenderSystem()->removeListener(mRenderSystemListener);
    }

    // Clean up the Triton resources.
    if (ocean) {
        delete ocean;
        ocean = 0;
    }

    if (environment) {
        delete environment;
        environment = 0;
    }

    if (resourceLoader) {
        delete resourceLoader;
        resourceLoader = 0;
    }
}

// Gets called when each face of the dynamic cubemap is rendered
void TritonApplication::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    if (!mSceneMgr || !mCubeCamera || !mCamera) return;

    mCubeCamera->setPosition(mCamera->getPosition());

    // No point drawing the water into the cubemap, as it's what receives the cubemap
    mSceneMgr->removeRenderQueueListener(myListener);

    // point the camera in the right direction based on which face of the cubemap this is
    mCubeCamera->setOrientation(Ogre::Quaternion::IDENTITY);

    if (evt.source == mTargets[0]) mCubeCamera->yaw(Ogre::Degree(-90));
    else if (evt.source == mTargets[1]) mCubeCamera->yaw(Ogre::Degree(90));
    else if (evt.source == mTargets[2]) mCubeCamera->pitch(Ogre::Degree(90));
    else if (evt.source == mTargets[3]) mCubeCamera->pitch(Ogre::Degree(-90));
    else if (evt.source == mTargets[4]) mCubeCamera->yaw(Ogre::Degree(0));
    else if (evt.source == mTargets[5]) mCubeCamera->yaw(Ogre::Degree(180));
}

void TritonApplication::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
    // Restore our listener for drawing the water.
    if (mSceneMgr) {
        mSceneMgr->addRenderQueueListener(myListener);
    }

    // Only update the cube map once to preserve performance. If you have dynamic time
    // of day effects, you'll want to re-create the cube map when conditions change.
    //evt.source->removeAllListeners();
}

void TritonApplication::SetupTriton(void)
{
    // Obtain the D3D device, if we're using D3D.
    void *pD3DDevice = 0;
    mWindow->getCustomAttribute("D3DDEVICE",&pD3DDevice);

    // WARNING: If you're in full screen mode, any license warning dialog boxes will be obscured.
    // Work in windowed mode until you have a real Triton license.

    // Substitute the path to your Triton resources directory below
    Ogre::String resPath = getenv("TRITON_PATH");
    resPath += "\\resources\\";
    resourceLoader = new Triton::ResourceLoader(resPath.c_str());

    // Create and initialize the environment
    if (!environment) {
        environment = new Triton::Environment();
        environment->SetLicenseCode(kUserName, kLicenseCode);
    }

    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP,
                                   pD3DDevice == NULL ? Triton::OPENGL_2_0 : Triton::DIRECTX_9,
                                   resourceLoader, pD3DDevice);



    if (err != Triton::SUCCEEDED) {
        OGRE_EXCEPT(Ogre::Exception::ERR_INVALID_STATE, "Error initializing Triton - do you have the "
                    "Triton SDK installed?", "Triton::Environment::Initialize");
    }

    // Add some wind so we get some waves
    Triton::WindFetch wf;
    wf.SetWind(10.0, 30.0);
    environment->AddWindFetch(wf);

    // Create the ocean itself
    ocean = Triton::Ocean::Create(environment, Triton::TESSENDORF);

    Ogre::LogManager::getSingletonPtr()->logMessage(ocean->GetFFTName());
}

void TritonApplication::createViewports(void)
{
    // setup default viewport layout and camera
    mCamera = mSceneMgr->createCamera("MainCamera");
    Ogre::Viewport *vp = mWindow->addViewport(mCamera);

    mCamera->setAspectRatio((Ogre::Real)vp->getActualWidth() / (Ogre::Real)vp->getActualHeight());
    mCamera->setNearClipDistance(5);

    mCameraMan = new OgreBites::SdkCameraMan(mCamera);   // create a default camera controller
}

//-------------------------------------------------------------------------------------
void TritonApplication::createScene(void)
{
    // Initialize Triton
    SetupTriton();

    // Create a dynamically updated cube map. This updates itself every frame, but if your environment
    // doesn't change much you could likely get away with updating less frequently.
    createCubemap();

    // Throw in a stock sky box
    mSceneMgr->setSkyBox(true, "SkyBox");

    // setup some basic lighting for our scene
    mSceneMgr->setAmbientLight(Ogre::ColourValue(0.3f, 0.3f, 0.3f));

    // Inject hooks for Triton
    myListener = new MyRenderQueueListener(mSceneMgr, mRoot, mCubeMap);
    mSceneMgr->addRenderQueueListener(myListener);

    mRenderSystemListener = new MyRenderSystemListener();
    mRoot->getRenderSystem()->addListener(mRenderSystemListener);

    // Create a light
    Ogre::Light* l = mSceneMgr->createLight("MainLight");
    l->setType(Ogre::Light::LT_DIRECTIONAL);
    l->setPosition(0, 30, 0);
    l->setDirection(0, -1, 0);

    // set the camera's initial position and orientation
    mCamera->setPosition(0, 40, 150);
    mCamera->yaw(Ogre::Degree(5));

    // Create an Entity
    Ogre::Entity* ogreHead = mSceneMgr->createEntity("Head", "ogrehead.mesh");

    // Create a SceneNode and attach the Entity to it
    Ogre::SceneNode* headNode = mSceneMgr->getRootSceneNode()->createChildSceneNode("HeadNode");
    headNode->attachObject(ogreHead);
    headNode->setPosition(0, 0, -100);
}

// This is lifted from the stock CubeMapping sample
void TritonApplication::createCubemap()
{
    // create the camera used to render to our cubemap
    mCubeCamera = mSceneMgr->createCamera("CubeMapCamera");
    mCubeCamera->setFOVy(Ogre::Degree(90));
    mCubeCamera->setAspectRatio(1);
    mCubeCamera->setFixedYawAxis(false);
    mCubeCamera->setNearClipDistance(5);

    // create our dynamic cube map texture
    mCubeMap = Ogre::TextureManager::getSingleton().createManual("dyncubemap",
               Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_CUBE_MAP, 512, 512, 0,
               Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    // assign our camera to all 6 render targets of the texture (1 for each direction)
    for (unsigned int i = 0; i < 6; i++) {
        mTargets[i] = mCubeMap->getBuffer(i)->getRenderTarget();
        mTargets[i]->addViewport(mCubeCamera)->setOverlaysEnabled(false);
        mTargets[i]->addListener(this);
    }
}

void TritonApplication::destroyScene()
{
    Ogre::TextureManager::getSingleton().remove("dyncubemap");
}

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
    INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
    int main(int argc, char *argv[])
#endif
    {
        // Create application object
        TritonApplication app;

        // This is only here so the license warning dialog will pop up before you go full screen
        if (!environment) {
            environment = new Triton::Environment();
            environment->SetLicenseCode(kUserName, kLicenseCode);
        }

        try {
            app.go();
        } catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
            MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
            std::cerr << "An exception has occured: " <<
                      e.getFullDescription().c_str() << std::endl;
#endif
        }

        return 0;
    }

#ifdef __cplusplus
}
#endif
