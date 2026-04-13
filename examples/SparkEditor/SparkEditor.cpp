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

#include <osg/Geode>
#include <osg/TexGen>
#include <osg/Texture2D>
#include <osg/Drawable>

#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Quat>
#include <osgUtil/PrintVisitor>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>
#include <osg/BlendFunc>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "OsgImGuiHandler.hpp"

#include <SPARK.h>
#include <SPARK_GL.h>


template<typename T>
std::string Vec3ToString(const T& vec) {
    std::stringstream ss;
    ss << "{" << vec.x() << "," << vec.y() << "," << vec.z() << "}";
    return ss.str();
}

struct VortexPara {
    float amp = 0.1; 
    float len = 0.04;
    float speed = 0.08;
}; 

struct CameraInfo {
    osg::Vec3 pos;
    osg::Vec3 rot;   // Euler (rad)
    osg::Vec3 scale;
};

CameraInfo ExtractCameraInfo(osg::Camera* cam)
{
    CameraInfo info;

    osg::Matrix viewMat = cam->getViewMatrix();
    osg::Matrix invMat = osg::Matrix::inverse(viewMat); // ⭐关键

    osg::Quat rotation;
    osg::Vec3 translation;
    osg::Vec3 scale;
    osg::Quat so;

    invMat.decompose(translation, rotation, scale, so);

    info.pos = translation;
    info.scale = scale;
    info.rot = rotation.asVec3(); // 欧拉角（注意不是很好用）

    return info;
}

static VortexPara vortexPara;

static SPK::Ref<SPK::ColorSimpleInterpolator> colorInterpolator;

static ImVec4 beginColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f) ;
static ImVec4 EndColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f) ;

class RotateCallback : public osg::NodeCallback {
public:
    RotateCallback() : osg::NodeCallback(), enabled_(true) {}
    void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::MatrixTransform* xform = dynamic_cast<osg::MatrixTransform*>(node);
        if (xform && enabled_) {
            double t = nv->getFrameStamp()->getSimulationTime();
            xform->setMatrix(osg::Matrix::rotate(t, osg::Vec3(0, 1, 0)));
            auto mat = xform->getMatrix();
            // std::cout<<"Pos:"<<Vec3ToString(mat.getTrans())<<std::endl;
            // auto eular =  mat.getRotate()*osg::Vec3(0,0,-1);
            std::cout<<"node:"<< node->getName()<<std::endl;
            auto eular =  mat.getRotate().asVec3();
            std::cout<<"Rot:"<<Vec3ToString(eular)<<std::endl;
        }
        traverse(node, nv);
    }

    bool enabled_;
};

class OceanCallback : public osg::NodeCallback {
  public:
    OceanCallback() : osg::NodeCallback(), enabled_(true) {}
    void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        if (node && enabled_) {
            double t = nv->getFrameStamp()->getSimulationTime();
            auto ss = node->getStateSet();
            // std::cout<<"Update OceanCallback, time:"<<t<<std::endl;
            if(ss)
            {
                // std::cout<<"Update OceanCallback, for "<<node->getName()<<std::endl;
                ss->getOrCreateUniform("iTime", osg::Uniform::FLOAT)->set(float(t));
                ss->getOrCreateUniform("vortexPara.amp", osg::Uniform::FLOAT)->set(vortexPara.amp);
                ss->getOrCreateUniform("vortexPara.len", osg::Uniform::FLOAT)->set(vortexPara.len);
                ss->getOrCreateUniform("vortexPara.speed", osg::Uniform::FLOAT)->set(vortexPara.speed);
            }

            // ImGui::Begin("Material");
            // ImGui::ShowDemoWindow();
            // ImGui::End();
        }
    }

    bool enabled_;      
    
};

class ParticleCallback : public osg::NodeCallback {
public:
    ParticleCallback() : osg::NodeCallback(), enabled_(true) {}
    void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        // 确保值在 [0,1] 范围内，乘以 255 并四舍五入
        auto toUint8 = [](float v) -> uint8_t {
            v = std::clamp(v, 0.0f, 1.0f);
            return static_cast<uint8_t>(v * 255.0f + 0.5f);
        };

        if (node && enabled_ && colorInterpolator) {
            SPK::Color c1(
                toUint8(beginColor.x),
                toUint8(beginColor.y),
                toUint8(beginColor.z),
                toUint8(beginColor.w)
            );
            SPK::Color c2(
                toUint8(EndColor.x),
                toUint8(EndColor.y),
                toUint8(EndColor.z),
                toUint8(EndColor.w)
            );
            colorInterpolator->setValues(c1,c2);
        };
        traverse(node, nv);
    }

    bool enabled_;
};

osg::ref_ptr<osg::MatrixTransform> createPlane(){
    osg::ref_ptr<osg::MatrixTransform> rootMat = new osg::MatrixTransform();
    
    // 创建mesh
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    int size = 100;
    
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
    for(int j=0;j<size;j++){     
        for(int i=0;i<size;i++){
          vertices->push_back(osg::Vec3(float(i)/size, float(j)/size, 0));
          // 3. 定义纹理坐标（可选，用于纹理映射）
          texcoords->push_back(osg::Vec2(float(i)/size, float(j)/size)); 
      }
    }
    geometry->setVertexArray(vertices.get());
    geometry->setTexCoordArray(0, texcoords);
    for(int j=0;j<size-1;j++){     
      for(int i=0;i<size-1;i++){
        // 4. 定义图元（两个三角形，使用索引）
        // 计算四个顶点的索引（按行优先）
        int idx00 = j * size + i;          // 左下
        int idx10 = j * size + i + 1;      // 右下
        int idx01 = (j + 1) * size + i;    // 左上
        int idx11 = (j + 1) * size + i + 1;// 右上

        triangles->push_back(idx00); triangles->push_back(idx10); triangles->push_back(idx01); // 第一个三角形
        triangles->push_back(idx10); triangles->push_back(idx11); triangles->push_back(idx01); // 第二个三角形（注意顺序：逆时针）
      }
    }
    geometry->addPrimitiveSet(triangles);

    geode->addDrawable(geometry.get());

    geode->setName("plane");
    geode->addUpdateCallback(new OceanCallback());

    rootMat->setMatrix(osg::Matrix::scale(10,10,1));

    rootMat->addChild(geode.get());

    return rootMat;
}

// 创建带箭头的向量
osg::ref_ptr<osg::MatrixTransform> createVectorArrow(const osg::Vec3& start, const osg::Vec3& direction,
    float length, const osg::Vec4& color)
{
    osg::ref_ptr<osg::MatrixTransform> rootMat = new osg::MatrixTransform();

    // 计算终点
    osg::Vec3 end = start + direction * length;
    osg::Vec3 mid = start + direction * (length * 0.8f); // 箭头开始位置

    // 1. 创建线段部分
    osg::ref_ptr<osg::Geode> lineGeode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> lineGeometry = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> lineVertices = new osg::Vec3Array();
    lineVertices->push_back(start);
    lineVertices->push_back(mid);
    lineGeometry->setVertexArray(lineVertices.get());

    osg::ref_ptr<osg::Vec4Array> lineColors = new osg::Vec4Array();
    lineColors->push_back(color);
    lineGeometry->setColorArray(lineColors.get(), osg::Array::BIND_OVERALL);
    lineGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, 2));

    // 设置线宽
    osg::StateSet* lineSS = lineGeometry->getOrCreateStateSet();
    lineSS->setAttribute(new osg::LineWidth(2.0f));

    lineGeode->addDrawable(lineGeometry.get());
    rootMat->addChild(lineGeode.get());

    // 原点
    osg::ref_ptr<osg::Sphere> sphere = new osg::Sphere(osg::Vec3(0, 0, 0), length * 0.05f);
    osg::ref_ptr<osg::ShapeDrawable> sphereDrawable = new osg::ShapeDrawable(sphere);
    sphereDrawable->setColor(osg::Vec4(1,0,0,1));

    osg::ref_ptr<osg::MatrixTransform> sphereTransform = new osg::MatrixTransform();
    sphereTransform->setMatrix(osg::Matrix::translate(start));
    sphereTransform->addChild(sphereDrawable.get());
    rootMat->addChild(sphereTransform.get());


    // 2. 创建箭头部分（圆锥）
    // 创建圆锥
    float arrowRadius = length * 0.05f;
    float arrowHeight = length * 0.2f;
    osg::ref_ptr<osg::Cone> cone = new osg::Cone(osg::Vec3(0, 0, 0), arrowRadius, arrowHeight);
    osg::ref_ptr<osg::ShapeDrawable> coneDrawable = new osg::ShapeDrawable(cone);
    coneDrawable->setColor(color);

    // 计算箭头旋转矩阵
    osg::ref_ptr<osg::MatrixTransform> arrowTransform = new osg::MatrixTransform();

    // 需要将圆锥旋转到方向向量的方向
    osg::Vec3 zAxis(0, 0, 1); // 圆锥默认沿Z轴
    osg::Vec3 dir = direction;
    dir.normalize();

    // 计算旋转轴和角度
    osg::Vec3 axis = zAxis ^ dir; // 叉积得到旋转轴
    float angle = acos(zAxis * dir); // 点积得到角度

    osg::Matrix matrix;
    matrix.makeTranslate(mid); // 平移到箭头位置
    matrix.preMultRotate(osg::Quat(angle, axis)); // 旋转到方向

    arrowTransform->setMatrix(matrix);
    arrowTransform->addChild(coneDrawable.get());

    rootMat->addChild(arrowTransform.get());

    return rootMat;
}

// 使用示例：绘制法线向量
osg::ref_ptr<osg::MatrixTransform> createNormalVector(const osg::Vec3& position,
    const osg::Vec3& normal,
    float scale = 1.0f)
{
    // 假设normal是单位向量，scale控制长度
    return createVectorArrow(position, normal, scale, osg::Vec4(0, 1, 0, 1)); // 绿色法线
}

osg::ref_ptr<osg::MatrixTransform> createCube(osg::Vec3 Pos,osg::Vec3 color) {
    osg::ref_ptr<osg::Box> box = new osg::Box(osg::Vec3(0,0,0), 0.2f);
    osg::ref_ptr<osg::ShapeDrawable> boxDrawable = new osg::ShapeDrawable(box);
    boxDrawable->setColor(osg::Vec4(color, 1));

    osg::ref_ptr<osg::MatrixTransform> cubeTransform = new osg::MatrixTransform();
    cubeTransform->setMatrix(osg::Matrix::translate(Pos));
    cubeTransform->addChild(boxDrawable.get());
    return cubeTransform;
}

osg::Group* createCoord()
{
    osg::Group* root = new osg::Group();

    osg::Vec3 ori = osg::Vec3(0, 0, 0);
    osg::Vec3 dir = osg::Vec3(1, 1, 1);
    dir.normalize();
    osg::ref_ptr<osg::MatrixTransform> line = createNormalVector(
        ori,    // 起点
        dir     // 方向
    );

    //line->addUpdateCallback(new RotateCallback());
    line->setMatrix(osg::Matrix::translate(osg::Vec3(0,0,0)));

    auto lineRotate = line->getMatrix().getRotate().asVec3();
    std::cout<<"Line Rot:"<<Vec3ToString(lineRotate)<<std::endl;

    osg::ref_ptr<osg::MatrixTransform> coordOrigin = createCube(osg::Vec3(0,0,0), osg::Vec3(0, 0, 0));
    osg::ref_ptr<osg::MatrixTransform> coordX= createCube(osg::Vec3(1,0,0), osg::Vec3(1, 0, 0));
    osg::ref_ptr<osg::MatrixTransform> coordY= createCube(osg::Vec3(0,1,0), osg::Vec3(0, 1, 0));
    osg::ref_ptr<osg::MatrixTransform> coordZ= createCube(osg::Vec3(0,0,1), osg::Vec3(0, 0, 1));
    root->addChild(coordOrigin.get());
    root->addChild(coordX.get());
    root->addChild(coordY.get());
    root->addChild(coordZ.get());

    root->addChild(line.get());
    root->setName("coord");

    return root;
}

class ImGuiInitOperation : public osg::Operation
{
public:
    ImGuiInitOperation()
        : osg::Operation("ImGuiInitOperation", false)
    {
    }

    void operator()(osg::Object* object) override
    {
        osg::GraphicsContext* context = dynamic_cast<osg::GraphicsContext*>(object);
        if (!context)
            return;

        if (!ImGui_ImplOpenGL3_Init())
        {
            std::cout << "ImGui_ImplOpenGL3_Init() failed\n";
        }
    }
};

class ImGuiDemo : public OsgImGuiHandler
{
public:
    ImGuiDemo(osgViewer::Viewer* viewer)
        : _viewer(viewer) {}

protected:
    void drawUi() override
    {
        ImGui::Begin("Rotor Wash");

        // ===== 相机信息 =====
        if (_viewer)
        {
            osg::Camera* cam = _viewer->getCamera();
            CameraInfo info = ExtractCameraInfo(cam);

            ImGui::Separator();
            ImGui::Text("Camera Info");

            ImGui::Text("Pos: %.3f %.3f %.3f",
                info.pos.x(), info.pos.y(), info.pos.z());

            ImGui::Text("Rot(rad): %.3f %.3f %.3f",
                info.rot.x(), info.rot.y(), info.rot.z());

            ImGui::Text("Scale: %.3f %.3f %.3f",
                info.scale.x(), info.scale.y(), info.scale.z());
        }

        // 粒子参数测试
        if (ImGui::CollapsingHeader("Particle Color Interpolator")) {
            ImGui::Indent();
            ImGui::ColorEdit4("BeginColor", (float*)&beginColor);
            ImGui::ColorEdit4("EndColor", (float*)&EndColor);
            ImGui::Unindent();
        }

        ImGui::End();
    }

private:
    osgViewer::Viewer* _viewer;
};

namespace {

GLuint createSparkAtlasTexture()
{
    // 2x2 atlas texture used by GLQuadRenderer::setAtlasDimensions(2, 2).
    const GLubyte pixels[] = {
        255, 176, 96, 64,
        176, 128, 80, 48,
        96,  80,  48, 32,
        64,  48,  32, 16
    };

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_LUMINANCE,
        4,
        4,
        0,
        GL_LUMINANCE,
        GL_UNSIGNED_BYTE,
        pixels);
    glBindTexture(GL_TEXTURE_2D, 0);
    return textureId;
}

SPK::Ref<SPK::System> createSparkSystem(GLuint textureId)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("SparkEditor_System");

    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = SPK::GL::GLQuadRenderer::create();
    renderer->setBlendMode(SPK::BLEND_MODE_ADD);
    renderer->enableRenderingOption(SPK::RENDERING_OPTION_DEPTH_WRITE, false);
    renderer->setTexture(textureId);
    renderer->setTexturingMode(SPK::TEXTURE_MODE_2D);
    renderer->setAtlasDimensions(2, 2);

    SPK::Ref<SPK::SphericEmitter> emitter = SPK::SphericEmitter::create(
        SPK::Vector3D(0.0f, 0.0f, -1.0f),
        0.0f,
        3.14159f / 4.0f,
        SPK::Point::create(),
        true,
        -1,
        100.0f,
        0.2f,
        0.5f);

    SPK::Ref<SPK::Group> phantomGroup = system->createGroup(40);
    SPK::Ref<SPK::Group> trailGroup = system->createGroup(1000);
    SPK::Ref<SPK::Plane> ground = SPK::Plane::create(SPK::Vector3D(0.0f, -1.0f, 0.0f));

    phantomGroup->setLifeTime(5.0f, 5.0f);
    phantomGroup->setRadius(0.06f);
    phantomGroup->addEmitter(SPK::SphericEmitter::create(
        SPK::Vector3D(0.0f, 1.0f, 0.0f),
        0.0f,
        3.14159f / 4.0f,
        SPK::Point::create(SPK::Vector3D(0.0f, -1.0f, 0.0f)),
        true,
        -1,
        2.0f,
        1.2f,
        2.0f));
    phantomGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -1.0f, 0.0f)));
    phantomGroup->addModifier(SPK::Obstacle::create(ground, 0.8f));
    phantomGroup->addModifier(SPK::EmitterAttacher::create(trailGroup, emitter, true));

    trailGroup->setLifeTime(0.5f, 1.0f);
    trailGroup->setRadius(0.06f);
    trailGroup->setRenderer(renderer);
    // trailGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0xFF802080, 0xFF000000));
    colorInterpolator = SPK::ColorSimpleInterpolator::create(0xFF802080, 0xFF000000);
    trailGroup->setColorInterpolator(colorInterpolator);
    trailGroup->setParamInterpolator(SPK::PARAM_TEXTURE_INDEX, SPK::FloatRandomInitializer::create(0.0f, 4.0f));
    trailGroup->setParamInterpolator(SPK::PARAM_ROTATION_SPEED, SPK::FloatRandomInitializer::create(-0.1f, 1.0f));
    trailGroup->setParamInterpolator(SPK::PARAM_ANGLE, SPK::FloatRandomInitializer::create(0.0f, 2.0f * 3.14159f));
    trailGroup->addModifier(SPK::Rotator::create());
    trailGroup->addModifier(SPK::Destroyer::create(ground));

    return system;
}

class SparkParticlesDrawable : public osg::Drawable
{
public:
    SparkParticlesDrawable()
        : _initialized(false), _textureId(0), _lastSimulationTime(-1.0)
    {
        setSupportsDisplayList(false);
        setUseVertexBufferObjects(false);
        setDataVariance(osg::Object::DYNAMIC);
        setInitialBound(osg::BoundingBox(-10, -10, -10, 10, 10, 10));
    }

    SparkParticlesDrawable(const SparkParticlesDrawable& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
        : osg::Drawable(copy, copyop),
          _initialized(copy._initialized),
          _textureId(copy._textureId),
          _system(copy._system),
          _lastSimulationTime(copy._lastSimulationTime)
    {}

    META_Object(example_SparkEditor, SparkParticlesDrawable);

    void drawImplementation(osg::RenderInfo& renderInfo) const override
    {
        if (!_initialized)
        {
            _initialized = true;

            SPK::System::setClampStep(true, 0.1f);
            SPK::System::useAdaptiveStep(0.001f, 0.01f);
            _textureId = createSparkAtlasTexture();
            
            // program create
            // {
                // _system = createSparkSystem(_textureId);
            // }

            // // save
            // {
                // bool saved = SPK::IO::Manager::get().save("spark_editor.spk.xml", _system);
                // if(saved)
                //     std::cout<<"System saved to spark_editor.spk.xml"<<std::endl;
                // else
                //     std::cout<<"Failed to save system"<<std::endl;  
            // }
                

            // // load  
            {
                SPK::Ref<SPK::GL::GLQuadRenderer> renderer = SPK::GL::GLQuadRenderer::create();
                renderer->setBlendMode(SPK::BLEND_MODE_ADD);
                renderer->enableRenderingOption(SPK::RENDERING_OPTION_DEPTH_WRITE, false);
                renderer->setTexture(_textureId);
                renderer->setTexturingMode(SPK::TEXTURE_MODE_2D);
                renderer->setAtlasDimensions(2, 2);
                
                colorInterpolator = SPK::ColorSimpleInterpolator::create(0xFF802080, 0xFF000000);
                
                _system = SPK::IO::Manager::get().load("spark_editor.spk");
                
                _system->getGroup(1)->setColorInterpolator(colorInterpolator);
                _system->getGroup(1)->setRenderer(renderer);
                if(_system)
                    std::cout<<"System loaded from spark_editor.spk"<<std::endl;
                else
                    std::cout<<"Failed to load system"<<std::endl;                                      
            }          
        }

        if (!_system)
            return;

        const osg::FrameStamp* frameStamp = renderInfo.getState()->getFrameStamp();
        if (frameStamp)
        {
            const double now = frameStamp->getSimulationTime();
            float deltaTime = 1.0f / 60.0f;
            if (_lastSimulationTime >= 0.0)
                deltaTime = static_cast<float>(now - _lastSimulationTime);
            _lastSimulationTime = now;

            if (deltaTime < 0.0001f)
                deltaTime = 1.0f / 60.0f;
            _system->updateParticles(deltaTime);
        }

        SPK::GL::GLRenderer::saveGLStates();
        _system->renderParticles();
        SPK::GL::GLRenderer::restoreGLStates();
    }

private:
    mutable bool _initialized;
    mutable GLuint _textureId;
    mutable SPK::Ref<SPK::System> _system;
    mutable double _lastSimulationTime;
};

osg::ref_ptr<osg::MatrixTransform> createSparkNode()
{
    osg::ref_ptr<osg::MatrixTransform> rootMat = new osg::MatrixTransform();

    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    geode->setCullingActive(false);
    geode->addDrawable(new SparkParticlesDrawable());
    geode->addUpdateCallback(new ParticleCallback());

    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE), osg::StateAttribute::ON);
    ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    // Move particles slightly in front of the default camera.
    rootMat->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 0.0f));
    rootMat->addChild(geode.get());
    return rootMat;
}

} // namespace


int main(int , char **)
{
    // create viewer on heap as a test, this looks to be causing problems
    // on init on some platforms, and seg fault on exit when multi-threading on linux.
    // Normal stack based version below works fine though...

    // construct the viewer.
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;

    osgDB::FilePathList& pathList = osgDB::Registry::instance()->getDataFilePathList();
    
    std::cout << "OSG Data File Paths:" << std::endl;
    for (osgDB::FilePathList::iterator it = pathList.begin(); 
         it != pathList.end(); ++it) {
        std::cout << "  " << *it << std::endl;
    }

    // add model to viewer.
    osg::Group* root = new osg::Group();
    root->addChild(createCoord());
    // root->addChild(createPlane());
    root->addChild(createSparkNode());
    
    viewer->setThreadingModel(osgViewer::Viewer::ThreadingModel::SingleThreaded);
    viewer->setRealizeOperation(new ImGuiInitOperation);
    viewer->addEventHandler(new ImGuiDemo(viewer.get()));

    viewer->realize();
    std::cout<<"getThreadingModel:"<< viewer->getThreadingModel()<<std::endl;

    viewer->setSceneData( root );

    return viewer->run();
}
