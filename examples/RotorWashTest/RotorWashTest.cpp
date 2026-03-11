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

static int patchdata[][16] =
{
    /* rim */
  {102, 103, 104, 105, 4, 5, 6, 7, 8, 9, 10, 11,
    12, 13, 14, 15},
    /* body */
  {12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27},
  {24, 25, 26, 27, 29, 30, 31, 32, 33, 34, 35, 36,
    37, 38, 39, 40},
    /* lid */
  {96, 96, 96, 96, 97, 98, 99, 100, 101, 101, 101,
    101, 0, 1, 2, 3,},
  {0, 1, 2, 3, 106, 107, 108, 109, 110, 111, 112,
    113, 114, 115, 116, 117},
    /* bottom */
  {118, 118, 118, 118, 124, 122, 119, 121, 123, 126,
    125, 120, 40, 39, 38, 37},
    /* handle */
  {41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52,
    53, 54, 55, 56},
  {53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64,
    28, 65, 66, 67},
    /* spout */
  {68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83},
  {80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
    92, 93, 94, 95}
};
/* *INDENT-OFF* */

static float cpdata[][3] =
{
    {0.2, 0, 2.7}, {0.2, -0.112, 2.7}, {0.112, -0.2, 2.7}, {0,
    -0.2, 2.7}, {1.3375, 0, 2.53125}, {1.3375, -0.749, 2.53125},
    {0.749, -1.3375, 2.53125}, {0, -1.3375, 2.53125}, {1.4375,
    0, 2.53125}, {1.4375, -0.805, 2.53125}, {0.805, -1.4375,
    2.53125}, {0, -1.4375, 2.53125}, {1.5, 0, 2.4}, {1.5, -0.84,
    2.4}, {0.84, -1.5, 2.4}, {0, -1.5, 2.4}, {1.75, 0, 1.875},
    {1.75, -0.98, 1.875}, {0.98, -1.75, 1.875}, {0, -1.75,
    1.875}, {2, 0, 1.35}, {2, -1.12, 1.35}, {1.12, -2, 1.35},
    {0, -2, 1.35}, {2, 0, 0.9}, {2, -1.12, 0.9}, {1.12, -2,
    0.9}, {0, -2, 0.9}, {-2, 0, 0.9}, {2, 0, 0.45}, {2, -1.12,
    0.45}, {1.12, -2, 0.45}, {0, -2, 0.45}, {1.5, 0, 0.225},
    {1.5, -0.84, 0.225}, {0.84, -1.5, 0.225}, {0, -1.5, 0.225},
    {1.5, 0, 0.15}, {1.5, -0.84, 0.15}, {0.84, -1.5, 0.15}, {0,
    -1.5, 0.15}, {-1.6, 0, 2.025}, {-1.6, -0.3, 2.025}, {-1.5,
    -0.3, 2.25}, {-1.5, 0, 2.25}, {-2.3, 0, 2.025}, {-2.3, -0.3,
    2.025}, {-2.5, -0.3, 2.25}, {-2.5, 0, 2.25}, {-2.7, 0,
    2.025}, {-2.7, -0.3, 2.025}, {-3, -0.3, 2.25}, {-3, 0,
    2.25}, {-2.7, 0, 1.8}, {-2.7, -0.3, 1.8}, {-3, -0.3, 1.8},
    {-3, 0, 1.8}, {-2.7, 0, 1.575}, {-2.7, -0.3, 1.575}, {-3,
    -0.3, 1.35}, {-3, 0, 1.35}, {-2.5, 0, 1.125}, {-2.5, -0.3,
    1.125}, {-2.65, -0.3, 0.9375}, {-2.65, 0, 0.9375}, {-2,
    -0.3, 0.9}, {-1.9, -0.3, 0.6}, {-1.9, 0, 0.6}, {1.7, 0,
    1.425}, {1.7, -0.66, 1.425}, {1.7, -0.66, 0.6}, {1.7, 0,
    0.6}, {2.6, 0, 1.425}, {2.6, -0.66, 1.425}, {3.1, -0.66,
    0.825}, {3.1, 0, 0.825}, {2.3, 0, 2.1}, {2.3, -0.25, 2.1},
    {2.4, -0.25, 2.025}, {2.4, 0, 2.025}, {2.7, 0, 2.4}, {2.7,
    -0.25, 2.4}, {3.3, -0.25, 2.4}, {3.3, 0, 2.4}, {2.8, 0,
    2.475}, {2.8, -0.25, 2.475}, {3.525, -0.25, 2.49375},
    {3.525, 0, 2.49375}, {2.9, 0, 2.475}, {2.9, -0.15, 2.475},
    {3.45, -0.15, 2.5125}, {3.45, 0, 2.5125}, {2.8, 0, 2.4},
    {2.8, -0.15, 2.4}, {3.2, -0.15, 2.4}, {3.2, 0, 2.4}, {0, 0,
    3.15}, {0.8, 0, 3.15}, {0.8, -0.45, 3.15}, {0.45, -0.8,
    3.15}, {0, -0.8, 3.15}, {0, 0, 2.85}, {1.4, 0, 2.4}, {1.4,
    -0.784, 2.4}, {0.784, -1.4, 2.4}, {0, -1.4, 2.4}, {0.4, 0,
    2.55}, {0.4, -0.224, 2.55}, {0.224, -0.4, 2.55}, {0, -0.4,
    2.55}, {1.3, 0, 2.55}, {1.3, -0.728, 2.55}, {0.728, -1.3,
    2.55}, {0, -1.3, 2.55}, {1.3, 0, 2.4}, {1.3, -0.728, 2.4},
    {0.728, -1.3, 2.4}, {0, -1.3, 2.4}, {0, 0, 0}, {1.425,
    -0.798, 0}, {1.5, 0, 0.075}, {1.425, 0, 0}, {0.798, -1.425,
    0}, {0, -1.5, 0.075}, {0, -1.425, 0}, {1.5, -0.84, 0.075},
    {0.84, -1.5, 0.075}
};

static float tex[2][2][2] =
{
  { {0, 0},
    {1, 0}},
  { {0, 1},
    {1, 1}}
};

/* *INDENT-ON* */

static void
teapot(GLint grid, GLenum type)
{
  float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
  long i, j, k, l;

  glPushAttrib(GL_ENABLE_BIT | GL_EVAL_BIT);
  glEnable(GL_AUTO_NORMAL);
  glEnable(GL_NORMALIZE);
  glEnable(GL_MAP2_VERTEX_3);
  glEnable(GL_MAP2_TEXTURE_COORD_2);
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 4; j++) {
      for (k = 0; k < 4; k++) {
        for (l = 0; l < 3; l++) {
          p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
          q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
          if (l == 1)
            q[j][k][l] *= -1.0;
          if (i < 6) {
            r[j][k][l] =
              cpdata[patchdata[i][j * 4 + (3 - k)]][l];
            if (l == 0)
              r[j][k][l] *= -1.0;
            s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
            if (l == 0)
              s[j][k][l] *= -1.0;
            if (l == 1)
              s[j][k][l] *= -1.0;
          }
        }
      }
    }
    glMap2f(GL_MAP2_TEXTURE_COORD_2, 0, 1, 2, 2, 0, 1, 4, 2,
      &tex[0][0][0]);
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
      &p[0][0][0]);
    glMapGrid2f(grid, 0.0, 1.0, grid, 0.0, 1.0);
    glEvalMesh2(type, 0, grid, 0, grid);
    glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
      &q[0][0][0]);
    glEvalMesh2(type, 0, grid, 0, grid);
    if (i < 6) {
      glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
        &r[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
      glMap2f(GL_MAP2_VERTEX_3, 0, 1, 3, 4, 0, 1, 12, 4,
        &s[0][0][0]);
      glEvalMesh2(type, 0, grid, 0, grid);
    }
  }
  glPopAttrib();
}


// Now the OSG wrapper for the above OpenGL code, the most complicated bit is computing
// the bounding box for the above example, normally you'll find this the easy bit.
class Teapot : public osg::Drawable
{
    public:
        Teapot() {}

        /** Copy constructor using CopyOp to manage deep vs shallow copy.*/
        Teapot(const Teapot& teapot,const osg::CopyOp& copyop=osg::CopyOp::SHALLOW_COPY):
            osg::Drawable(teapot,copyop) {}

        META_Object(myTeapotApp,Teapot)


        // the draw immediate mode method is where the OSG wraps up the drawing of
        // of OpenGL primitives.
        virtual void drawImplementation(osg::RenderInfo&) const
        {
            // teapot(..) doesn't use vertex arrays at all so we don't need to toggle their state
            // if we did we'd need to something like following call
            // state.disableAllVertexArrays(), see src/osg/Geometry.cpp for the low down.

            // just call the OpenGL code.
            teapot(14,GL_FILL);
        }


        // we need to set up the bounding box of the data too, so that the scene graph knows where this
        // objects is, for both positioning the camera at start up, and most importantly for culling.
        virtual osg::BoundingBox computeBoundingBox() const
        {
            osg::BoundingBox bbox;

            // follow is some truly horrible code required to calculate the
            // bounding box of the teapot.  Have used the original code above to do
            // help compute it.
            float p[4][4][3], q[4][4][3], r[4][4][3], s[4][4][3];
            long i, j, k, l;

            for (i = 0; i < 10; i++) {
              for (j = 0; j < 4; j++) {
                for (k = 0; k < 4; k++) {

                  for (l = 0; l < 3; l++) {
                    p[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                    q[j][k][l] = cpdata[patchdata[i][j * 4 + (3 - k)]][l];
                    if (l == 1)
                      q[j][k][l] *= -1.0;

                    if (i < 6) {
                      r[j][k][l] =
                        cpdata[patchdata[i][j * 4 + (3 - k)]][l];
                      if (l == 0)
                        r[j][k][l] *= -1.0;
                      s[j][k][l] = cpdata[patchdata[i][j * 4 + k]][l];
                      if (l == 0)
                        s[j][k][l] *= -1.0;
                      if (l == 1)
                        s[j][k][l] *= -1.0;
                    }
                  }

                  bbox.expandBy(osg::Vec3(p[j][k][0],p[j][k][1],p[j][k][2]));
                  bbox.expandBy(osg::Vec3(q[j][k][0],q[j][k][1],q[j][k][2]));

                  if (i < 6)
                  {
                    bbox.expandBy(osg::Vec3(r[j][k][0],r[j][k][1],r[j][k][2]));
                    bbox.expandBy(osg::Vec3(s[j][k][0],s[j][k][1],s[j][k][2]));
                  }


                }
              }
            }

            return bbox;
        }

    protected:

        virtual ~Teapot() {}

};

template<typename T>
std::string Vec3ToString(const T& vec) {
    std::stringstream ss;
    ss << "{" << vec.x() << "," << vec.y() << "," << vec.z() << "}";
    return ss.str();
}

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


osg::Group* createTeapot()
{
    osg::Geode* geode = new osg::Geode();

    // add the teapot to the geode.
    geode->addDrawable( new Teapot );
    geode->setName("geode");

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


    osg::ref_ptr<osg::MatrixTransform> transformNode = new osg::MatrixTransform();

    osg::Matrix matrix;
    matrix.makeRotate(osg::DegreesToRadians(45.0),osg::Vec3(0,0,1));
    transformNode->setMatrix(matrix);
    transformNode->addChild(geode);
    transformNode->addUpdateCallback(new RotateCallback());
    transformNode->setName("transformNode");

    osg::Group* root = new osg::Group();
    root->addChild(transformNode);
  
    root->setName("root");

    return root;
}

osg::ref_ptr<osg::MatrixTransform> createOcean(){
    osg::ref_ptr<osg::MatrixTransform> rootMat = new osg::MatrixTransform();
    
    // 创建mesh
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
    vertices->push_back(osg::Vec3(-1, -1, 0));
    vertices->push_back(osg::Vec3(1, -1, 0));
    vertices->push_back(osg::Vec3(-1, 1, 0));
    vertices->push_back(osg::Vec3(1, 1, 0));
    geometry->setVertexArray(vertices.get());

    // 3. 定义纹理坐标（可选，用于纹理映射）
    osg::ref_ptr<osg::Vec2Array> texcoords = new osg::Vec2Array;
    texcoords->push_back(osg::Vec2(0.0f, 0.0f)); // 对应顶点0
    texcoords->push_back(osg::Vec2(1.0f, 0.0f)); // 对应顶点1
    texcoords->push_back(osg::Vec2(0.0f, 1.0f)); // 对应顶点2
    texcoords->push_back(osg::Vec2(1.0f, 1.0f)); // 对应顶点3
    geometry->setTexCoordArray(0, texcoords);

    // 4. 定义图元（两个三角形，使用索引）
    osg::ref_ptr<osg::DrawElementsUInt> triangles = new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, 0);
    triangles->push_back(0); triangles->push_back(1); triangles->push_back(2); // 第一个三角形
    triangles->push_back(1); triangles->push_back(3); triangles->push_back(2); // 第二个三角形（注意顺序：逆时针）
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
    program->addShader(osgDB::readRefShaderFile(osg::Shader::VERTEX,"shaders/RotorWash/rotor_wash.vert"));
    program->addShader(osgDB::readRefShaderFile(osg::Shader::FRAGMENT,"shaders/RotorWash/rotor_wash.frag"));

    // program->addShader(osgDB::readRefShaderFile(osg::Shader::VERTEX,"shaders/pass3.vert"));
    // program->addShader(osgDB::readRefShaderFile(osg::Shader::FRAGMENT,"shaders/pass3.frag"));
   
    auto vertShader = osgDB::readRefShaderFile(osg::Shader::VERTEX,"shaders/RotorWash/rotor_wash.vert");

    if (!vertShader) { // 注意：compileShader() 通常由 Program 自动调用，但可以手动触发检查
     std::cout << "Failed to read vertex shader" << std::endl;
    }
    
    osg::ref_ptr<osg::StateSet> ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(
        program.get(),
        osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

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

int main(int , char **)
{
#if 1

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

    viewer->setSceneData( root );

    return viewer->run();

#else

    // construct the viewer.
    osgViewer::Viewer viewer;

    // add model to viewer.
    viewer.setSceneData( createTeapot() );

    // create the windows and run the threads.
    return viewer.run();
#endif

}
