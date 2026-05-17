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

#ifndef MESHCREATION_H
#define MESHCREATION_H

#include <NiFloatInterpolator.h>
#include <NiSample.h>
#include <NiStreamProcessor.h>
#include <NiTStridedRandomAccessIterator.h>
#include <NiPSKernelDefinitions.h>

class HeightField;

class NiUICheckBox;
class NiUISlider;
class NiDataStream;
class NiTritonRenderClick;

namespace Triton
{
class Ocean;
class Environment;
class ResourceLoader;
}

//--------------------------------------------------------------------------------------------------
// Programmatic Mesh Creation Sample Class
//--------------------------------------------------------------------------------------------------
class MeshCreation : public NiSample
{
public:
    MeshCreation();
    ~MeshCreation();

    virtual void UpdateFrame();
    virtual void Terminate();
    virtual bool CreateRenderer();
protected:
    virtual bool CreateScene();

    bool CreateOcean();
    void InitializeOcean();

    // Creates the terrain
    NiMeshPtr CreateTerrainMesh(NiMesh* pkSceneTerrain);

    // Create the sky mesh
    NiMeshPtr CreateSkyMesh(NiMesh* pkSceneSky);

    void QuitMeshCreation(void);

    bool m_bUpdate;
    NiWireframeProperty* m_pkWireProp;
    HeightField* m_pkHeightField;
    NiMesh* m_pkSky;

    NiFloatInterpolator* m_pkBirthRateInterpolator;
    NiPSKernelColorKey* m_pkColorKeys;

    NiTritonRenderClick *m_pkTritonRenderClick;

    Triton::Ocean *m_pkOcean;
    Triton::Environment *m_pkEnvironment;
    Triton::ResourceLoader *m_pkResourceLoader;

};

#include "MeshCreation.inl"

#endif
