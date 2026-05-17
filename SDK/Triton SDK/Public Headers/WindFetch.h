// Copyright (c) 2011-2013 Sundog Software LLC. All rights reserved worldwide.

#ifndef TRITON_WINDFETCH_H
#define TRITON_WINDFETCH_H

/** \file WindFetch.h
   \brief A localized or global area of wind of given speed and direction.
 */
#ifdef SWIG
%module TritonWindFetch
#define TRITONAPI
%{
#include "WindFetch.h"
using namespace Triton;
%}
#endif

#include "Vector3.h"

#pragma pack(push)
#pragma pack(8)

namespace Triton
{
/** A localized or global area of wind of given speed and direction. */
class WindFetch : public MemObject
{
public:
    /** Default constructor. */
    WindFetch();

    /** Sets the wind speed and direction of this wind fetch.
        \param speed        The wind speed in world units per second. Note, setting this 
        in excess of our stated limits (up to Beaufort Scale 9 or appx 30 m/s, may result
        in numerical instability.
        \param direction    The wind direction, in radians clockwise from North. Note
                            this is the direction the wind is coming from - the waves
                            will move in the opposite direction.
    */
    void TRITONAPI SetWind(double speed, double direction);

    /** Sets a localized area, in the form of an ellipsoid, in which the wind fetch is active.
        If this method is not called, the wind fetch is assumed to be global. If using the
        JONSWAP model, the size of the fetch specified will be used to determine the fetch
        length. It's important that this is realistic; fetch lengths on the order of 100km
        will produce the results you probably expect - so the distance from the observer
        to the center of the ellipsoid defined by this method should be on that order.

        The center represents the 'origin' of the wind, and the wind will only affect
        observers within the ellipsoid defined by the radii given.

        \sa ClearLocalization()
        \sa SetFetchLength()

        \param center   The center of the ellipsoid that models the bounds of the wind fetch.
        \param radii    The radii in X, Y, and Z of the ellipsoid, in world units.
    */
    void TRITONAPI SetLocalization(const Vector3& center, const Vector3& radii);

    /** If using the JONSWAP model, swells will increase depending on the "fetch length," or the
        distance the wind has travelled. If you specified a fetch radius using SetLocalization(),
        this would normally be used to determine the fetch length. However, you may set an explicit
        fetch length (in world units) using this method, which will override SetLocalization() until
        ClearFetchLength() is called.

        If you are not using the JONSWAP model, this value is ignored.

        \sa ClearFetchLength()

        \param fetch The distance the wind has travelled before reaching the observer. Typical values
        are on the order of 100km.
    */
    void TRITONAPI SetFetchLength(double fetch);

    /** Clears any localization and makes this wind fetch globally applied. \sa SetLocalization() */
    void TRITONAPI ClearLocalization();

    /** Clears any explicit fetch length specified by SetFetchLength(). */
    void TRITONAPI ClearFetchLength();

    /** Retrieves the wind direction and speed from this wind fetch at the given location. If the
        location specified is not included by the bounds of this wind fetch, or the wind fetch is not
        global, no wind will be returned.

        \param position         The location at which you want to retrieve wind information from this fetch.
        \param windSpeed        The wind speed, in units per second, resulting from this fetch at the given position.
        \param windDirection    The direction of the wind, in radians, resulting from this fetch at the given position.
                                Represents the direction the wind is coming from, clockwise from North.
        \param fetchLength      The distance the wind has travelled before reaching this position. Used only by the JONSWAP
                                model. This will be based on distance from the fetch origin set with SetLocalization(), unless
                                SetFetchLength() has been called which will take precedence. If neither SetLocalization() or
                                SetFetchLength() has been called on this WindFetch, 0 will be returned and the JONSWAP
                                model will switch to the PIERSON_MOSKOWITZ model.
    */
    void TRITONAPI GetWindAtLocation(const Vector3& position, double& windSpeed, double& windDirection, double& fetchLength) const;

    // Called internally:
    void TRITONAPI AdjustForLeftHanded();

protected:
    bool isGlobal;
    Vector3 center, radii;
    double speed, direction, fetchLength;
};
}

#pragma pack(pop)

#endif
