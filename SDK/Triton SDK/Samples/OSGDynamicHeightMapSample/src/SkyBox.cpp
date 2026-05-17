#include <osg/Vec3>
#include <osg/Vec4>
#include <osg/Quat>
#include <osg/Matrix>
#include <osg/ShapeDrawable>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Transform>
#include <osg/Material>
#include <osg/NodeCallback>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/TexMat>
#include <osg/TexGen>
#include <osg/TexEnv>
#include <osg/TexEnvCombine>
#include <osg/TextureCubeMap>
#include <osg/VertexProgram>

#include <osgDB/Registry>
#include <osgDB/ReadFile>

#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Optimizer>

#include <osgViewer/Viewer>

#include <iostream>

#ifndef GL_TEXTURE_CUBE_MAP_SEAMLESS
#define GL_TEXTURE_CUBE_MAP_SEAMLESS 0x884F
#endif

using namespace osg;

#define OSG_TEST_CUBEMAP 0

static TextureCubeMap* readCubeMap( )
{
    osg::ref_ptr< TextureCubeMap> cubemap = new osg::TextureCubeMap();

#if OSG_TEST_CUBEMAP
    // OSG test cubemap - useful for testing reflections
#define CUBEMAP_FILENAME(face) "../" #face ".png"
#else
#define CUBEMAP_FILENAME(face) #face ".tga"
#endif

    const char *tritonPath = getenv("TRITON_PATH");
    if (!tritonPath) {
        printf("Can't find Triton; set the TRITON_PATH environment variable ");
        printf("to point to the directory containing the SDK.\n");
        exit(0);
    }

    std::string resPath(tritonPath);
#ifdef _WIN32
    resPath += "\\Samples\\";
#else
    resPath += "/Samples/";
#endif

    Image* imagePosX = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(posX));
    Image* imageNegX = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(negX));
    Image* imagePosY = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(posY));
    Image* imageNegY = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(negY));
    Image* imagePosZ = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(posZ));
    Image* imageNegZ = osgDB::readImageFile(resPath + CUBEMAP_FILENAME(negZ));

    if (imagePosX && imageNegX && imagePosY && imageNegY && imagePosZ && imageNegZ) {
        cubemap->setImage(TextureCubeMap::POSITIVE_X, imagePosX);
        cubemap->setImage(TextureCubeMap::NEGATIVE_X, imageNegX);
        cubemap->setImage(TextureCubeMap::POSITIVE_Y, imagePosY);
        cubemap->setImage(TextureCubeMap::NEGATIVE_Y, imageNegY);
        cubemap->setImage(TextureCubeMap::POSITIVE_Z, imagePosZ);
        cubemap->setImage(TextureCubeMap::NEGATIVE_Z, imageNegZ);

        cubemap->setWrap(Texture::WRAP_S, Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(Texture::WRAP_T, Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(Texture::WRAP_R, Texture::CLAMP_TO_EDGE);

        cubemap->setFilter(Texture::MIN_FILTER, Texture::LINEAR);
        cubemap->setFilter(Texture::MAG_FILTER, Texture::LINEAR);
    }

    return cubemap.release();
}

static Matrix * worldToCubeMap( void )
{
    static Matrix R = Matrix::rotate( DegreesToRadians( 90.0f ), 1.0f,0.0f,0.0f ) ;
    //R.makeIdentity();
    return (Matrix*)&R;
}


// Update texture matrix for cubemaps
struct TexMatCallback : public NodeCallback {
public:

    TexMatCallback(TexMat& tm) :
        _texMat(tm) {
    }

    virtual void operator()(Node* node, NodeVisitor* nv) {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        if (cv) {
            const Matrix& MV = *cv->getModelViewMatrix();
            const Matrix& R = *worldToCubeMap();

            Quat q;
            // If you get a compile error on the following line, you're using an older
            // version of OSG and should just replace it with MV.get(q)
            q = MV.getRotate();
            const Matrix C = Matrix::rotate( q.inverse() );

            _texMat.setMatrix( C * R );
        }

        traverse(node,nv);
    }

    TexMat& _texMat;
};

class MoveEarthySkyWithEyePointTransform : public Transform
{
public:
    /** Get the transformation matrix which moves from local coords to world coords.*/
    virtual bool computeLocalToWorldMatrix(Matrix& matrix,NodeVisitor* nv) const {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        if (cv) {
            Vec3 eyePointLocal = cv->getEyeLocal();
            matrix.preMult(Matrix::translate(eyePointLocal));
        }
        return true;
    }

    /** Get the transformation matrix which moves from world coords to local coords.*/
    virtual bool computeWorldToLocalMatrix(Matrix& matrix,NodeVisitor* nv) const {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        if (cv) {
            Vec3 eyePointLocal = cv->getEyeLocal();
            matrix.postMult(Matrix::translate(-eyePointLocal));
        }
        return true;
    }
};

Node* createSkyBox( osg::TextureCubeMap * & skymap )
{
    skymap = readCubeMap( );
    StateSet* stateset = new StateSet();

    TexEnv* te = new TexEnv;
    te->setMode(TexEnv::REPLACE);
    stateset->setTextureAttributeAndModes(0, te, StateAttribute::ON);

    TexGen *tg = new TexGen;
    tg->setMode(TexGen::NORMAL_MAP);
    stateset->setTextureAttributeAndModes(0, tg, StateAttribute::ON);

    TexMat *tm = new TexMat;
    stateset->setTextureAttribute(0, tm);

    stateset->setTextureAttributeAndModes(0, skymap, StateAttribute::ON);

    stateset->setMode( GL_TEXTURE_CUBE_MAP_SEAMLESS, osg::StateAttribute::ON );

    stateset->setMode( GL_LIGHTING, StateAttribute::OFF );
    stateset->setMode( GL_CULL_FACE, StateAttribute::OFF );

    // clear the depth to the far plane.
    Depth* depth = new Depth;
    depth->setFunction(Depth::ALWAYS);
    depth->setRange(1.0,1.0);
    stateset->setAttributeAndModes(depth, StateAttribute::ON );

    stateset->setRenderBinDetails(-1,"RenderBin");

    Drawable* drawable = new ShapeDrawable
    (new Sphere(Vec3(0.0f,0.0f,0.0f),30000 ));

    Geode* geode = new Geode;
    geode->setCullingActive(false);
    geode->setStateSet( stateset );
    geode->addDrawable(drawable);

    Transform* transform = new MoveEarthySkyWithEyePointTransform;
    transform->setCullingActive(false);
    transform->addChild(geode);

    ClearNode* clearNode = new ClearNode;
//  clearNode->setRequiresClear(false);
    clearNode->setCullCallback(new TexMatCallback(*tm));
    clearNode->addChild(transform);

    return clearNode;
}

