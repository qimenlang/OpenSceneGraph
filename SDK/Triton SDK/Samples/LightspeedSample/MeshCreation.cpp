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

// Portions copyright (c) 2011 Sundog Software LLC. All rights reserved worldwide.

#include "MeshCreation.h"
#include "HeightField.h"

#include <NiPoint4.h>
#include <NiUIManager.h>
#include <NiUICheckbox.h>
#include <NiUISlider.h>
#include <NiStandardMaterial.h>
#include <NiCommonMaterialLib.h>
#include <NiCommonSemantics.h>
#include <NiDataStreamElement.h>
#include <NiDataStreamLock.h>
#include <NiAnimation.h>
#include <NiMeshUtilities.h>
#include <NiPSParticleSystem.h>
#include <NiPSEmitParticlesCtlr.h>
#include <NiPSSimulatorGeneralStep.h>
#include <NiMesh.h>
#include <NiLicense.h>

#include <NiDX9Renderer.h>
#include <NiD3D10Renderer.h>
#include <ecrD3D11Renderer/D3D11Renderer.h>
#include <ecrD3D11RendererSetup/D3D11RendererSetup.h>
#include <NiDX9RendererSetup.h>
#include <NiD3D10RendererSetup.h>

#include "Triton.h"

using namespace Triton;

NiEmbedGamebryoLicenseCode;

static const float DESIRED_TERRAIN_QUAD_SIZE = 4.0f;

static const float UV_SCALE_FACTOR = 3.0f;


static void GetProjection(double *pfOut, const NiFrustum &kFrust)
{
    NIASSERT(pfOut);

    double fN = kFrust.m_fNear;
    double fF = kFrust.m_fFar;
    double fL = kFrust.m_fLeft;
    double fR = kFrust.m_fRight;
    double fT = kFrust.m_fTop;
    double fB = kFrust.m_fBottom;

    if( !kFrust.m_bOrtho) {
        pfOut[ 0] = 2.0f/(fR-fL);
        pfOut[ 1] = 0.0f;
        pfOut[ 2] = 0.0f;
        pfOut[ 3] = 0.0f;
        pfOut[ 4] = 0.0f;
        pfOut[ 5] = 2.0f/(fT-fB);
        pfOut[ 6] = 0.0f;
        pfOut[ 7] = 0.0f;
        pfOut[ 8] = (fR+fL)/(fR-fL);
        pfOut[ 9] =-(fT+fB)/(fT-fB);
        pfOut[10] = fF/(fF-fN);
        pfOut[11] = 1.0f;
        pfOut[12] = 0.0f;
        pfOut[13] = 0.0f;
        pfOut[14] = -(fN*fF)/(fF-fN);
        pfOut[15] = 0.0f;
    } else {
        pfOut[ 0] = 2.0f/(fR-fL);
        pfOut[ 1] = 0.0f;
        pfOut[ 2] = 0.0f;
        pfOut[ 3] = 0.0f;
        pfOut[ 4] = 0.0f;
        pfOut[ 5] = 2.0f/(fT-fB);
        pfOut[ 6] = 0.0f;
        pfOut[ 7] = 0.0f;
        pfOut[ 8] = 0.0f;
        pfOut[ 9] = 0.0f;
        pfOut[10] = 1.0/(fF-fN);
        pfOut[11] = 0.0f;
        pfOut[12] =(fR+fL)/(fR-fL);
        pfOut[13] =-(fT+fB)/(fT-fB);
        pfOut[14] = -fN/(fF-fN);
        pfOut[15] = 1.0f;
    }
}

static void GetView(double *pfOut, const NiCamera *pkCamera)
{
    const NiPoint3& kDir = pkCamera->GetWorldDirection();
    NiPoint3 kRight = pkCamera->GetWorldRightVector();
    NiPoint3 kUp = pkCamera->GetWorldUpVector();
    NiPoint3 kLoc = pkCamera->GetWorldLocation();

    pfOut[ 0] = kRight.x;
    pfOut[ 1] = kUp.x;
    pfOut[ 2] = kDir.x;
    pfOut[ 3] = 0.0f;
    pfOut[ 4] = kRight.y;
    pfOut[ 5] = kUp.y;
    pfOut[ 6] = kDir.y;
    pfOut[ 7] = 0.0f;
    pfOut[ 8] = kRight.z;
    pfOut[ 9] = kUp.z;
    pfOut[10] = kDir.z;
    pfOut[11] = 0.0f;
    pfOut[12] = -(kRight * kLoc);
    pfOut[13] = -(kUp * kLoc);
    pfOut[14] = -(kDir * kLoc);
    pfOut[15] = 1.0f;
}

class NiTritonRenderClick : public NiViewRenderClick
{
public:
    NiTritonRenderClick(Triton::Ocean *pOcean, Triton::Environment *pEnv, NiCamera *pCam) : m_pkOcean(pOcean),
        m_pkEnvironment(pEnv), m_spCamera(pCam), NiViewRenderClick() {}

    virtual ~NiTritonRenderClick() {}

    virtual void PerformRendering(unsigned int) {
        if (m_pkOcean) {
            double pdProj[16], pdView[16];

            GetProjection(pdProj, m_spCamera->GetViewFrustum());
            GetView(pdView, m_spCamera);
            m_pkEnvironment->SetProjectionMatrix(pdProj);
            m_pkEnvironment->SetCameraMatrix(pdView);

            DWORD millis = timeGetTime();
            m_pkOcean->Draw((double)millis * 0.001);
        }
    }
private:
    Triton::Ocean *m_pkOcean;
    Triton::Environment *m_pkEnvironment;
    NiCamera *m_spCamera;
};

//--------------------------------------------------------------------------------------------------
bool MeshCreation::CreateOcean()
{
    const char *tritonPath = getenv("TRITON_PATH");
    if (!tritonPath) {
        NILOG("Can't find Triton; set the TRITON_PATH environment variable "
              "to point to the directory containing the SDK.\n");
        return false;
    }

    std::string resPath(tritonPath);
    resPath += "\\Resources\\";
    m_pkResourceLoader = new Triton::ResourceLoader(resPath.c_str());

    m_pkEnvironment = new Triton::Environment();
    Triton::EnvironmentError err = Triton::NO_DEVICE;

    if (NiDX9Renderer::GetRenderer()) {
        err = m_pkEnvironment->Initialize(Triton::FLAT_ZUP,
                                          Triton::DIRECTX_9, m_pkResourceLoader, NiDX9Renderer::GetRenderer()->GetD3DDevice());
    } else if (ecr::D3D11Renderer::GetRenderer()) {
        err = m_pkEnvironment->Initialize(Triton::FLAT_ZUP,
                                          Triton::DIRECTX_11, m_pkResourceLoader, ecr::D3D11Renderer::GetRenderer()->GetD3D11Device());
    } else {
        NILOG("Triton only supports DirectX9 and DirectX11 with Gamebryo.\n");
        return false;
    }

    if (err != Triton::SUCCEEDED) {
        NILOG("Failed to initialize Triton - is the resource path passed in to "
              "the ResouceLoader constructor valid?\n");
        return false;
    }

    m_pkEnvironment->SetLicenseCode("Your license name", "Your license code");

    // Set up wind of 5 m/s blowing North
    Triton::WindFetch wf;
    wf.SetWind(5.0, 0.0);
    m_pkEnvironment->AddWindFetch(wf);

    m_pkEnvironment->SetSeaLevel(2.0);

    Triton::Vector3 skyColor(95.0 / 255.0, 95.0 / 255.0, 156.0 / 255.0);
    m_pkEnvironment->SetAmbientLight(Triton::Vector3(1.0, 1.0, 1.0));
    m_pkEnvironment->SetDirectionalLight(Triton::Vector3(0, 0, 1), Triton::Vector3(1.0, 1.0, 1.0));
    m_pkEnvironment->SetAboveWaterVisibility(5000.0, skyColor);

    m_pkOcean = Triton::Ocean::Create(m_pkEnvironment, Triton::TESSENDORF);
    m_pkOcean->EnableSpray(false);

    return true;
}

//--------------------------------------------------------------------------------------------------
void MeshCreation::InitializeOcean()
{
    // Create a render click to contain the ocean, and add it to the main render step.
    if (!m_pkTritonRenderClick) {
        m_pkTritonRenderClick = NiNew NiTritonRenderClick(m_pkOcean, m_pkEnvironment, m_spCamera);

        NIASSERT(m_spFrame);
        NiDefaultClickRenderStep* pkMainRenderStep = NiDynamicCast(NiDefaultClickRenderStep,
                m_spFrame->GetRenderStepByName(m_kMainRenderStepName));
        NIASSERT(pkMainRenderStep);

        pkMainRenderStep->AppendRenderClick(m_pkTritonRenderClick);
    }
}

//--------------------------------------------------------------------------------------------------
NiApplication* NiApplication::Create()
{
    return NiNew MeshCreation;
}

//--------------------------------------------------------------------------------------------------
MeshCreation::MeshCreation()
    :
    NiSample("Triton Demo",
             DEFAULT_WIDTH,
             DEFAULT_HEIGHT, true),
    m_bUpdate(true)
{
    m_fMinFramePeriod = 1.0f / 500.0f;

    // Set some camera vars (We assume Z is up)
    m_kNavUpAxis = NiPoint3(0.0f, 0.0f, 1.0f);
    m_fNavDefaultScale = 55.0f;

    m_pkTritonRenderClick = 0;
}

//--------------------------------------------------------------------------------------------------
MeshCreation::~MeshCreation()
{
    // We do not own this the particle system does
    m_pkSky = NULL;

    NiDelete m_pkHeightField;
    m_pkHeightField = NULL;

    delete m_pkOcean;
    delete m_pkEnvironment;
    delete m_pkResourceLoader;
}

//--------------------------------------------------------------------------------------------------
bool MeshCreation::CreateRenderer()
{
    // Status strings
    NILOG("Create renderer ...");

#ifdef _PS3
    m_kPS3GLInitParameters.uiHostMemorySize = 128 << 20;
#endif

    if (!NiApplication::CreateRenderer())
        return false;

    m_spRenderer->SetBackgroundColor(NiColor(0.25f, 0.25f, 0.25f));

    NiCamera kCamera;
    m_spRenderer->GetCameraData(kCamera);
    NiFrustum kFrustum = kCamera.GetViewFrustum();
    kFrustum.m_fFar = 100000.0f;
    kCamera.SetViewFrustum(kFrustum);
    m_spRenderer->SetCameraData(&kCamera);

    // Status strings
    NILOG("Finished\n");

    return true;
}

//--------------------------------------------------------------------------------------------------
bool MeshCreation::CreateScene()
{
    // The following code enables CPU read access for any NiDataStream
    // that are created in the process of loading.
    EE_ASSERT(NiDataStream::GetFactory());
    NiDataStream::GetFactory()->SetCallback(
        NiDataStreamFactory::ForceCPUReadAccessCallback);

    // Create the scene
    m_spScene = NiNew NiNode;

    const char* pcNifFilename = "MeshCreation.nif";

    // Status strings
    NILOG("Loading \"MeshCreation.nif\"...");

    // Load the scene
    NiStream kStream;
    if (!kStream.Load(ConvertMediaFilename(pcNifFilename))) {
        NiOutputDebugString(
            "Error:  Could not find the \"MeshCreation.nif\".\n");
        return false;
    }

    // Status strings
    NILOG("Finished\n");

    // Load all objects from the stream into the scene.
    for (NiUInt32 ui = 0; ui < kStream.GetObjectCount(); ui++) {
        NiObject* pkObj = (NiObject*)kStream.GetObjectAt(ui);
        if (NiIsKindOf(NiCamera, pkObj)) {
            m_spCamera = (NiCamera*)pkObj;
        } else if (NiIsKindOf(NiAVObject, pkObj)) {
            m_spScene->AttachChild((NiAVObject*)pkObj);
        }
    }

    // Create the wire frame property
    m_pkWireProp = NiNew NiWireframeProperty();
    m_pkWireProp->SetWireframe(false);

    // Get the scene root
    NiNode* pkRoot = NiDynamicCast(NiNode,
                                   m_spScene->GetObjectByName("Scene Root"));
    if (!pkRoot) {
        NILOG("Could not find \"Scene Root\" in scene.\n");
        return false;
    }

    // Status strings
    NILOG("Removing temporary meshes and extracting materials...");

    // Remove the temp terrain plane and extract the terrain material
    NiAVObjectPtr spkSceneTerrainObject = NULL;
    NiMeshPtr spkSceneTerrain = NULL;
    NiNodePtr spkSceneTerrainParentNode = NULL;
    spkSceneTerrainObject = m_spScene->GetObjectByName(
                                "Initial Terrain Material");
    if (spkSceneTerrainObject) {
        // Check to see if we are parent node
        spkSceneTerrainParentNode = NiDynamicCast(NiNode,
                                    spkSceneTerrainObject);
        if (spkSceneTerrainParentNode &&
                spkSceneTerrainParentNode->GetChildCount() == 1) {
            spkSceneTerrain =
                NiDynamicCast(NiMesh, spkSceneTerrainParentNode->GetAt(0));

            spkSceneTerrainParentNode->DetachChild(spkSceneTerrain);
            spkSceneTerrainParentNode->GetParent()->DetachChild(
                spkSceneTerrainParentNode);

            // Make sure no scale is applied to the sky
            NiTransform kIdentity;
            kIdentity.MakeIdentity();
            spkSceneTerrainParentNode->SetLocalTransform(kIdentity);
        } else {
            spkSceneTerrain = NiDynamicCast(NiMesh, spkSceneTerrainObject);
            if (spkSceneTerrain) {
                spkSceneTerrain->GetParent()->DetachChild(spkSceneTerrain);
                spkSceneTerrain->RemoveAllStreamRefs();
            }
        }
    }

    // Remove the temp sky plane and extract the sky material
    NiAVObjectPtr spkSceneSkyObject = NULL;
    NiMeshPtr spkSceneSky = NULL;
    NiNodePtr spkSceneSkyParentNode = NULL;
    spkSceneSkyObject = m_spScene->GetObjectByName("Initial Sky Material");
    if (spkSceneSkyObject) {
        // Check to see if we are parent node
        spkSceneSkyParentNode = NiDynamicCast(NiNode, spkSceneSkyObject);
        if (spkSceneSkyParentNode &&
                spkSceneSkyParentNode->GetChildCount() == 1) {
            spkSceneSky =
                NiDynamicCast(NiMesh, spkSceneSkyParentNode->GetAt(0));

            spkSceneSkyParentNode->DetachChild(spkSceneSky);
            spkSceneSkyParentNode->GetParent()->DetachChild(
                spkSceneSkyParentNode);

            // Make sure no scale is applied to the sky
            NiTransform kIdentity;
            kIdentity.MakeIdentity();
            spkSceneSkyParentNode->SetLocalTransform(kIdentity);
        } else {
            spkSceneSky = NiDynamicCast(NiMesh, spkSceneSkyObject);
            if (spkSceneSky) {
                spkSceneSky->GetParent()->DetachChild(spkSceneSky);
                spkSceneSky->RemoveAllStreamRefs();
            }
        }
    }

    // Status strings
    NILOG("Finished\n");

    // Compact all nodes so we remove the empty spaces in the scene graph.
    m_spScene->CompactChildArray(true);

    // Status strings
    NILOG("Initializing the height field...");

    // Initialize the height field class
    m_pkHeightField = NiNew HeightField(ConvertMediaFilename("Terrain.bmp"));
    if (m_pkHeightField && !m_pkHeightField->IsValid()) {
        NILOG("Invalid height field.\n");
        return false;
    }

    // Status strings
    NILOG("Finished\n");

    // Status strings
    NILOG("Creating the terrain mesh...");

    // Create the terrain mesh
    NiMeshPtr spkTerrain = CreateTerrainMesh(spkSceneTerrain);
    if (!spkTerrain) {
        NILOG("Could not create the terrain mesh.\n");
        return false;
    }

    // Attach wire frame to the terrain
    spkTerrain->AttachProperty(m_pkWireProp);

    // Status strings
    NILOG("Finished\n");

    // We no longer need the height field so we can delete it
    NiDelete m_pkHeightField;
    m_pkHeightField = NULL;

    // Status strings
    NILOG("Creating the sky dome mesh ...");

    // Initialize the water particle system
    NiMeshPtr spSky =
        CreateSkyMesh(spkSceneSky);
    if (!spSky) {
        NILOG("Could not create the sky mesh.\n");
        return false;
    }

    // Status strings
    NILOG("Finished\n");

    // Add everything to the scene graph in the order in which we want
    // them to be.

    // Status strings
    NILOG("Attaching the sky dome mesh ...");

    // Draw the sky first.
    if (spkSceneSkyParentNode) {
        // If we have a parent attach the child
        spkSceneSkyParentNode->AttachChild(spSky);
        pkRoot->AttachChild(spkSceneSkyParentNode);
    } else {
        pkRoot->AttachChild(spSky);
    }
    m_pkSky = spSky;

    // Status strings
    NILOG("Finished\n");

    // Status strings
    NILOG("Attaching the terrain mesh ...");

    // Next goes the terrain.
    if (spkSceneTerrainParentNode) {
        // If we have a parent attach the child
        spkSceneTerrainParentNode->AttachChild(spkTerrain);
        pkRoot->AttachChild(spkSceneTerrainParentNode);
    } else {
        pkRoot->AttachChild(spkTerrain);
    }

    // Status strings
    NILOG("Finished\n");

    // Status strings
    NILOG("Creating water\n");

    if (!CreateOcean()) {
        NILOG("Failed to create ocean.\n");
        return false;
    }

    NILOG("Finished\n");

    return true;
}

//--------------------------------------------------------------------------------------------------
NiMeshPtr MeshCreation::CreateTerrainMesh(NiMesh* spkSceneTerrain)
{
    // Compute the range
    float fRangeX = (MAX_TERRAIN_SIZE_X - MIN_TERRAIN_SIZE_X);
    float fRangeY = (MAX_TERRAIN_SIZE_Y - MIN_TERRAIN_SIZE_Y);
    float fRangeZ = (MAX_TERRAIN_SIZE_Z - MIN_TERRAIN_SIZE_Z);

    // Compute the total number of vertices and tris that will exist when we
    // are done with subdivision.
    NiUInt32 uiNumVertsX = (NiUInt32)(fRangeX / DESIRED_TERRAIN_QUAD_SIZE);
    NiUInt32 uiNumVertsY = (NiUInt32)(fRangeY / DESIRED_TERRAIN_QUAD_SIZE);

    // Initialize the vert size
    NiUInt32 uiTotalVerts = (uiNumVertsX + 1) * (uiNumVertsY + 1);
    while (uiTotalVerts > 0xffff) {
        uiNumVertsX--;
        uiNumVertsY--;
        uiTotalVerts = (uiNumVertsX + 1) * (uiNumVertsY + 1);
    }

    // Initialize the number of triangles
    NiUInt32 uiTotalTris = (uiNumVertsX * 2) * uiNumVertsY;

    // Create the mesh using the mesh builder
    NiMeshPtr spTerrain = NiNew NiMesh();
    spTerrain->SetName("Terrain");
    spTerrain->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);

    // Create the index stream
    NiDataStreamElementLock kIndexLock = spTerrain->AddStreamGetLock(
            NiCommonSemantics::INDEX(), 0,
            NiDataStreamElement::F_UINT16_1,
            3 * uiTotalTris,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX_INDEX);

    EE_ASSERT(kIndexLock.count() == (3 * uiTotalTris));

    NiTStridedRandomAccessIterator<NiUInt16> kIndicesIter =
        kIndexLock.begin<NiUInt16>();

    // Create the position stream
    NiDataStreamElementLock kPositionLock = spTerrain->AddStreamGetLock(
            NiCommonSemantics::POSITION(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            uiTotalVerts,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kPositionLock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint3> kPointsIter =
        kPositionLock.begin<NiPoint3>();

    // Create the texture uvs stream
    NiDataStreamElementLock kTexCoord0Lock = spTerrain->AddStreamGetLock(
                NiCommonSemantics::TEXCOORD(), 0,
                NiDataStreamElement::F_FLOAT32_2,
                uiTotalVerts,
                NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_STATIC,
                NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kTexCoord0Lock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint2> kTexCoords0Iter =
        kTexCoord0Lock.begin<NiPoint2>();

    // Create the texture uvs stream
    NiDataStreamElementLock kTexCoord1Lock = spTerrain->AddStreamGetLock(
                NiCommonSemantics::TEXCOORD(), 1,
                NiDataStreamElement::F_FLOAT32_2,
                uiTotalVerts,
                NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_STATIC,
                NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kTexCoord1Lock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint2> kTexCoords1Iter =
        kTexCoord1Lock.begin<NiPoint2>();

    // Create the normal stream
    NiDataStreamElementLock kNormalLock = spTerrain->AddStreamGetLock(
            NiCommonSemantics::NORMAL(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            uiTotalVerts,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kNormalLock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint3> kNormalsIter =
        kNormalLock.begin<NiPoint3>();

    // Create the bi-normal stream
    NiDataStreamElementLock kBiNormalLock = spTerrain->AddStreamGetLock(
            NiCommonSemantics::BINORMAL(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            uiTotalVerts,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kBiNormalLock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint3> kBiNormalsIter =
        kBiNormalLock.begin<NiPoint3>();

    // Create the tangent stream
    NiDataStreamElementLock kTangentLock = spTerrain->AddStreamGetLock(
            NiCommonSemantics::TANGENT(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            uiTotalVerts,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kTangentLock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint3> kTangentsIter =
        kTangentLock.begin<NiPoint3>();

    // Get the increment values
    float fIncrementX = fRangeX / (float)uiNumVertsX;
    float fIncrementY = fRangeY / (float)uiNumVertsY;

    // Get the increment values for the texture coordinates
    float fIncrementU0 = 1.0f / (float)uiNumVertsX;
    float fIncrementV0 = 1.0f / (float)uiNumVertsY;
    float fIncrementU1 = UV_SCALE_FACTOR / (float)uiNumVertsX;
    float fIncrementV1 = UV_SCALE_FACTOR / (float)uiNumVertsY;

    // Set the positions
    NiUInt32 uiNumVerts = 0;
    NiPoint3 kPosition(MIN_TERRAIN_SIZE_X, MIN_TERRAIN_SIZE_Y, 0.0f);
    NiPoint2 kUvCoord0(0.0f, 0.0f);
    NiPoint2 kUvCoord1(0.0f, 0.0f);

    // Iterate through all the vertices starting from the lower left moving to
    // the upper right.
    for (NiUInt32 uiY = 0; uiY <= uiNumVertsY; uiY++) {
        // Set the x position back to default
        kPosition.x = MIN_TERRAIN_SIZE_X;
        kUvCoord0.x = 0.0f;
        kUvCoord1.x = 0.0f;

        for (NiUInt32 uiX = 0; uiX <= uiNumVertsX; uiX++) {
            // Get the height from the height field.
            kPosition.z =
                m_pkHeightField->GetHeight(kPosition.x, kPosition.y);

            // Set the vertex position
            kPointsIter[uiNumVerts] = kPosition;
            kTexCoords0Iter[uiNumVerts] = kUvCoord0;
            kTexCoords1Iter[uiNumVerts] = kUvCoord1;

            kNormalsIter[uiNumVerts] = NiPoint3::ZERO;
            kBiNormalsIter[uiNumVerts] = NiPoint3::ZERO;
            kTangentsIter[uiNumVerts] = NiPoint3::ZERO;

            // Increment to the next vertex
            uiNumVerts++;

            // Increment the position
            kPosition.x += fIncrementX;
            kUvCoord0.x += fIncrementU0;
            kUvCoord1.x += fIncrementU1;
        }

        // Increment the position
        kPosition.y += fIncrementY;
        kUvCoord0.y += fIncrementV0;
        kUvCoord1.y += fIncrementV1;
    }

    // Set the indices and the normals, tangents, binormals.
    int * pnCount = NiAlloc(int, uiTotalVerts);
    memset(pnCount, 0, sizeof(int) * uiTotalVerts);

    // Run through all vertices and initialize the normals and the triangle
    // indices.
    NiUInt32 uiIndex = 0;
    NiPoint3 kNormal;
    for (NiUInt32 uiY = 0; uiY < uiNumVertsY; uiY++) {
        for (NiUInt32 uiX = 0; uiX < uiNumVertsX; uiX++) {
            // Set the index buffer
            NiUInt32 uiIndex0 =
                (uiY * (uiNumVertsX + 1)) + uiX;
            NiUInt32 uiIndex1 =
                ((uiY + 1) * (uiNumVertsX + 1)) + uiX;
            NiUInt32 uiIndex2 =
                ((uiY + 1) * (uiNumVertsX + 1)) + (uiX + 1);
            NiUInt32 uiIndex3 =
                (uiY * (uiNumVertsX + 1)) + (uiX + 1);

            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex0;
            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex2;
            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex1;

            // Calculate the normals
            kNormal = NiGeometricUtils::GetTriNormal(kPointsIter[uiIndex0],
                      kPointsIter[uiIndex2], kPointsIter[uiIndex1]);

            // Add the normal so we can take the average normal for this
            // vertex.
            if (!pnCount[uiIndex0])
                kNormalsIter[uiIndex0] = kNormal;
            else
                kNormalsIter[uiIndex0] += kNormal;
            pnCount[uiIndex0]++;

            if (!pnCount[uiIndex2])
                kNormalsIter[uiIndex2] = kNormal;
            else
                kNormalsIter[uiIndex2] += kNormal;
            pnCount[uiIndex2]++;

            if (!pnCount[uiIndex1])
                kNormalsIter[uiIndex1] = kNormal;
            else
                kNormalsIter[uiIndex1] += kNormal;
            pnCount[uiIndex1]++;

            // Do it for the next triangle
            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex0;
            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex3;
            kIndicesIter[uiIndex++] = (NiUInt16)uiIndex2;

            // Calculate the normals
            kNormal = NiGeometricUtils::GetTriNormal(kPointsIter[uiIndex0],
                      kPointsIter[uiIndex2], kPointsIter[uiIndex1]);

            // Add the normal so we can take the average normal for this
            // vertex.
            if (!pnCount[uiIndex0])
                kNormalsIter[uiIndex0] = kNormal;
            else
                kNormalsIter[uiIndex0] += kNormal;
            pnCount[uiIndex0]++;

            if (!pnCount[uiIndex3])
                kNormalsIter[uiIndex3] = kNormal;
            else
                kNormalsIter[uiIndex3] += kNormal;
            pnCount[uiIndex3]++;

            if (!pnCount[uiIndex2])
                kNormalsIter[uiIndex2] = kNormal;
            else
                kNormalsIter[uiIndex2] += kNormal;
            pnCount[uiIndex2]++;
        }
    }

    // Calculate the average normal per vertex.
    for (NiUInt32 ui = 0; ui < uiTotalVerts; ui++) {
        kNormalsIter[ui] /= (float)pnCount[ui];
    }

    // Reset the count
    memset(pnCount, 0, sizeof(int) * uiTotalVerts);

    // Calculate the tangents and binormals
    NiPoint3 kTangent;
    NiPoint3 kBiNormal;
    for (NiUInt32 uiY = 0; uiY < uiNumVertsY; uiY++) {
        for (NiUInt32 uiX = 0; uiX < uiNumVertsX; uiX++) {
            // Get the surrounding vertices.
            NiUInt32 uiIndex0 =
                (uiY * (uiNumVertsX + 1)) + uiX;
            NiUInt32 uiIndex1 =
                ((uiY + 1) * (uiNumVertsX + 1)) + uiX;
            NiUInt32 uiIndex2 =
                ((uiY + 1) * (uiNumVertsX + 1)) + (uiX + 1);
            NiUInt32 uiIndex3 =
                (uiY * (uiNumVertsX + 1)) + (uiX + 1);

            // Find for the first primitive
            NiMeshUtilities::FindBT(kPointsIter[uiIndex0],
                                    kPointsIter[uiIndex2], kPointsIter[uiIndex1],
                                    kTexCoords0Iter[uiIndex0], kTexCoords0Iter[uiIndex2],
                                    kTexCoords0Iter[uiIndex1], kBiNormal, kTangent);

            // Increment the binormal and tangent to get the average binormal
            // and tangent for the first face.
            if (!pnCount[uiIndex0]) {
                kBiNormalsIter[uiIndex0] = kBiNormal;
                kTangentsIter[uiIndex0] = kTangent;
            } else {
                kBiNormalsIter[uiIndex0] += kBiNormal;
                kTangentsIter[uiIndex0] += kTangent;
            }
            pnCount[uiIndex0]++;

            if (!pnCount[uiIndex2]) {
                kBiNormalsIter[uiIndex2] = kBiNormal;
                kTangentsIter[uiIndex2] = kTangent;
            } else {
                kBiNormalsIter[uiIndex2] += kBiNormal;
                kTangentsIter[uiIndex2] += kTangent;
            }
            pnCount[uiIndex2]++;

            if (!pnCount[uiIndex1]) {
                kBiNormalsIter[uiIndex1] = kBiNormal;
                kTangentsIter[uiIndex1] = kTangent;
            } else {
                kBiNormalsIter[uiIndex1] += kBiNormal;
                kTangentsIter[uiIndex1] += kTangent;
            }
            pnCount[uiIndex1]++;

            // Find for the other primitive
            NiMeshUtilities::FindBT(kPointsIter[uiIndex0],
                                    kPointsIter[uiIndex3], kPointsIter[uiIndex2],
                                    kTexCoords0Iter[uiIndex0], kTexCoords0Iter[uiIndex3],
                                    kTexCoords0Iter[uiIndex2], kBiNormal, kTangent);

            // Increment the binormal and tangent to get the average binormal
            // and tangent for the second face.
            if (!pnCount[uiIndex0]) {
                kBiNormalsIter[uiIndex0] = kBiNormal;
                kTangentsIter[uiIndex0] = kTangent;
            } else {
                kBiNormalsIter[uiIndex0] += kBiNormal;
                kTangentsIter[uiIndex0] += kTangent;
            }
            pnCount[uiIndex0]++;

            if (!pnCount[uiIndex3]) {
                kBiNormalsIter[uiIndex3] = kBiNormal;
                kTangentsIter[uiIndex3] = kTangent;
            } else {
                kBiNormalsIter[uiIndex3] += kBiNormal;
                kTangentsIter[uiIndex3] += kTangent;
            }
            pnCount[uiIndex3]++;

            if (!pnCount[uiIndex2]) {
                kBiNormalsIter[uiIndex2] = kBiNormal;
                kTangentsIter[uiIndex2] = kTangent;
            } else {
                kBiNormalsIter[uiIndex2] += kBiNormal;
                kTangentsIter[uiIndex2] += kTangent;
            }
            pnCount[uiIndex2]++;
        }
    }

    // Calculate the average binormal and tangent.
    for (NiUInt32 ui = 0; ui < uiTotalVerts; ui++) {
        kBiNormalsIter[ui] /= (float)pnCount[ui];
        kTangentsIter[ui] /= (float)pnCount[ui];
    }

    // Free up the count
    NiFree(pnCount);
    pnCount = NULL;

    // Unlock the data
    kTangentLock.Unlock();
    kBiNormalLock.Unlock();
    kNormalLock.Unlock();
    kTexCoord0Lock.Unlock();
    kTexCoord1Lock.Unlock();
    kPositionLock.Unlock();
    kIndexLock.Unlock();

    // Set the bound
    NiBound kTerrainBound;
    NiPoint3 kCenter(
        MIN_TERRAIN_SIZE_X + (0.5f * fRangeX),
        MIN_TERRAIN_SIZE_Y + (0.5f * fRangeY),
        MIN_TERRAIN_SIZE_Z + (0.5f * fRangeZ));
    float fRadius = sqrtf(fRangeX * fRangeX + fRangeY * fRangeY +
                          fRangeZ * fRangeZ);
    kTerrainBound.SetCenterAndRadius(kCenter, fRadius);
    spTerrain->SetModelBound(kTerrainBound);

    // Add the property and effect state to the terrain
    spTerrain->SetPropertyState(spkSceneTerrain->GetPropertyState());
    spTerrain->SetEffectState(spkSceneTerrain->GetEffectState());

    // Add all the properties to this instance from the given terrain plane
    NiTListIterator kIter = spkSceneTerrain->GetPropertyList().GetHeadPos();
    while (kIter != NULL) {
        NiProperty* pkProp = spkSceneTerrain->GetPropertyList().GetNext(kIter);
        if (pkProp)
            spTerrain->AttachProperty(pkProp);
    }

    // Add all the materials to this instance from the given terrain plane
    for (NiUInt32 ui = 0; ui < spkSceneTerrain->GetMaterialCount(); ui++) {
        spTerrain->ApplyAndSetActiveMaterial(
            spkSceneTerrain->GetMaterialInstance(ui)->GetMaterial(),
            spkSceneTerrain->GetMaterialInstance(ui)->GetMaterialExtraData());
    }

    return spTerrain;
}

//--------------------------------------------------------------------------------------------------
NiMeshPtr MeshCreation::CreateSkyMesh(NiMesh* spkSceneSky)
{
    // create a pyramid
    NiPoint3 pPyramidVerts[5] = {
        NiPoint3(-1.0f, -1.0f, 0.0f),
        NiPoint3(-1.0f, 1.0f, 0.0f),
        NiPoint3(1.0f, 1.0f, 0.0f),
        NiPoint3(1.0f, -1.0f, 0.0f),
        NiPoint3(0.0f, 0.0f, 1.0f)
    };

    NiUInt16 pPyramidPolys[4 * 3] = {
        1, 4, 0,
        2, 4, 1,
        3, 4, 2,
        0, 4, 3,
    };

    NiUInt32 uiNumVerts = 5;
    NiUInt32 uiNumTris = 4;
    NiUInt32 uiTotalVerts = 5;
    NiUInt32 uiTotalTris = 4;

    // Compute the total number of vertices and triangles that will exist when
    // we are done with subdivision.
    NiUInt32 uiNumDivisions = 4;
    NiUInt32 i;
    for (i = 0; i < uiNumDivisions; i++) {
        uiTotalVerts += 3*uiTotalTris;
        uiTotalTris *= 4;
    }

    // Create the mesh using the mesh builder
    NiMeshPtr spSky = NiNew NiMesh();
    spSky->SetName("Sky");
    spSky->SetPrimitiveType(NiPrimitiveType::PRIMITIVE_TRIANGLES);

    // Create the index stream
    NiDataStreamElementLock kIndexLock = spSky->AddStreamGetLock(
            NiCommonSemantics::INDEX(), 0,
            NiDataStreamElement::F_UINT16_1,
            3 * uiTotalTris,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX_INDEX);

    EE_ASSERT(kIndexLock.count() == (3 * uiTotalTris));

    NiTStridedRandomAccessIterator<NiUInt16> kIndicesIter =
        kIndexLock.begin<NiUInt16>();

    // Create the position stream
    NiDataStreamElementLock kPositionLock = spSky->AddStreamGetLock(
            NiCommonSemantics::POSITION(), 0,
            NiDataStreamElement::F_FLOAT32_3,
            uiTotalVerts,
            NiDataStream::ACCESS_GPU_READ |
            NiDataStream::ACCESS_CPU_WRITE_STATIC,
            NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kPositionLock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint3> kPointsIter =
        kPositionLock.begin<NiPoint3>();

    // Create the texture uvs stream
    NiDataStreamElementLock kTexCoord0Lock = spSky->AddStreamGetLock(
                NiCommonSemantics::TEXCOORD(), 0,
                NiDataStreamElement::F_FLOAT32_2,
                uiTotalVerts,
                NiDataStream::ACCESS_GPU_READ |
                NiDataStream::ACCESS_CPU_WRITE_STATIC,
                NiDataStream::USAGE_VERTEX);

    EE_ASSERT(kTexCoord0Lock.count() == uiTotalVerts);

    NiTStridedRandomAccessIterator<NiPoint2> kTexCoords0Iter =
        kTexCoord0Lock.begin<NiPoint2>();

    // starting with an icosahedron, normalize the vertices
    for (i = 0; i < uiNumVerts; i++) {
        pPyramidVerts[i].Unitize();
        kPointsIter[i] = pPyramidVerts[i];
    }

    // copy the icosahedron connectivity as a starting point
    NiUInt32 uiSize = uiNumTris * 3;
    NiUInt16* pIndices = NiAlloc(NiUInt16, uiTotalTris * 3);
    NiUInt16* pTempIndices = NiAlloc(NiUInt16, uiTotalTris * 3);
    for (i = 0; i < uiSize; i+=3) {
        pIndices[i] = pPyramidPolys[i];
        pIndices[i + 1] = pPyramidPolys[i + 1];
        pIndices[i + 2] = pPyramidPolys[i + 2];
    }

    // subdivide the icosahedron
    for (i = 0; i < uiNumDivisions; i++) {
        // Store traversal pointers for the source and destination
        // connectivity arrays.
        NiUInt16* pSrc = pIndices;
        NiUInt16* pDest = pTempIndices;

        // for each triangle
        for (NiUInt32 t = 0; t < uiNumTris; t++) {
            // get the indices to the triangle's vertices
            NiUInt16 iI1 = *pSrc++,
                     iI2 = *pSrc++,
                     iI3 = *pSrc++;

            // Compute the three new vertices as triangle edge midpoints and
            // normalize the points back onto the sphere.
            kPointsIter[uiNumVerts] = kPointsIter[iI1] + kPointsIter[iI2];
            kPointsIter[uiNumVerts].Unitize();
            kPointsIter[uiNumVerts+1] = kPointsIter[iI2] + kPointsIter[iI3];
            kPointsIter[uiNumVerts+1].Unitize();
            kPointsIter[uiNumVerts+2] = kPointsIter[iI3] + kPointsIter[iI1];
            kPointsIter[uiNumVerts+2].Unitize();

            // create 4 new triangles to tessellate the old triangle
            *(pDest++) = iI1;
            *(pDest++) = (NiUInt16)uiNumVerts;
            *(pDest++) = (NiUInt16)(uiNumVerts+2);

            *(pDest++) = iI2;
            *(pDest++) = (NiUInt16)(uiNumVerts+1);
            *(pDest++) = (NiUInt16)uiNumVerts;

            *(pDest++) = iI3;
            *(pDest++) = (NiUInt16)(uiNumVerts+2);
            *(pDest++) = (NiUInt16)(uiNumVerts+1);

            *(pDest++) = (NiUInt16)uiNumVerts;
            *(pDest++) = (NiUInt16)(uiNumVerts+1);
            *(pDest++) = (NiUInt16)(uiNumVerts+2);

            // update the number of vertices
            uiNumVerts+=3;
        }

        // swap the two temporary connectivity arrays
        NiUInt16* pTemp = pIndices;
        pIndices = pTempIndices;
        pTempIndices = pTemp;

        // update the number of triangles
        uiNumTris *= 4;
    }

    // Copy it into the mesh
    uiSize = uiNumTris * 3;
    for (i = 0; i < uiSize; i++) {
        kIndicesIter[i] = pIndices[i];
    }

    NiFree(pIndices);
    pIndices = NULL;
    NiFree(pTempIndices);
    pTempIndices = NULL;

    // Compute the range to compute the scale
    float fRangeX = (MAX_TERRAIN_SIZE_X - MIN_TERRAIN_SIZE_X);
    float fRangeY = (MAX_TERRAIN_SIZE_Y - MIN_TERRAIN_SIZE_Y);

    // Get the 2D radius ignoring the height of the terrain.
    const float fSqrtOfTwo = NiSqrt(2.0f);
    NiPoint3 kCenter(0,0,0);
    float fRadius =
        NiSqrt(fRangeX * fRangeX + fRangeY * fRangeY) / fSqrtOfTwo;

    // Set the texture coordinate sets with a cylinder projection. Scale the
    // vertices by the radius.
    for (i = 0; i < uiNumVerts; i++) {
        const NiPoint3& kPosition = kPointsIter[i];
        NiPoint2 kTexCoord (
            (NiACos(kPosition.x) / (fSqrtOfTwo * NI_TWO_PI)) * 2.0f,
            1.0f - kPosition.z);
        kTexCoords0Iter[i] = kTexCoord;
        kPointsIter[i] *= fRadius;

        // Drop it below the terrain level.
        kPointsIter[i].z += (MIN_TERRAIN_SIZE_Z * 2.0f);
    }

    // Unlock the data
    kTexCoord0Lock.Unlock();
    kPositionLock.Unlock();
    kIndexLock.Unlock();

    // Set the bound
    NiBound kSkyBound;
    kSkyBound.SetCenterAndRadius(kCenter, fRadius);
    spSky->SetModelBound(kSkyBound);

    if (spkSceneSky) {
        // Add the property and effect state to the terrain
        spSky->SetPropertyState(spkSceneSky->GetPropertyState());
        spSky->SetEffectState(spkSceneSky->GetEffectState());

        // Add all the properties to this instance from the given terrain plane
        NiTListIterator kIter = spkSceneSky->GetPropertyList().GetHeadPos();
        while (kIter != NULL) {
            NiProperty* pkProp = spkSceneSky->GetPropertyList().GetNext(kIter);
            if (pkProp)
                spSky->AttachProperty(pkProp);
        }

        // Add all the materials to this instance from the given terrain plane
        for (NiUInt32 ui = 0; ui < spkSceneSky->GetMaterialCount(); ui++) {
            spSky->ApplyAndSetActiveMaterial(
                spkSceneSky->GetMaterialInstance(ui)->GetMaterial(),
                spkSceneSky->GetMaterialInstance(ui)->GetMaterialExtraData());
        }
    }

    NiZBufferProperty* pkZBufferProp =
        (NiZBufferProperty*)spSky->GetProperty(NiZBufferProperty::GetType());
    if (!pkZBufferProp) {
        pkZBufferProp = NiNew NiZBufferProperty();
        spSky->AttachProperty(pkZBufferProp);
    }
    pkZBufferProp->SetZBufferTest(true);
    pkZBufferProp->SetZBufferWrite(false);

    // Attach wire frame to the sky
    spSky->AttachProperty(m_pkWireProp);

    return spSky;
}

//--------------------------------------------------------------------------------------------------
void MeshCreation::Terminate()
{
    NiSample::Terminate();
}

//--------------------------------------------------------------------------------------------------
void MeshCreation::UpdateFrame()
{
    InitializeOcean();

    if (m_bUpdate) {
        m_spScene->Update(m_fCurrentTime);
    }

    // Make the sky move with the camera
    if (m_pkSky)
        m_pkSky->SetWorldTranslate(m_spCamera->GetWorldTranslate());

    NiSample::UpdateFrame();
}
