#ifndef TRITON_DRAWABLE_H
#define TRITON_DRAWABLE_H

#include<Triton.h>
#include<osg/Drawable>
#include<osg/TextureCubeMap>
#include<osg/Texture2D>
#include<osg/Camera>

/* Note! To use log depth buffers, you must both uncomment USE_LOG_DEPTH_BUFFER below,
   and modify your Resources/user-vert-functions.glsl file's overridePosition function
   as follows:

    vec4 overridePosition(in vec4 position)
    {
        if (Fcoef > 0.0)
        {
            vec4 adjustedClipPosition = position;
            adjustedClipPosition.z = (log2(max(1e-6, position.w + 1.0))*Fcoef - 1.0) * position.w;
            return adjustedClipPosition;
        }

        return position;
    }

    and, add this at the top of user-vert-functions.glsl:

    uniform float Fcoef = 0;

*/

//#define USE_LOG_DEPTH_BUFFER

struct TritonUpdateCallback : public virtual osg::Drawable::UpdateCallback {
    TritonUpdateCallback(Triton::Ocean *pOcean) : ocean(pOcean) {}

    /** Update the underlying FFT for the waves from the update thread */
    virtual void update(osg::NodeVisitor* nv, osg::Drawable*) {
        if (ocean) {
            // This can cause thread deadlocks and do more harm than good...
            //ocean->UpdateSimulation(nv->getFrameStamp()->getSimulationTime());
        }
    }

    Triton::Ocean *ocean;
};

class TritonDrawable : public osg::Drawable
{
public:
    TritonDrawable( osg::TextureCubeMap * environmentMap = 0, osg::Texture2D* heightMap = 0, osg::Camera* heightMapCamera = 0);

    virtual bool isSameKindAs(const Object* obj) const {
        return dynamic_cast<const TritonDrawable*>(obj)!=NULL;
    }
    virtual Object* cloneType() const {
        return new TritonDrawable();
    }
    virtual Object* clone(const osg::CopyOp& copyop) const {
        return new TritonDrawable();
    }

    virtual void drawImplementation(osg::RenderInfo& renderInfo) const;

protected:

    void Setup( void );
    void Cleanup( void );

    virtual ~TritonDrawable();
    osg::ref_ptr< osg::TextureCubeMap >  _cubeMap;

    // The three main Triton objects you need:
    Triton::ResourceLoader  *_resourceLoader;
    Triton::Environment     *_environment;
    Triton::Ocean           *_ocean;
    osg::Texture2D          *_heightMap;
    osg::Camera             *_heightMapCamera;

};


#endif