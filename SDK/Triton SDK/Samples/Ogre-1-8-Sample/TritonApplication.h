/*
-----------------------------------------------------------------------------
Filename:    TutorialApplication.h
-----------------------------------------------------------------------------

This source file is part of the
   ___                 __    __ _ _    _
  /___\__ _ _ __ ___  / / /\ \ (_) | _(_)
 //  // _` | '__/ _ \ \ \/  \/ / | |/ / |
/ \_// (_| | | |  __/  \  /\  /| |   <| |
\___/ \__, |_|  \___|   \/  \/ |_|_|\_\_|
      |___/
      Tutorial Framework
      http://www.ogre3d.org/tikiwiki/
-----------------------------------------------------------------------------
*/
#ifndef __TritonApplication_h_
#define __TritonApplication_h_

#include "BaseApplication.h"

class TritonApplication : public BaseApplication, public Ogre::RenderTargetListener
{
public:
    TritonApplication(void);
    virtual ~TritonApplication(void);

    void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
    void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

protected:
    virtual void createScene(void);
    virtual void destroyScene(void);
    virtual void createViewports(void);
    void SetupTriton(void);
    void createCubemap(void);

    Ogre::TexturePtr mCubeMap;
    Ogre::Camera* mCubeCamera;
    Ogre::RenderTarget* mTargets[6];
    MyRenderQueueListener *myListener;

    bool cubeMapRendered;

    MyRenderSystemListener *mRenderSystemListener;
};

#endif // #ifndef __TritonApplication_h_
