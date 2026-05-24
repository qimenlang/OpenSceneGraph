#ifndef TRITON_DRAWABLE_H
#define TRITON_DRAWABLE_H

#include<Triton.h>
#include<osg/Drawable>
#include<osg/Node>
#include<osg/TextureCubeMap>
#include<osg/Fog>

namespace osg { class State; }

struct TritonUpdateCallback : public virtual osg::Drawable::UpdateCallback {
    TritonUpdateCallback(Triton::Ocean *pOcean) : ocean(pOcean) {}

    /** Update the underlying FFT for the waves from the update thread */
    virtual void update(osg::NodeVisitor* nv, osg::Drawable*) {
        if (ocean) {
            // This can actually cause thread deadlocks and usually does more harm
            // than good...
            //ocean->UpdateSimulation(nv->getFrameStamp()->getSimulationTime());
        }
    }

    Triton::Ocean *ocean;
};

class TritonDrawable : public osg::Drawable
{
public:
    TritonDrawable( osg::TextureCubeMap * cubeMap = NULL, osg::Fog *fog = NULL );

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

    /** Node whose world-space center drives RotorWash (e.g. a cube MatrixTransform). */
    void setRotorWashSourceNode(osg::Node* node) { _rotorWashSource = node; }

protected:

    void Setup( void );
    void Cleanup( void );

    virtual ~TritonDrawable();
    osg::ref_ptr< osg::TextureCubeMap >  _cubeMap;

    // The three main Triton objects you need:
    Triton::ResourceLoader *_resourceLoader;
    Triton::Environment    *_environment;
    Triton::Ocean          *_ocean;
    Triton::RotorWash      *_rotorWash;
    osg::observer_ptr<osg::Node> _rotorWashSource;

    osg::ref_ptr<osg::Fog> _fog;

    Triton::Vector3 _aboveWaterFogColor, _belowWaterFogColor;
    double _aboveWaterVisibility, _belowWaterVisibility;

    void applyUserRotorCenterUniform(osg::State& state) const;

    mutable int _userRotorCenterLoc[2];

};


#endif
