/* OpenSceneGraph example, osgteapot.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#include "teapot.h"
#include <osg/Geode>
#include <osg/TexGen>
#include <osg/Texture2D>
#include <osg/BlendFunc>
#include <osg/Depth>

#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Quat>
#include <osgUtil/PrintVisitor>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>


#include "deferred.h"
#include <osg/AnimationPath>
#include <osg/PolygonMode>
#include <osgDB/ReadFile>
#include <osgShadow/SoftShadowMap>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/TangentSpaceGenerator>

// The classic OpenGL teapot... taken form glut-3.7/lib/glut/glut_teapot.c

/* Copyright (c) Mark J. Kilgard, 1994. */

/**
(c) Copyright 1993, Silicon Graphics, Inc.

ALL RIGHTS RESERVED

Permission to use, copy, modify, and distribute this software
for any purpose and without fee is hereby granted, provided
that the above copyright notice appear in all copies and that
both the copyright notice and this permission notice appear in
supporting documentation, and that the name of Silicon
Graphics, Inc. not be used in advertising or publicity
pertaining to distribution of the software without specific,
written prior permission.

THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU
"AS-IS" AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR
OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF
MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  IN NO
EVENT SHALL SILICON GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE
ELSE FOR ANY DIRECT, SPECIAL, INCIDENTAL, INDIRECT OR
CONSEQUENTIAL DAMAGES OF ANY KIND, OR ANY DAMAGES WHATSOEVER,
INCLUDING WITHOUT LIMITATION, LOSS OF PROFIT, LOSS OF USE,
SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD PARTIES, WHETHER OR
NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF THE POSSIBILITY
OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE OR
PERFORMANCE OF THIS SOFTWARE.

US Government Users Restricted Rights

Use, duplication, or disclosure by the Government is subject to
restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
(c)(1)(ii) of the Rights in Technical Data and Computer
Software clause at DFARS 252.227-7013 and/or in similar or
successor clauses in the FAR or the DOD or NASA FAR
Supplement.  Unpublished-- rights reserved under the copyright
laws of the United States.  Contractor/manufacturer is Silicon
Graphics, Inc., 2011 N.  Shoreline Blvd., Mountain View, CA
94039-7311.

OpenGL(TM) is a trademark of Silicon Graphics, Inc.
*/


/* Rim, body, lid, and bottom data must be reflected in x and
   y; handle and spout data across the y axis only.  */

struct PrintGLCallback : public osg::Camera::DrawCallback
{
    mutable bool done = false;

    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        if(done) return;
        done = true;
        std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
        std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
        std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;
    }
};


osg::ref_ptr<osg::Group> createTransparentGroup(){
    osg::ref_ptr<osg::Group> transparentGroup = new osg::Group();

    osg::ref_ptr<osg::MatrixTransform> cube0 = createCube(osg::Vec3(0,0,5),osg::Vec3(1.0,0.0,0.0));
    osg::ref_ptr<osg::MatrixTransform> cube1 = createCube(osg::Vec3(0,0,20),osg::Vec3(0.0,1.0,0.0));
    osg::ref_ptr<osg::MatrixTransform> cube2 = createCube(osg::Vec3(0,0,35),osg::Vec3(0.0,0.0,1.0));

    transparentGroup->addChild(cube0.get());
    transparentGroup->addChild(cube1.get());
    transparentGroup->addChild(cube2.get());

    // 设置透明属性
    osg::StateSet* ss = transparentGroup->getOrCreateStateSet();
    // 开启混合
    ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    // 典型 alpha blending
    ss->setAttributeAndModes(
        new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA),
        osg::StateAttribute::ON
    );
    // 开启深度测试
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
    // 关闭深度写入
    ss->setAttributeAndModes(
        new osg::Depth(osg::Depth::LESS, 0.0, 1.0, false),
        osg::StateAttribute::ON
    );

    ss->setRenderBinDetails(10, "RenderBin");
    return transparentGroup;
}

int main()
{    
    // Display everything.
    osgViewer::Viewer viewer;    
    // add the stats handler
    viewer.addEventHandler(new osgViewer::StatsHandler);
    
    // Make screenshots with 'c'.
    viewer.addEventHandler(
new osgViewer::ScreenCaptureHandler(
new osgViewer::ScreenCaptureHandler::WriteToFile(
"screenshot",
"png",
osgViewer::ScreenCaptureHandler::WriteToFile::OVERWRITE)));
        
    viewer.getCamera()->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
    // 明确设置
    double zNear = 0.1;
    double zFar  = 2000.0;
    
    viewer.getCamera()->setProjectionMatrixAsPerspective(
        60.0,             // fovy
        1.0,       // aspect（OSG 会在 resize 时更新）
        zNear,
        zFar
    );

    std::string glContextVersion = osg::DisplaySettings::instance()->getGLContextVersion();
    int glContextProfileMask = osg::DisplaySettings::instance()->getGLContextProfileMask();
    std::cout<<"Requested GL Context Version: "<<glContextVersion<<std::endl;
    std::cout<<"Requested GL Context Profile Mask: "<<glContextProfileMask<<std::endl;
        
    viewer.getCamera()->setPostDrawCallback(new PrintGLCallback());

    // Useful declaration.
    osg::ref_ptr<osg::StateSet> ss;
    // Scene.
    osg::Vec3 lightPos(0, 0, 80);
    osg::ref_ptr<osg::Group> scene = createSceneRoom();
    osg::ref_ptr<osg::Group> tspScene = createTransparentGroup();

    osg::ref_ptr<osg::LightSource> light = createLight(lightPos);
    scene->addChild(light.get());
    // Shadowed scene.
    osg::ref_ptr<osgShadow::SoftShadowMap> shadowMap = new osgShadow::SoftShadowMap;
    shadowMap->setJitteringScale(16);
    shadowMap->addShader(osgDB::readRefShaderFile("shaders/pass1Shadow.frag"));
    shadowMap->setLight(light.get());
    osg::ref_ptr<osgShadow::ShadowedScene> shadowedScene = new osgShadow::ShadowedScene;
    shadowedScene->setShadowTechnique(shadowMap.get());
    shadowedScene->addChild(scene.get());

    Pipeline p = createPipelinePlainOSG(scene,tspScene, shadowedScene, lightPos,viewer.getCamera());
    // Quads to display 1 pass textures.
    osg::ref_ptr<osg::Camera> qTexN =
        createTextureDisplayQuad(osg::Vec3(0, 0.7, 0),
                                 p.pass2Normals,
                                 p.textureSize);
    osg::ref_ptr<osg::Camera> qTexP =
        createTextureDisplayQuad(osg::Vec3(0, 0.35, 0),
                                 p.pass2Positions,
                                 p.textureSize);
    osg::ref_ptr<osg::Camera> qTexC =
        createTextureDisplayQuad(osg::Vec3(0, 0, 0),
                                 p.pass2Colors,
                                 p.textureSize);
    osg::ref_ptr<osg::Camera> qTexD =
        createTextureDisplayQuad(osg::Vec3(0.7, 0, 0),
                                 p.pass2Depth,
                              p.textureSize);
    
    osg::ref_ptr<osg::Camera> qTexTsp =
    createTextureDisplayQuad(osg::Vec3(0.7, 0.35, 0),
    p.pass5Transparent,
    p.textureSize);
                              
    // Qaud to display 2 pass shadow texture.
    osg::ref_ptr<osg::Camera> qTexS =
        createTextureDisplayQuad(osg::Vec3(0.7, 0.7, 0),
                                p.pass1Shadows,
                                p.textureSize);
    // Quad to display 3 pass final (screen) texture.
    osg::ref_ptr<osg::Camera> qTexFinal =
        createTextureDisplayQuad(osg::Vec3(0, 0, 0),
                                p.pass3Final,
                                p.textureSize,
                                1,
                                1);
                                 // Must be processed before the first pass takes
    // the result into pass1Shadows texture.
    p.graph->insertChild(0, shadowedScene.get());
    // Quads are displayed in order, so the biggest one (final) must be first,
    // otherwise other quads won't be visible.
    p.graph->addChild(qTexFinal.get());
    p.graph->addChild(qTexN.get());
    p.graph->addChild(qTexP.get());
    p.graph->addChild(qTexC.get());
    p.graph->addChild(qTexD.get());
    p.graph->addChild(qTexS.get());
    p.graph->addChild(qTexTsp.get());

    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setSceneData(p.graph.get());
    return viewer.run();
}

