// Copyright (c) 2013 Sundog Software, LLC. All rights reserved worldwide.

#ifndef TRITON_IMPACT_H
#define TRITON_IMPACT_H

#ifdef SWIG
%module TritonImpact
%import "Ocean.h"
#define TRITONAPI
%{
#include "Impact.h"
using namespace Triton;
%}
#endif

#include "TritonCommon.h"
#include "Vector3.h"

/** \file Impact.h
    \brief An object that generates impact wave and spray effects, ie from projectiles or explosions.
*/

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
class Ocean;

/** A RotorWash object will generate spray and circular waves on the ocean surface in the direction
    it is pointing. */
class Impact : public MemObject
{
public:
    /** Construct a Impact with the Triton::Ocean it will be associated with.
        \param pOcean The Triton::Ocean object you will associate this Impact with. A common error is to create an Impact before
                      the ocean has been initialized, so make sure this is a valid, non-NULL ocean pointer.
        \param pImpactorDiameter The diameter of the disturbance in world units.
        \param pMass The mass of the impactor in grams
        \param pSprayEffects Whether you wish this impact to emit spray particles.
        \param pSprayScale A scaling factor applied to the initial velocity of spray particles.
    */
    Impact(Ocean *pOcean, double pImpactorDiameter, double pMass, bool pSprayEffects = false, double pSprayScale = 2.0);

    /** Starts the effect of an impact at a given position, direction, and velocity. No impact will be generated until this is called.
        \param pPosition The position of the impact, in world coordinates. This should be above sea level; a ray cast
                         using the pDirection parameter will be used to find the exact point on the water surface for the effect.
        \param pDirection A normalized direction vector indicating the direction the impact is pointing toward.
        \param pVelocity The velocity of the object producing the impact, in meters per second.
        \param pTime     The current simulated time sample, in seconds. This may be relative to any
                         reference point in time, as long as that reference point is consistent among
                         the multiple calls to Trigger().
    */
    void TRITONAPI Trigger(const Vector3& pPosition, const Vector3& pDirection, double pVelocity, double pTime);

    /** Retrieves the world position of the Impact.
        \return The world position of the Impact, as last specified by Triton::Impact::Trigger().
    */
    Vector3 TRITONAPI GetPosition() const {
        return position;
    }

    /** Retrieves the velocity of the Impact.
        \return The velocity of the Impact in world units per second, as last specified by
                Triton::Impact::Trigger().
    */
    double TRITONAPI GetVelocity() const {
        return velocity;
    }

protected:
    Vector3 position;
    double velocity, impactorDiameter;
    Ocean *ocean;
    bool sprayEffects;
    int wakeNumber;
    int numSprays;
    double energyScale;
    double sprayScale;
    double mass;
    double maxEnergy;
    double positionVariation;
    float transparency;
    double decayRate;
};
}

#pragma pack(pop)

#endif