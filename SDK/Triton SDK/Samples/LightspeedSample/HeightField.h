// EMERGENT GAME TECHNOLOGIES PROPRIETARY INFORMATION
//
// This software is supplied under the terms of a license agreement or
// nondisclosure agreement with Emergent Game Technologies and may not
// be copied or disclosed except in accordance with the terms of that
// agreement.
//
//      Copyright (c) 1996-2009 Emergent Game Technologies.
//      All Rights Reserved.
//
// Emergent Game Technologies, Calabasas, CA 91302
// http://www.emergent.net

#ifndef HEIGHTFIELD_H
#define HEIGHTFIELD_H

#include "NiMemObject.h"

static const float MIN_TERRAIN_SIZE_X = -512.0f;
static const float MIN_TERRAIN_SIZE_Y = -512.0f;
static const float MIN_TERRAIN_SIZE_Z = -64.0f;
static const float MAX_TERRAIN_SIZE_X = 512.0f;
static const float MAX_TERRAIN_SIZE_Y = 512.0f;
static const float MAX_TERRAIN_SIZE_Z = 64.0f;

class NiPixelData;

//--------------------------------------------------------------------------------------------------
// Class to handle the height field
//--------------------------------------------------------------------------------------------------

class HeightField : public NiMemObject
{
public:
    HeightField(const char* szBmpFileName);
    ~HeightField();

    bool IsValid() const;
    float GetHeight(const float fX, const float fY) const;

private:

    NiPixelData* m_pkPixelData;
};

#endif
