#include "SDKSample.h"
#include "OgreRenderQueue.h"
#include "OgreD3D9Texture.h"
#include "OgreGLTexture.h"
#include "Triton.h"
#include <d3d9.h>

// Copyright (c) 2012-2013 Sundog Software, LLC. All rights reserved worldwide.

/**
This is an example of integrating Triton's library for ocean waves into Ogre3D. More info on
Triton may be found at http://www.sundog-soft.com/ This sample draws the ocean using a renderer
queue listener, and integrates with a dynamically generated environment cube map for accurate
reflections in Triton's water.

If you don't yet have a Triton license, be sure to run in windowed mode as you'll need
to acknowledge a warning dialog box when Triton::Environment is constructed.

This source is based on the Ogre3D tutorial applications, and you should use the same project settings
from the tutorials with a few additions:

- You must link in Triton-MTD-DLL.lib for debug builds or
  Triton-MT-DLL.lib for release builds. Refer to the Triton documentation
  (http://www.sundog-soft.com/docs/triton/html/index.html) for the proper library names to use for
  other code generation types.

- Add the "public headers" directory of the Triton SDK to your include path.

- Add the "lib/vcX/win32" directory of the Triton SDK to your library path, where X is the version
  of visual studio you're using (6, 7, 8, 9, or 10). x64 libraries are also available.

- You'll need to add the include paths and library paths for the RenderSystems we access directly
*/

// Set these to your own once you've purchased a license:
static char* kUserName = "User Name";
static char* kLicenseCode = "License Code";

static Triton::Environment *environment = 0;
static Triton::Ocean *ocean = 0;
static Triton::ResourceLoader *resourceLoader = 0;

// We use a RenderQueueListener to piggyback on the skybox queue, so we can draw the ocean
// as one of the last things in the frame, but before overlays.
class MyRenderQueueListener : public Ogre::RenderQueueListener
{
public:
    MyRenderQueueListener(Ogre::SceneManager *pSceneMgr, Ogre::Root *pRoot, Ogre::TexturePtr pCubeMap) : RenderQueueListener(),
        sceneMgr(pSceneMgr), cubeMap(pCubeMap), root(pRoot) {}

    void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation) {
        if (queueGroupId == Ogre::RENDER_QUEUE_SKIES_LATE) {
            if (ocean) {
                // Grab the camera and projection matrices from Ogre, and
                // feed them to Triton
                SetMatrices();
                // Grab the ambient and directional light from Ogre, and
                // feed them to Triton
                SetLighting();
                // Grab the dynamic cube map and feed that to Triton for
                // use in reflections
                SetEnvMap();

                // Now, actually draw the ocean's projected grid
                double t = (double)(root->getTimer()->getMilliseconds()) * 0.001;
                ocean->Draw(t);
            }
        }
    }

protected:

    void SetMatrices(void) {
        // Extracts the modelview and projection matrices from Ogre
        // and gives them to Triton, so it can render the ocean
        // consistently.
        Ogre::Camera *cam = sceneMgr->getCamera("MainCamera");

        if (cam && environment) {
            Ogre::Matrix4 proj = cam->getProjectionMatrixWithRSDepth();
            Ogre::Matrix4 view = cam->getViewMatrix();
            double dProj[16], dView[16];
            int idx = 0;
            for (int row = 0; row < 4; row++) {
                for (int col = 0; col < 4; col++) {
                    dProj[idx] = proj[col][row];
                    dView[idx] = view[col][row];
                    idx++;
                }
            }
            environment->SetProjectionMatrix(dProj);
            environment->SetCameraMatrix(dView);
        }
    }

    void SetLighting(void) {
        // Grab the scene's lighting properties and hand that to Triton, to
        // light the ocean consistently with the rest of the scene
        Ogre::Light *light = sceneMgr->getLight("MainLight");
        if (environment && light) {
            Ogre::ColourValue ambient;
            ambient = sceneMgr->getAmbientLight();
            Ogre::Vector3 dir = light->getDirection();
            Ogre::ColourValue diff = light->getDiffuseColour();

            environment->SetAmbientLight(Triton::Vector3(ambient.r, ambient.g, ambient.b));
            environment->SetDirectionalLight(Triton::Vector3(-dir.x, -dir.y, -dir.z),
                                             Triton::Vector3(diff.r, diff.g, diff.b));
        }
    }

    void SetEnvMap() {
        // Grab the underlying environment cube map and hand it to Triton for reflections.
        // This does force us to go under the hood with the Ogre render systems.
        // Be sure to also pass in a texture matrix to SetEnvironmentMap if your cube
        // map faces are anything other than +x, -x, +y, -y, +z, -z.
        Ogre::D3D9Texture *d3d9Tex = dynamic_cast< Ogre::D3D9Texture * >( cubeMap.get() );

        // Our cube map textures are upside down from what Triton expects, so we flip them
        Triton::Matrix3 flipZ = Triton::Matrix3::Identity;
        flipZ.elem[2][2] = -1;

        if (d3d9Tex) {
            IDirect3DCubeTexture9 *tex = d3d9Tex->getCubeTexture();

            if (tex) environment->SetEnvironmentMap((Triton::TextureHandle)tex, flipZ);
        } else {
            Ogre::GLTexture *glTex = dynamic_cast< Ogre::GLTexture * >( cubeMap.get() );
            if (glTex) {
                GLuint tex = glTex->getGLID();
                environment->SetEnvironmentMap((Triton::TextureHandle)tex, flipZ);
            }
        }
    }

    Ogre::SceneManager *sceneMgr;
    Ogre::TexturePtr cubeMap;
    Ogre::Root *root;
};

// Properly handle device lost and reset events under DX9:
class MyRenderSystemListener : public Ogre::RenderSystem::Listener
{
public:
    MyRenderSystemListener() : Ogre::RenderSystem::Listener() {}

    void eventOccurred(const Ogre::String& eventName, const Ogre::NameValuePairList *parameters) {
        if (eventName == "DeviceLost") {
            if (ocean) {
                ocean->D3D9DeviceLost();
            }
        } else if (eventName == "DeviceRestored") {
            if (ocean) {
                ocean->D3D9DeviceReset();
            }
        }
    }
};
