// Copyright (c) 2012-2013 Sundog Software LLC. All rights reserved worldwide.

#ifndef TRITON_RANDOM_NUMBER_GENERATOR_H
#define TRITON_RANDOM_NUMBER_GENERATOR_H

/** \file RandomNumberGenerator.h
    \brief An interface for overriding Triton's generation of random numbers.
*/
#include "TritonCommon.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** An interface for generating random numbers in Triton. Subclass this interface and pass an
    instance to Environment::SetRandomNumberGenerator in order to override Triton's default
    usage of rand(). This may be useful for enforcing deterministic behavior across several
    channels. */
class RandomNumberGenerator : public MemObject
{
public:
    RandomNumberGenerator() {}
    virtual ~RandomNumberGenerator() {}

    /** Return an evenly distributed random double-precision number within a given range.
        \param start The lowest value in the range
        \param end The highest value in the range
        \return An evenly distributed random number within the range.
    */
    virtual double TRITONAPI GetRandomDouble(double start, double end) const = 0;

    /** Return an evenly distributed random integer within a given range.
        \param start The lowest value in the range
        \param end The highest value in the range
        \return An evenly distributed random number within the range.
    */
    virtual int TRITONAPI GetRandomInt(int start, int end) const = 0;

    /** Seeds the random number generator with a given value, to ensure consistent
        results.
        \param seed A value used to seed the random number generator's sequence
                    of psuedo-random numbers.
    */
    virtual void TRITONAPI SetRandomSeed(unsigned int seed) = 0;
};
}

#pragma pack(pop)

#endif
