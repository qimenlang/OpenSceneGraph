// Copyright (c) 2004-2011  Sundog Software, LLC. All rights reserved worldwide.

/**
    \file Matrix3.h
    \brief Implements a 3x3 matrix and its operations.
 */
#ifdef SWIG
%module TritonMatrix3
%include "Vector3.h"
#define TRITONAPI
%{
#include "Matrix3.h"
using namespace Triton;
%}
#endif

#ifndef TRITON_MATRIX3_H
#define TRITON_MATRIX3_H

#include "MemAlloc.h"
#include "Vector3.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** A simple 3x3 matrix class and its operations. */
class Matrix3 : public MemObject
{
public:
    static Matrix3 Identity;

    /** Default contructor; performs no initialization for efficiency. */
    Matrix3() {
    }

    /** Constructor that instantiates the 3x3 matrix with initial values. */
    Matrix3(double e11, double e12, double e13,
            double e21, double e22, double e23,
            double e31, double e32, double e33) {
        elem[0][0] = e11;
        elem[0][1] = e12;
        elem[0][2] = e13;
        elem[1][0] = e21;
        elem[1][1] = e22, elem[1][2] = e23;
        elem[2][0] = e31, elem[2][1] = e32, elem[2][2] = e33;
    }

    /** Constructor that takes an array of 9 doubles in row-major order. */
    Matrix3(double *m) {
        int i = 0;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                elem[row][col] = m[i++];
            }
        }
    }

    /** Destructor. */
    ~Matrix3() {
    }

    /** Returns a static 3x3 float array in row major order. */
    void TRITONAPI ToFloatArray(float val[9]) const {
        int i = 0;
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                val[i++] = (float)elem[row][col];
            }
        }
    }

    /** Populates the matrix to model a rotation about the X axis by a given
       amount, in radians. */
    void TRITONAPI FromRx(double rad);

    /** Populates the matrix to model a rotation about the Y axis by a given
       amount, in radians. */
    void TRITONAPI FromRy(double rad);

    /** Populates the matrix to model a rotation about the Z axis by a give
       amount, in radians. */
    void TRITONAPI FromRz(double rad);

    /** Populates the matrix as a series of rotations about the X, Y, and Z
       axes (in that order) by specified amounts in radians. */
    void TRITONAPI FromXYZ(double Rx, double Ry, double Rz);

    /** Multiplies two matrices together. */
    Matrix3 TRITONAPI operator * (const Matrix3& mat);

    /** Multiplies the matrix by a vector, yielding another 3x1 vector. */
    Vector3 TRITONAPI operator* (const Vector3& rkVector) const;

    /** Multiplies a 1x3 vector by a matrix, yielding a 1x3 vector. */
    friend Vector3 TRITONAPI operator * (const Vector3& vec, const Matrix3& mat);

    /** Caculate the inverse of the matrix. */
    Matrix3 TRITONAPI Transpose() const;

/// The data members are public for convenience.
    double elem[3][3];
};
}

#pragma pack(pop)

#endif
