#include "teapot.h"

void RotateCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
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

osg::Geode* createTeapot()
{
    osg::Geode* geode = new osg::Geode();

    // add the teapot to the geode.
    geode->addDrawable( new Teapot );

    // add a reflection map to the teapot.
    osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile("Images/reflect.rgb");
    if (image)
    {
        osg::Texture2D* texture = new osg::Texture2D;
        texture->setImage(image);

        osg::TexGen* texgen = new osg::TexGen;
        texgen->setMode(osg::TexGen::SPHERE_MAP);

        osg::StateSet* stateset = new osg::StateSet;
        stateset->setTextureAttributeAndModes(0,texture,osg::StateAttribute::ON);
        stateset->setTextureAttributeAndModes(0,texgen,osg::StateAttribute::ON);

        geode->setStateSet(stateset);
    }

    return geode;
}

osg::ref_ptr<osg::MatrixTransform> createTeapotAtPos(osg::Vec3 pos){
    osg::ref_ptr<osg::MatrixTransform> cubeTransform = new osg::MatrixTransform();
    cubeTransform->setMatrix(osg::Matrix::translate(pos));
    cubeTransform->addChild(createTeapot());
    return cubeTransform;
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
    float scale )
{
    // 假设normal是单位向量，scale控制长度
    return createVectorArrow(position, normal, scale, osg::Vec4(0, 1, 0, 1)); // 绿色法线
}

osg::ref_ptr<osg::MatrixTransform> createCube(osg::Vec3 Pos,osg::Vec3 color) {
    osg::ref_ptr<osg::Box> box = new osg::Box(osg::Vec3(0,0,0), 10.0f);
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