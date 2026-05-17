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

#include "HeightField.h"

#include <NiPixelData.h>
#include <efd/File.h>
#include <NiBMPReader.h>

//--------------------------------------------------------------------------------------------------
HeightField::HeightField(const char* szBmpFileName)
{
    // Init member data
    m_pkPixelData = NULL;

    // Open the file
    efd::File* pkFile = efd::File::GetFile(szBmpFileName,
                                           efd::File::READ_ONLY);
    if ((!pkFile) || (!(*pkFile))) {
        NILOG("Could not find source pixel data for height field.\n");
        NiDelete pkFile;
        return;
    }

    // Remove the height field from the scene
    NiBMPReader kReader;
    m_pkPixelData = kReader.ReadFile(*pkFile, NULL);
    if (!m_pkPixelData) {
        NILOG("Could not find source pixel data for height field.\n");
        return;
    }

    // Delete the file handle
    NiDelete pkFile;
    pkFile = NULL;

    if (m_pkPixelData->GetPixelFormat() != NiPixelFormat::RGB24) {
        NiDelete m_pkPixelData;
        m_pkPixelData = NULL;
        return;
    }
}

//--------------------------------------------------------------------------------------------------
HeightField::~HeightField()
{
    NiDelete m_pkPixelData;
    m_pkPixelData = NULL;
}

//--------------------------------------------------------------------------------------------------
bool HeightField::IsValid() const
{
    return (m_pkPixelData != NULL);
}

//--------------------------------------------------------------------------------------------------
float HeightField::GetHeight(const float fX, const float fY) const
{
    NiUInt32 uiWidth = m_pkPixelData->GetWidth();
    NiUInt32 uiHeight = m_pkPixelData->GetHeight();
    const NiUInt8* pcHeightField = m_pkPixelData->GetPixels();

    float fNormX = ((fX - MIN_TERRAIN_SIZE_X) /
                    (MAX_TERRAIN_SIZE_X - MIN_TERRAIN_SIZE_X)) * (float)uiWidth;
    float fNormY = ((fY - MIN_TERRAIN_SIZE_Y) /
                    (MAX_TERRAIN_SIZE_Y - MIN_TERRAIN_SIZE_Y)) * (float)uiHeight;

    NiUInt32 uiMinPosX = (NiUInt32)(fNormX);
    NiUInt32 uiMaxPosX = uiMinPosX + 1;
    NiUInt32 uiMinPosY = (NiUInt32)(fNormY);
    NiUInt32 uiMaxPosY = uiMinPosY + 1;

    float fXMaxFactor = (fNormX - (float)uiMinPosX);
    float fYMaxFactor = (fNormY - (float)uiMinPosY);

    if (uiMaxPosX >= uiWidth) {
        uiMaxPosX = uiWidth - 1;
        fXMaxFactor = 0.0f;
    }

    if (uiMaxPosY >= uiHeight) {
        uiMaxPosY = uiHeight - 1;
        fYMaxFactor = 0.0f;
    }

    float fXMinFactor = 1.0f - fXMaxFactor;
    float fYMinFactor = 1.0f - fYMaxFactor;

    float fHeight00 =
        ((float)pcHeightField[(uiMinPosY *
                               uiWidth + uiMinPosX) * 3] / 255.0f);
    float fHeight10 =
        ((float)pcHeightField[(uiMinPosY *
                               uiWidth + uiMaxPosX) * 3] / 255.0f);
    float fHeight01 =
        ((float)pcHeightField[(uiMaxPosY *
                               uiWidth + uiMinPosX) * 3] / 255.0f);
    float fHeight11 =
        ((float)pcHeightField[(uiMaxPosY *
                               uiWidth + uiMaxPosX) * 3] / 255.0f);

    float fHeightFactor = ((fHeight00 * fXMinFactor * fYMinFactor) +
                           (fHeight10 * fXMaxFactor * fYMinFactor) +
                           (fHeight01 * fXMinFactor * fYMaxFactor) +
                           (fHeight11 * fXMaxFactor * fYMaxFactor));

    return MIN_TERRAIN_SIZE_Z +
           (fHeightFactor * (MAX_TERRAIN_SIZE_Z - MIN_TERRAIN_SIZE_Z));

}

//--------------------------------------------------------------------------------------------------
