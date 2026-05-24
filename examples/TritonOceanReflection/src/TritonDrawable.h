#ifndef TRITON_DRAWABLE_H
#define TRITON_DRAWABLE_H

#include<Triton.h>
#include<osg/Drawable>
#include<osg/TextureCubeMap>
#include<osg/Texture2D>
#include<osg/Node>

struct TritonUpdateCallback : public virtual osg::Drawable::UpdateCallback {
    TritonUpdateCallback(Triton::Ocean *pOcean) : ocean(pOcean) {}

    /** Update the underlying FFT for the waves from the update thread */
    virtual void update(osg::NodeVisitor* nv, osg::Drawable*) {
        if (ocean) {
            // This can actually cause thread deadlocks and do more harm than good...
            //ocean->UpdateSimulation(nv->getFrameStamp()->getSimulationTime());
        }
    }

    Triton::Ocean *ocean;
};

class TritonDrawable : public osg::Drawable
{
public:
    TritonDrawable( bool geocentric = false,
                    osg::TextureCubeMap * cubeMap = NULL, osg::RefMatrix * cubeMapProjection = NULL,
                    osg::Texture2D * planarReflectionMap = NULL, osg::RefMatrix * planarReflectionProjection = NULL );

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

    void UpdateShipPos( double x, double y, double z, double dx, double dy, double dz, double velocity );

    /** Node whose world-space center drives RotorWash (e.g. a cube MatrixTransform). */
    void setRotorWashSourceNode(osg::Node* node) { _rotorWashSource = node; }

protected:

    void Setup( void );
    void Cleanup( void );

    virtual ~TritonDrawable();
    osg::ref_ptr< osg::TextureCubeMap >  _cubeMap;
    osg::ref_ptr< osg::RefMatrix >       _cubeMapProjection;
    osg::ref_ptr< osg::Texture2D >       _planarReflectionMap;
    osg::ref_ptr< osg::RefMatrix >       _planarReflectionProjection;

    // The three main Triton objects you need:
    Triton::ResourceLoader *_resourceLoader;
    Triton::Environment    *_environment;
    Triton::Ocean          *_ocean;
    Triton::WakeGenerator  *_ship;
    bool                    _geocentric;
    osg::Vec3d              _shipPosition;
    osg::Vec3d              _shipDirection;
    float                   _shipVelocity;

    Triton::RotorWash      *_rotorWash;
    osg::observer_ptr<osg::Node> _rotorWashSource;

    void applyUserRotorCenterUniform(osg::State& state) const;

    mutable int _userRotorCenterLoc[2];

};


#endif