
/* OpenSceneGraph example, deferred.
 *
 *  Original code by Michael Kapelko, published to osg example with permission
 *  OSG Deferred Shading ( https://bitbucket.org/kornerr/osg-deferred-shading )
 *  Shader cleanup, removal of osgFX EffectCompositor and exchange of textures
 *  done by Christian Buchner
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

#include "deferred.h"
#include "osg/Texture2DMultisample"

#include <osg/AnimationPath>
#include <osg/PolygonMode>
#include <osgDB/ReadFile>
#include <osgShadow/SoftShadowMap>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgUtil/TangentSpaceGenerator>

#ifdef OSG_LIBRARY_STATIC
// in case of a static build...
USE_OSGPLUGIN(osg2)
USE_OSGPLUGIN(png)
USE_OSGPLUGIN(jpeg)
USE_OSGPLUGIN(glsl)
USE_SERIALIZER_WRAPPER_LIBRARY(osg)
USE_GRAPHICSWINDOW()
#endif


osg::Texture2D *createTextureDepth(int textureSize)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize(textureSize, textureSize);
    tex2D->setInternalFormat(GL_DEPTH_COMPONENT24);
    tex2D->setSourceFormat(GL_DEPTH_COMPONENT);
    tex2D->setSourceType(GL_UNSIGNED_INT);
    return tex2D.release();
}

osg::Texture2D *createTexture2D(int textureSize)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setTextureSize(textureSize, textureSize);
    tex2D->setInternalFormat(GL_RGBA16F_ARB);
    tex2D->setSourceFormat(GL_RGBA);
    tex2D->setSourceType(GL_FLOAT);
    return tex2D.release();
}

osg::Camera *createHUDCamera(double left,
                             double right,
                             double bottom,
                             double top)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setAllowEventFocus(false);
    camera->setProjectionMatrix(osg::Matrix::ortho2D(left, right, bottom, top));
    camera->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    return camera.release();
}

osg::ref_ptr<osg::LightSource> createLight(const osg::Vec3 &pos)
{
    osg::ref_ptr<osg::LightSource> light = new osg::LightSource;
    light->getLight()->setPosition(osg::Vec4(pos.x(), pos.y(), pos.z(), 1));
    light->getLight()->setAmbient(osg::Vec4(0.2, 0.2, 0.2, 1));
    light->getLight()->setDiffuse(osg::Vec4(0.8, 0.8, 0.8, 1));
    return light;
}

class CreateTangentSpace : public osg::NodeVisitor
{
public:
    CreateTangentSpace() : NodeVisitor(NodeVisitor::TRAVERSE_ALL_CHILDREN), tsg(new osgUtil::TangentSpaceGenerator) {}
    virtual void apply(osg::Geode& geode)
    {
        for (unsigned int i = 0; i < geode.getNumDrawables(); ++i)
        {
            osg::Geometry *geo = dynamic_cast<osg::Geometry *>(geode.getDrawable(i));
            if (geo != NULL)
            {
                // assume the texture coordinate for normal maps is stored in unit #0
                tsg->generate(geo, 0);
                // pass2.vert expects the tangent array to be stored inside gl_MultiTexCoord1
                geo->setTexCoordArray(1, tsg->getTangentArray());
            }
        }
        traverse(geode);
    }
private:
    osg::ref_ptr<osgUtil::TangentSpaceGenerator> tsg;
};

Pipeline createPipelinePlainOSG(
        osg::ref_ptr<osg::Group> scene,
        osg::ref_ptr<osg::Group> transparentGroup,
        osg::ref_ptr<osgShadow::ShadowedScene> shadowedScene,
        const osg::Vec3 lightPos,
        osg::ref_ptr<osg::Camera> mainCamera)
{
    Pipeline p;
    p.graph = new osg::Group;
    p.textureSize = 1024;
    p.sceneDepth = createTextureDepth(p.textureSize);

    // Pass 1 (shadow).
    p.pass1Shadows = createTexture2D(p.textureSize);
    osg::ref_ptr<osg::Camera> pass1 =
        createRTTCamera(p.pass1Shadows,false,"pass1");
    pass1->attach(osg::Camera::COLOR_BUFFER, p.pass1Shadows);
    pass1->addChild(shadowedScene.get());

    // pass2 shades expects tangent vectors to be available as texcoord array for texture #1
    // we use osgUtil::TangentSpaceGenerator to generate these
    CreateTangentSpace cts;
    scene->accept(cts);

    // Pass 2 (positions, normals, colors).
    p.pass2Positions = createTexture2D(p.textureSize);
    p.pass2Normals   = createTexture2D(p.textureSize);
    p.pass2Colors    = createTexture2D(p.textureSize);
    p.pass2Depth     = createTexture2D(p.textureSize);
    osg::ref_ptr<osg::Camera> pass2 =
        createRTTCamera(p.pass2Positions, false,"pass2");
    pass2->attach(osg::Camera::COLOR_BUFFER0, p.pass2Positions);
    pass2->attach(osg::Camera::COLOR_BUFFER1, p.pass2Normals);
    pass2->attach(osg::Camera::COLOR_BUFFER2, p.pass2Colors);
    pass2->attach(osg::Camera::COLOR_BUFFER3, p.pass2Depth);
    pass2->attach(osg::Camera::DEPTH_BUFFER, p.sceneDepth);

    pass2->addChild(scene.get());
    double fovy, aspect, zNear, zFar;
    mainCamera->getProjectionMatrixAsPerspective(
        fovy, aspect, zNear, zFar
    );
    osg::ref_ptr<osg::StateSet> ss = setShaderProgram(pass2, "shaders/pass2.vert", "shaders/pass2.frag");
    ss->setTextureAttributeAndModes(0, createTexture("Images/whitemetal_diffuse.jpg"));
    ss->setTextureAttributeAndModes(1, createTexture("Images/whitemetal_normal.jpg"));
    ss->addUniform(new osg::Uniform("diffMap", 0));
    ss->addUniform(new osg::Uniform("bumpMap", 1));
    ss->addUniform(new osg::Uniform("useBumpMap", 1));
    ss->addUniform(new osg::Uniform("uNear",(float) zNear));
    ss->addUniform(new osg::Uniform("uFar", (float)zFar));
        
    // Pass 3 (final).
    p.pass3Final = createTexture2D(p.textureSize);
    osg::ref_ptr<osg::Camera> pass3 =
        createRTTCamera( p.pass3Final, true,"pass3");
    pass3->attach(osg::Camera::COLOR_BUFFER, p.pass3Final);
    pass3->setRenderOrder(osg::Camera::PRE_RENDER);
    ss = setShaderProgram(pass3, "shaders/pass3.vert", "shaders/pass3.frag");
    ss->setTextureAttributeAndModes(0, p.pass2Positions);
    ss->setTextureAttributeAndModes(1, p.pass2Normals);
    ss->setTextureAttributeAndModes(2, p.pass2Colors);
    ss->setTextureAttributeAndModes(3, p.pass1Shadows);
    ss->addUniform(new osg::Uniform("posMap",    0));
    ss->addUniform(new osg::Uniform("normalMap", 1));
    ss->addUniform(new osg::Uniform("colorMap",  2));
    ss->addUniform(new osg::Uniform("shadowMap", 3));
    // Light position.
    ss->addUniform(new osg::Uniform("lightPos", lightPos));

    // Pass 4 (transparent objects).
    osg::ref_ptr<osg::Camera> pass4 =
    createRTTCamera( p.pass3Final, false,"pass4");
    pass4->attach(osg::Camera::COLOR_BUFFER, p.pass3Final);
    pass4->attach(osg::Camera::DEPTH_BUFFER, p.sceneDepth);
    pass4->addChild(transparentGroup.get());      
    //覆盖renderoder为post_render，确保在final pass之后绘制;
    pass4->setRenderOrder(osg::Camera::POST_RENDER);
    pass4->setClearMask( GL_STENCIL_BUFFER_BIT);
    
    // 辅助pass,输出透明对象到单独纹理 
    p.pass5Transparent = createTexture2D(p.textureSize);
    osg::ref_ptr<osg::Camera> pass5 =
    createRTTCamera(p.pass5Transparent,false,"pass5");
    pass5->attach(osg::Camera::COLOR_BUFFER, p.pass5Transparent);
    pass5->attach(osg::Camera::DEPTH_BUFFER, p.sceneDepth);
    pass5->setClearMask( GL_STENCIL_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
    
    pass5->addChild(transparentGroup.get());     
    pass5->setRenderOrder(osg::Camera::PRE_RENDER);

    // Graph.
    p.graph->addChild(pass1);
    p.graph->addChild(pass2);
    p.graph->addChild(pass3);
    p.graph->addChild(pass4);
    p.graph->addChild(pass5);

    return p;
}

osg::Camera *createRTTCamera(osg::Texture *tex,
                             bool isAbsolute,std::string name)
{
    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setClearColor(osg::Vec4(0,0,0,1));
    camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
    camera->setRenderOrder(osg::Camera::PRE_RENDER);
    camera->setName(name);
    if (tex && dynamic_cast<osg::Texture2DMultisample*>(tex) == nullptr)
    {
        tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
        tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
        camera->setViewport(0, 0, tex->getTextureWidth(), tex->getTextureHeight());
    }
    if (isAbsolute)
    {
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setViewMatrix(osg::Matrix::identity());
        camera->addChild(createScreenQuad(1.0f, 1.0f));
    }
    return camera.release();
}

osg::ref_ptr<osg::Group> createSceneRoom()
{
    // Room.
    osg::ref_ptr<osg::MatrixTransform> room = new osg::MatrixTransform;
    osg::ref_ptr<osg::Node> roomModel = osgDB::readRefNodeFile("simpleroom.osgt");
    room->addChild(roomModel);
    room->setMatrix(osg::Matrix::translate(0, 0, 1));
    // Torus.
    osg::ref_ptr<osg::MatrixTransform> torus = new osg::MatrixTransform;
    osg::ref_ptr<osg::Node> torusModel = osgDB::readRefNodeFile("torus.osgt");
    torus->addChild(torusModel);
    setAnimationPath(torus, osg::Vec3(0, 0, 15), 6, 16);
    // Torus2.
    osg::ref_ptr<osg::MatrixTransform> torus2 = new osg::MatrixTransform;
    torus2->addChild(torusModel);
    setAnimationPath(torus2, osg::Vec3(-20, 0, 100), 20, 0);
    // Torus3.
    osg::ref_ptr<osg::MatrixTransform> torus3 = new osg::MatrixTransform;
    torus3->addChild(torusModel);
    setAnimationPath(torus3, osg::Vec3(0, 0, 40), 3, 25);
    // Scene.
    osg::ref_ptr<osg::Group> scene = new osg::Group;
    scene->addChild(room);
    scene->addChild(torus);
    scene->addChild(torus2);
    scene->addChild(torus3);
    return scene;
}

osg::Geode *createScreenQuad(float width,
                             float height,
                             float scale,
                             osg::Vec3 corner)
{
    osg::Geometry* geom = osg::createTexturedQuadGeometry(
        corner,
        osg::Vec3(width, 0, 0),
        osg::Vec3(0, height, 0),
        0,
        0,
        scale,
        scale);
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    quad->addDrawable(geom);
    int values = osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED;
    quad->getOrCreateStateSet()->setAttribute(
        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK,
                             osg::PolygonMode::FILL),
        values);
    quad->getOrCreateStateSet()->setMode(GL_LIGHTING, values);
    return quad.release();
}

osg::Texture2D *createTexture(const std::string &fileName)
{
    osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    texture->setImage(osgDB::readRefImageFile(fileName));
    texture->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::REPEAT);
    texture->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::REPEAT);
    texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    texture->setMaxAnisotropy(16.0f);
    return texture.release();
}

osg::ref_ptr<osg::Camera> createTextureDisplayQuad(
    const osg::Vec3 &pos,
    osg::StateAttribute *tex,
    float scale,
    float width,
    float height)
{
    osg::ref_ptr<osg::Camera> hc = createHUDCamera();
    hc->addChild(createScreenQuad(width, height, scale, pos));
    hc->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex);
    return hc;
}

void setAnimationPath(osg::ref_ptr<osg::MatrixTransform> node,
                      const osg::Vec3 &center,
                      float time,
                      float radius)
{
    // Create animation.
    osg::ref_ptr<osg::AnimationPath> path = new osg::AnimationPath;
    path->setLoopMode(osg::AnimationPath::LOOP);
    unsigned int numSamples = 32;
    float delta_yaw = 2.0f * osg::PI / (static_cast<float>(numSamples) - 1.0f);
    float delta_time = time / static_cast<float>(numSamples);
    for (unsigned int i = 0; i < numSamples; ++i)
    {
        float yaw = delta_yaw * static_cast<float>(i);
        osg::Vec3 pos(center.x() + sinf(yaw)*radius,
                      center.y() + cosf(yaw)*radius,
                      center.z());
        osg::Quat rot(-yaw, osg::Z_AXIS);
        path->insert(delta_time * static_cast<float>(i),
                     osg::AnimationPath::ControlPoint(pos, rot));
    }
    // Assign it.
    node->setUpdateCallback(new osg::AnimationPathCallback(path.get()));
}

osg::ref_ptr<osg::StateSet> setShaderProgram(osg::ref_ptr<osg::Camera> pass,
                                             const std::string& vert,
                                             const std::string& frag)
{
    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(osgDB::readRefShaderFile(vert));
    program->addShader(osgDB::readRefShaderFile(frag));
    osg::ref_ptr<osg::StateSet> ss = pass->getOrCreateStateSet();
    ss->setAttributeAndModes(
        program.get(),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
    return ss;
}