// Copyright (c) 2013-2015 Sundog Software, LLC. All rights reserved worldwide.

#ifndef TRITON_ROTOR_WASH_H
#define TRITON_ROTOR_WASH_H

#ifdef SWIG
%module TritonRotorWash
%import "Ocean.h"
#define TRITONAPI
%{
#include "RotorWash.h"
using namespace Triton;
%}
#endif

#define NUM_DECALS 3
#define SMOOTH_BUFFER_SIZE 10

#include "TritonCommon.h"
#include "Vector3.h"

/** \file RotorWash.h
    \brief An object that generates rotor wash wave and spray effects.
*/

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
class Ocean;
class Decal;

/** A RotorWash object will generate spray and circular waves on the ocean surface in the direction
    it is pointing. */
class RotorWash : public MemObject
{
public:
    /** Construct a RotorWash with the same Triton::Ocean it will be associated with.
        \param pOcean The Triton::Ocean object you will associate this RotorWash with. A common error is to
                      create a RotorWash before the Ocean has been created, so make sure this is a valid,
                      non-null pointer.
        \param pRotorDiameter The diameter of the rotor blades in world units.
        \param pSprayEffects Whether you wish this wash to emit spray particles.
        \param pUseDecals Whether decal textures should be used to provide a more detailed, but costly effect.
    */
    RotorWash(Ocean *pOcean, double pRotorDiameter, bool pSprayEffects = false, bool pUseDecals = false);

    virtual ~RotorWash();

    /** For any active RotorWash, this should be called every frame to update its
        position and velocity. No wash will be generated until this is called.
        \param pPosition The position of the rotor, in world coordinates.
        \param pDirection A normalized direction vector indicating the direction the rotors are pointing down toward.
        \param pVelocity The velocity of the wind generating the wake, in world units per second.
        \param pTime     The current simulated time sample, in seconds. This may be relative to any
                         reference point in time, as long as that reference point is consistent among
                         the multiple calls to Update().
    */
    void TRITONAPI Update(const Vector3& pPosition, const Vector3& pDirection, double pVelocity, double pTime);

    /** Retrieves the world position of the RotorWash.
        \return The world position of the RotorWash, as last specified by Triton::RotorWash::Update().
    */
    Vector3 TRITONAPI GetPosition() const {
        return position;
    }

    /** Retrieves the airspeed velocity of the RotorWash.
        \return The air velocity of the RotorWash in world units per second, as last specified by
                Triton::RotorWash::Update().
    */
    double TRITONAPI GetVelocity() const {
        return velocity;
    }

    // Used internally to clear out the ocean reference when it is deleted.
    void OceanDestroyed() {
        ocean = 0;
    }

    // Used internally when ocean quality changes
    void OceanQualityChanged();

    // toggle decals at run time
    void setUseDecals(bool useDecals);
    bool GetUseDecals(void) const;
protected:
    void UpdateDecals(double time, const Vector3& position, double distanceDampening);

    Vector3 position, lastEmitPosition, lastLastEmitPosition, lastEmitSourcePosition;
    double velocity, rotorDiameter;
    Ocean *ocean;
    double lastWaveEmitTime, lastSprayEmitTime;
    double waveGenerationPeriod, sprayGenerationPeriod;
    double washDiameter;
    double windScale;
    double maxVelocity;
    double particleSize;
    double decayRate;
    double decalIntensity;
    float transparency;
    bool sprayEffects, firstEmit;
    int wakeNumber, lastWakeNumber;
    Decal *decals[NUM_DECALS];
    float decalPeriod;
    double decalStartTimes[NUM_DECALS];
    int currentDecal;
    float decalMinScale, decalMaxScale;
    double phaseOffset, phaseRandomness;
    bool useDecal;
    bool registered;
    Vector3 lastPosition;
    double lastTime, rotorDecalSpeedLimit;
	double velocitySmoothBuffer[SMOOTH_BUFFER_SIZE];
	int bufferEntry;
};
}

#pragma pack(pop)

#endif