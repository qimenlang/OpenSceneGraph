// Copyright (c) 2004-2012  Sundog Software, LLC All rights reserved worldwide.

/**
    \file Vector3.h
    \brief A 3D Vector class and its operations.
 */
#ifdef SWIG
%module TritonVector3
#define TRITONAPI
%{
#include "Vector3.h"
using namespace Triton;
%}
#endif

#ifndef TRITON_VECTOR3_H
#define TRITON_VECTOR3_H

#if defined(__INTEL_COMPILER)
#include <mathimf.h>
#else
#include <math.h>
#endif

#include "MemAlloc.h"
#include <stdio.h>
#include <iostream>

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** A 3D double-precision Vector class and its operations. */
class Vector3 : public MemObject
{
public:
    /** Default constructor, initializes to (0,0,0). */
    Vector3() {
        x = y = z = 0;
    }

    /** Constructs a Vector3 with the given x,y,z coordinates. */
    Vector3(double px, double py, double pz) : x(px), y(py), z(pz) {
    }

    /** Constructs a Vector 3 from a pointer to 3 doubles */
    Vector3(const double *p) : x(p[0]), y(p[1]), z(p[2]) {
    }

    /** Copy constructor */
    Vector3(const Vector3& v) : x(v.x), y(v.y), z(v.z) {
    }

    /** Returns the length of the vector. */
    double TRITONAPI Length() const {
        return sqrt(x*x + y*y + z*z);
    }

    /** Returns the squared length of the vector, which is faster than computing
       the length. */
    double TRITONAPI SquaredLength() const {
        return (x*x + y*y + z*z);
    }

    /** Scales the vector to be of length 1.0. */
    void TRITONAPI Normalize() {
        if (SquaredLength() > 0) {
            double n = 1.0 / Length();
            x *= n;
            y *= n;
            z *= n;
        }
    }

    /** Determines the dot product between this vector and another, and returns
       the result. */
    double TRITONAPI Dot (const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    /** Determines the cross product between this vector and another, and returns
       the result. */
    Vector3 TRITONAPI Cross (const Vector3& v) const {
        Vector3 kCross;

        kCross.x = y*v.z - z*v.y;
        kCross.y = z*v.x - x*v.z;
        kCross.z = x*v.y - y*v.x;

        return kCross;
    }

    /** Scales each x,y,z value of the vector by a constant n, and returns the result. */
    Vector3 TRITONAPI operator * (double n) const {
        return Vector3(x*n, y*n, z*n);
    }

    /** Multiplies the components of two vectors together, and returns the result. */
    Vector3 TRITONAPI operator * (const Vector3& v) const {
        return Vector3(x*v.x, y*v.y, z*v.z);
    }

    /** Adds a constant n to each component of the vector, and returns the result. */
    Vector3 TRITONAPI operator + (double n) const {
        return Vector3(x+n, y+n, z+n);
    }

    /** Subtracts the specified vector from this vector, and returns the result. */
    Vector3 TRITONAPI operator - (const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z- v.z);
    }

    /** Adds this vector to the specified vector, and returns the result. */
    Vector3 TRITONAPI operator + (const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    /** Tests if two vectors are exactly equal. */
    bool TRITONAPI operator == (const Vector3& v) const {
        return (v.x == x && v.y ==y && v.z == z);
    }

    /** Test if two vectors are not exactly equal. */
    bool TRITONAPI operator != (const Vector3& v) const {
        return (v.x != x || v.y != y || v.z != z);
    }

    /** Write this vector's data to a file. */
    void TRITONAPI Serialize(std::ostream& s) const {
        s.write((char *)&x, sizeof(double));
        s.write((char *)&y, sizeof(double));
        s.write((char *)&z, sizeof(double));
    }

    /** Restore this vector from a file. */
    void TRITONAPI Unserialize(std::istream& s) {
        s.read((char *)&x, sizeof(double));
        s.read((char *)&y, sizeof(double));
        s.read((char *)&z, sizeof(double));
    }

    /** Data members x,y,z public for convenience. */
    double x, y, z;
};

/** A 3D single-precision vector class, and its operations. */
class Vector3f : public MemObject
{
public:
    /** Default constructor; does not initialize the vector. */
    Vector3f() {
    }

    /** Construct a single precision vector from a double precision one. */
    Vector3f(const Triton::Vector3& v) : x((float)v.x), y((float)v.y), z((float)v.z) {
    }

    /** Constructs a Vector3f from the specified single-precision floating
       point x, y, and z values. */
    Vector3f(float px, float py, float pz) : x(px), y(py), z(pz) {
    }

    /** Returns the length of this vector. */
    float TRITONAPI Length() {
        return (float)sqrt(x*x + y*y + z*z);
    }

    /** Scales the vector to be of length 1.0. */
    void TRITONAPI Normalize() {
        float n = 1.0f / Length();
        x *= n;
        y *= n;
        z *= n;
    }

    /** Returns the dot product of this vector with the specified Vector3. */
    double TRITONAPI Dot (const Vector3f& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    /** Subtracts the specified vector from this vector, and returns the result. */
    Vector3f TRITONAPI operator - (const Vector3f& v) const {
        return Vector3(x - v.x, y - v.y, z- v.z);
    }

    /** Adds this vector to the specified vector, and returns the result. */
    Vector3f TRITONAPI operator + (const Vector3f& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    /** Multiplies the components of two vectors together, and returns the result. */
    Vector3f TRITONAPI operator * (const Vector3f& v) const {
        return Vector3f(x*v.x, y*v.y, z*v.z);
    }

    /** Scales each x,y,z value of the vector by a constant n, and returns the result. */
    Vector3f TRITONAPI operator * (float n) const {
        return Vector3f(x*n, y*n, z*n);
    }

    /** Data members x, y, z are public for convenience. */
    float x, y, z;
};


//JWH
class TBoundingBox : public MemObject
{
public:
    TBoundingBox() 
	{
    }

    TBoundingBox(const Triton::Vector3& vMin, const Triton::Vector3& vMax)
	{
		m_vMin = vMin;
		m_vMax = vMax;
    }

	TBoundingBox(double minX, double minY, double minZ, double maxX, double maxY, double maxZ)
	{
		m_vMin = Triton::Vector3(minX, minY, minZ);
		m_vMax = Triton::Vector3(maxX, maxY, maxZ);
	}

	inline bool Contains (const Triton::Vector3& v) const
	{
		return ((v.x >= m_vMin.x) && (v.y >= m_vMin.y) && (v.z >= m_vMin.z) && 
			(v.x <= m_vMax.x) && (v.y <= m_vMax.y) && (v.z <= m_vMax.z));
	}

   Vector3 m_vMin;
   Vector3 m_vMax;
};
}

#pragma pack(pop)

#endif
