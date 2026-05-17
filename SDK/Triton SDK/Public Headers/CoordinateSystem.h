// Copyright (c) 2011-2016 Sundog Software LLC. All rights reserved worldwide.

#ifndef TRITON_COORDINATESYSTEM_H
#define TRITON_COORDINATESYSTEM_H

#ifdef SWIG
%module TritonCoordinateSystem
#define TRITONAPI
%{
#include "CoordinateSystem.h"
using namespace Triton;
%}
#endif

namespace Triton
{
/** Supported coordinate systems for the Environment constructor. Triton will render the ocean using
geocentric coordinate systems relative to the center of the Earth (elliptical or spherical) or
using cartesian systems where sea level is at 0 on the up axis.*/
enum CoordinateSystem {
    WGS84_ZUP,          /** Elliptical WGS84 earth model, Z axis points up */
    WGS84_YUP,          /** Elliptical WGS84 earth model, Y axis points up */
    SPHERICAL_ZUP,      /** Spherical earth model, Z axis points up */
    SPHERICAL_YUP,      /** Spherical earth model, Y axis points up */
    FLAT_ZUP,           /** Flat earth model, Z axis points up */
    FLAT_YUP,           /** Flat earth model, Y axis points up */
};
}


#endif