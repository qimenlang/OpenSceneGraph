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

#include <osgDB/ReadFile>

#include <osgViewer/Viewer>
#include <osg/Matrix>
#include <osg/MatrixTransform>
#include <osg/Quat>
#include <osgUtil/PrintVisitor>
#include <osg/ShapeDrawable>
#include <osg/LineWidth>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include "OsgImGuiHandler.hpp"

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

static VortexPara vortexPara;

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


osg::ref_ptr<osg::MatrixTransform> createOcean(){
    osg::ref_ptr<osg::MatrixTransform> rootMat = new osg::MatrixTransform();
    
    // 创建mesh
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    // 100万个顶点的Mesh (1000x1000)，每个顶点包含位置和纹理坐标

    int size = 500;
    
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

    // 可选：为平面添加纹理
    // osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
    // osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Images/whitemetal_diffuse.jpg"); // 请替换为实际图片路径
    // if (image)
    // {
    //     texture->setImage(image);
    //     geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
    // }

    geode->addDrawable(geometry.get());

    // 创建程序对象并附加着色器
    osg::ref_ptr<osg::Program> program = new osg::Program;
    program->addShader(osgDB::readRefShaderFile(osg::Shader::VERTEX,"shaders/RotorWash/rotor_wash.glsl"));
    program->addShader(osgDB::readRefShaderFile(osg::Shader::VERTEX,"shaders/RotorWash/rotor_wash.vert"));
    program->addShader(osgDB::readRefShaderFile(osg::Shader::FRAGMENT,"shaders/RotorWash/rotor_wash.glsl"));
    program->addShader(osgDB::readRefShaderFile(osg::Shader::FRAGMENT,"shaders/RotorWash/rotor_wash.frag"));

    osg::ref_ptr<osg::StateSet> ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(
        program.get(),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    geode->setName("OceanGeode");
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

osg::Group* createLine()
{
    // 把 geode 放进一个新的 Group，从而确保返回的对象拥有并保持对 geode 的引用。
    osg::Group* root = new osg::Group();

    osg::Vec3 ori = osg::Vec3(0, 0, 0);
    osg::Vec3 dir = osg::Vec3(1, 1, 1);
    dir.normalize();
    // 使用示例
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
    root->setName("line_root");

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
protected:
    void drawUi() override
    {

    ImGui::Begin("Rotor Wash");
    if (ImGui::CollapsingHeader("vertor para")) {
        ImGui::Indent();
        ImGui::SliderFloat("vortex amp", &vortexPara.amp, 0.0f, 1.0f);
        ImGui::SliderFloat("vortex len", &vortexPara.len, 0.0f, 1.0f);
        ImGui::SliderFloat("vortex speed", &vortexPara.speed, 0.0f, 1.0f);
        ImGui::Unindent();
    }
    ImGui::End();
    // // ImGui code goes here...
    // ImGui::ShowDemoWindow();
    }
};


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
    root->addChild(createLine());
    root->addChild(createOcean());

    viewer->setThreadingModel(osgViewer::Viewer::ThreadingModel::SingleThreaded);

    
    viewer->setRealizeOperation(new ImGuiInitOperation);
    viewer->addEventHandler(new ImGuiDemo);

    viewer->realize();
    std::cout<<"getThreadingModel:"<< viewer->getThreadingModel()<<std::endl;

    viewer->setSceneData( root );

    return viewer->run();
}
