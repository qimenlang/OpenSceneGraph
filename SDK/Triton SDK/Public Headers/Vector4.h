// Copyright (c) 2004-2012  Sundog Software, LLC. All rights reserved worldwide.

/**
    \file Vector4.h
    \brief A simple 4D vector class.
 */

#ifdef SWIG
%module TritonVector4
#define TRITONAPI
%{
#include "Vector4.h"
using namespace Triton;
%}
#endif

#ifndef TRITON_VECTOR4_H
#define TRITON_VECTOR4_H

#include "MemAlloc.h"
#include "Vector3.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** A simple double-precision 4D vector class with no operations defined.
   Essentially a struct with constructors. */
class Vector4 : public MemObject
{
public:
    /** Constructs a Vector4 from the given x, y, z, and w values. */
    Vector4(double px, double py, double pz, double pw) : x(px), y(py), z(pz), w(pw) {
    }

    /** Constructs a Vector4 from a Vector3, setting w to 1. */
    Vector4(const Vector3& v3) : x(v3.x), y(v3.y), z(v3.z), w(1.0) {
    }

    /** Default constructor; initializes the Vector4 to (0, 0, 0, 1) */
    Vector4() : x(0), y(0), z(0), w(1.0) {
    }

    /** Determines the dot product between this vector and another, and returns
    the result. */
    double TRITONAPI Dot (const Vector4& v) const {
        return x * v.x + y * v.y + z * v.z + w * v.w;
    }

    /** Scales each x,y,z value of the vector by a constant n, and returns the result. */
    Vector4 TRITONAPI operator * (double n) const {
        return Vector4(x*n, y*n, z*n, w*n);
    }

    /** Multiplies the components of two vectors together, and returns the result. */
    Vector4 TRITONAPI operator * (const Vector4& v) const {
        return Vector4(x*v.x, y*v.y, z*v.z, w*v.w);
    }

    /** Adds a constant n to each component of the vector, and returns the result. */
    Vector4 TRITONAPI operator + (double n) const {
        return Vector4(x+n, y+n, z+n, w+n);
    }

    /** Subtracts the specified vector from this vector, and returns the result. */
    Vector4 TRITONAPI operator - (const Vector4& v) const {
        return Vector4(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    /** Adds this vector to the specified vector, and returns the result. */
    Vector4 TRITONAPI operator + (const Vector4& v) const {
        return Vector4(x + v.x, y + v.y, z + v.z, w + v.w);
    }
    /** The x, y, z, and w data members are public for convenience. */
    double x, y, z, w;
};
}

#pragma pack(pop)

#endif
