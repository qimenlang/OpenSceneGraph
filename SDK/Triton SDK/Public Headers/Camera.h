// Copyright (c) 2011-2016 Sundog Software, LLC. All rights reserved worldwide.

#ifndef TRITON_CAMERA_H
#define TRITON_CAMERA_H

#ifdef SWIG
%module TritonCamera
%import "Ocean.h"
#define TRITONAPI
%{
#include "Camera.h"
using namespace Triton;
%}
#endif

#include "TritonCommon.h"
#include "Vector3.h"
#include "CoordinateSystem.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
class Frustum;
class OrientedBoundingBox;
class Camera;

template< class Param1 >
class signal1;

typedef signal1< const Camera* > CameraSignal;

/** Triton's public interface for specifying the camera properties. *Do not* instantiate this
    directly*/
class Camera : public MemObject
{
public:
    Camera(CoordinateSystem cs);
    ~Camera();

    /** Sets the modelview matrix used for rendering the ocean; this must be called every frame prior to calling
        Ocean::Draw() if your camera orientation or position changes.

        \param m A pointer to 16 doubles representing a 4x4 modelview matrix.
        \param explicitCameraPosition In flat coordinate systems, this parameter can be used to "fool"
         Triton into using a camera position that is different from the one embedded in the view matrix
         provided in the "m" parameter. This can be useful if you need to center the high-resolution ocean
         geometry someplace other than the camera position, for example in very tight zooms on very distant
         locations. In normal situations, you'll want to just leave this set to NULL. This trick doesn't
         work in WGS84 systems as its projected grid LOD scheme is independent of the camera position.
         When used, this parameter should point to 3 doubles representing the camera position's XYZ coordinates.
    */
    void TRITONAPI SetCameraMatrix(const double *m, const double *explicitCameraPosition = 0);

    /** Sets the projection matrix used for rendering the ocean; this must be called every frame prior to calling
        Ocean::Draw().

        \param p A pointer to 16 doubles representing a 4x4 projection matrix.
    */
    void TRITONAPI SetProjectionMatrix(const double *p);

    /** Retrieves an array of 16 doubles representing the modelview matrix passed in via SetCameraMatrix(). */
    const double * TRITONAPI GetCameraMatrix() const {
        return modelview;
    }

    /** Retrieves an array of 16 doubles representing the projection matrix passed in via SetProjectionMatrix(). */
    const double * TRITONAPI GetProjectionMatrix() const {
        return projection;
    }

    /** Retrieves an array of 3 doubles representing the X, Y, and Z position of the camera, extracted from the
        modelview matrix passed in via SetCameraMatrix(). */
    const double * TRITONAPI GetCameraPosition() const {
        return camPos;
    }

    /** Retrieves a normalized vector pointing "up", based on the coordinate system specified in
        Environment::Initialize() and the current position from the modelview matrix passed in through
        Environment::SetCameraMatrix().*/
    Vector3 TRITONAPI GetUpVector() const;

    /** Retrieves a normalized vector pointing "right", based on the coordinate system specified in
        Environment::Initialize() and the current position from the modelview matrix passed in through
        Environment::SetCameraMatrix().*/
    Vector3 TRITONAPI GetRightVector() const;

    /** Returns the CoordinateSystem passed set on the camera, indicating the up vector and
    the presence of a geocentric or flat coordinate system. */
    CoordinateSystem TRITONAPI GetCoordinateSystem(void) const {
        return coordinateSystem;
    }

    /** Returns true if the given sphere lies within the view frustum, as defined by the modelview - projection
        matrix passed in via SetCameraMatrix() and SetProjectionMatrix().
        \param position The center of the sphere in world coordinates.
        \param radius The radius of the sphere in world coordinates.
        \return True if the sphere is not visible and should be culled.
    */
    bool TRITONAPI CullSphere(const Vector3& position, double radius) const;

    bool TRITONAPI CullOrientedBoundingBox(const OrientedBoundingBox& obb) const;

    /** Returns whether the CoordinateSystem passed set on the camera is geocentric,
    indicating an elliptical or spherical coordinate system where all points are relative to the center
    of the Earth. */
    bool TRITONAPI IsGeocentric() const {
        return coordinateSystem < FLAT_ZUP;
    }

    /** Associated a name with a camera. (Useful for debugging, etc) */
    void TRITONAPI SetName(const char* name);

    /** Get the name */
    const char* TRITONAPI GetName(void) const;

    /** Called just before the camera is deleted. Used internally to track cameras.
    Do *not* use directly
    */
    CameraSignal* signal_CameraDestroyed;

    /** Informs Triton of the current viewport position and size. Calling this is optional, but allows Triton to avoid
    querying OpenGL or DirectX for the current viewport parameters, which can cause a pipeline stall. If you call
    this method, you are responsible for calling it whenever the viewport changes.

        \param x The x position of the current viewport origin.
        \param y The y position of the current viewport origin.
        \param width The width of the current viewport.
        \param height The height of the current viewport.
    */
    void TRITONAPI SetViewport(int x, int y, int width, int height);

    /** Retrieves any viewport information previously set via SetViewport(). If SetViewport() has not been called, this
    method will return false and return zeros for all parameters.

        \param x The x position of the current viewport origin.
        \param y The y position of the current viewport origin.
        \param width The width of the current viewport.
        \param height The height of the current viewport.

        \return true if SetViewport was previously called, and valid information is returned.
    */
    bool TRITONAPI GetViewport(int& x, int& y, int& width, int& height) const;

    /** Abstractly set/get the render target*/
    void TRITONAPI SetRenderTarget(void* target);
    void* TRITONAPI GetRenderTarget(void) const;

    bool operator==(const Camera& rhs) const;
    bool operator!=(const Camera& rhs) const;
protected:
    double modelview[16], projection[16];

    double camPos[3];

    void ExtractFrustum();

    Frustum *frustum;

    CoordinateSystem coordinateSystem;

    Camera();

    TRITON_STRING name;

    int viewport[4];

    void* target;
};

}

#pragma pack(pop)


#endif
