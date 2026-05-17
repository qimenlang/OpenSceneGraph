// Copyright (c) 2013-2014 Sundog Software, LLC. All rights reserved worldwide.

#ifndef TRITON_OREINTED_BOUNDING_BOX_H
#define TRITON_OREINTED_BOUNDING_BOX_H

/** \file OrientedBoundingBox.h
\brief A class describing an oriented bounding box.
*/

#include "TritonCommon.h"
#include "Vector3.h"
#include "Matrix3.h"

namespace Triton
{
/** An oriented bounding box defined by a center point and three axes. */
class OrientedBoundingBox : public MemObject
{
public:
    /** Constructor. */
    OrientedBoundingBox();

    /** Define the OBB by a center point and vectors from center to extents in X, Y, and Z. */
    void Set(const Vector3& center, const Vector3& xExtent, const Vector3& yExtent, const Vector3& zExtent);

    /** Test if a point is enclosed by the box. */
    bool PointInBox(const Vector3& point, double slop) const;

    /** Recomputes the basis used for PointInBox */
    void RecomputeBasis();

    Vector3 center;
    Vector3 axes[3];
    double halfDistances[3];
    Matrix3 invBasis;
};
}

#endif
