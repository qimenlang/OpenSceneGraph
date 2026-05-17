// TritonTestDX11.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "../SamplesCommon/Licenses.h"

#undef NDEBUG
#include <cassert>

#include "DirectX11MultiThreadedSample.h"

#include <D3Dcompiler.h>


const enum DrawStrategy
{
    DRAWVIEWSUSINGIMMEDIATECONTEXT
    , DRAWVIEWSUSINGDEFERREDCONTEXTS
    , DRAWVIEWSUSINGDEFERREDCONTEXTSINTHREADS
};

const DrawStrategy drawStrategy = DRAWVIEWSUSINGDEFERREDCONTEXTSINTHREADS;

// for threading
#include <process.h>

using namespace Triton;

//#define GEOCENTRIC
#define WATER_MODEL Triton::TESSENDORF

#define TEST_WAKES 1
#define TEST_ROTORWASH 1

Triton::Environment *environment = 0;
Triton::Ocean *ocean = 0;
Triton::ResourceLoader *resourceLoader = 0;

double yaw = 0;

#if TEST_WAKES

struct Ship {
    Ship(Ocean* _ocean, const Triton::WakeGeneratorParameters& params, const Vector3& _direction, double _velocity)
        : ocean(_ocean)
        , shipPosition(Vector3(0, 0, 0))
        , shipPositionLast(Vector3(0, 0, 0))
        , direction(_direction)
        , velocity(_velocity) {
        wakeGenerator = new Triton::WakeGenerator(ocean, params);
        const_cast<Triton::Vector3&>(direction).Normalize();
    }
    Ship::~Ship() {
        delete wakeGenerator;
    }
    void update(double nowS, double delta) {
#ifdef GEOCENTRIC
        shipPosition.y = -6378137;
#else
        shipPosition.y = 0;
#endif

        shipPosition.z = delta * velocity;

        shipPositionLast.x = shipPosition.x;
        shipPositionLast.z = shipPosition.z;

        wakeGenerator->Update(shipPosition, direction, velocity, nowS);
    }

    Triton::Vector3 shipPosition;
    Triton::Vector3 shipPositionLast;
    Ocean* ocean;
    Triton::WakeGenerator* wakeGenerator;

    const Triton::Vector3 direction;
    const double velocity;
};
Ship* ship0 = 0;
#endif

#if TEST_ROTORWASH
Triton::RotorWash *rotorWash = 0;
Triton::Vector3 rotorPosition(0, 10, -50);
Triton::Vector3 rotorDirection(0, -1, 0);
#endif

Triton::Vector3 cameraPosition(0, 30, -150);

bool testGodRays = false;

static SkyBox *skyBox = 0;

// dx11 stuff
static ID3D11Device *device = 0;
static IDXGISwapChain *swapChain = 0;
static  ID3D11DepthStencilView *depthStencilView = 0;
static ID3D11RenderTargetView *renderTargetView = 0;

// win 32/64 stuff
#define MAX_LOADSTRING 100
#define WINWIDTH 800
#define WINHEIGHT 600

// Global Variables:
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


// multi-view stuff
static const int numViews = 2;
static const int view_width = 256;
static const int view_height = 256;

ID3D11Texture2D *rtt_textures[numViews];
ID3D11RenderTargetView* rtt_views[numViews];
ID3D11ShaderResourceView* rtt_srvs[numViews];

Camera* cameras[2];
Camera* mainCamera = 0;

ID3D11DeviceContext* contexts[numViews];
ID3D11CommandList*   command_lists[numViews] = {NULL};

ID3D11DeviceContext* context_main = 0;
ID3D11CommandList* command_list_main = 0;

static ID3D11Texture2D *depthStencilBuffer;

ID3D11DeviceContext* immediateContext = 0;

#define SAFE_RELEASE(x) if (x) {x->Release(); x = 0;}

HANDLE g_hPerSceneRenderDeferredThread[numViews];
int g_iPerSceneThreadInstanceData[numViews];
HANDLE g_hBeginPerSceneRenderDeferredEvent[numViews];
HANDLE g_hEndPerSceneRenderDeferredEvent[numViews];

HANDLE g_hPerSceneRenderDeferredThreadMain;
int g_iPerSceneThreadInstanceDataMain;
HANDLE g_hBeginPerSceneRenderDeferredEventMain;
HANDLE g_hEndPerSceneRenderDeferredEventMain;


void CreateRTT(ID3D11Texture2D*& rtt_texture, ID3D11RenderTargetView*& rtt_view, ID3D11ShaderResourceView*& rtt_srv)
{
    rtt_texture = 0;
    D3D11_TEXTURE2D_DESC desc;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Width = (UINT)view_width;
    desc.Height = (UINT)view_height;

    HRESULT hr = device->CreateTexture2D(&desc,0, &rtt_texture);
    assert(hr==S_OK);


    D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
    RTVDesc.Format             = desc.Format;
    RTVDesc.ViewDimension      = D3D11_RTV_DIMENSION_TEXTURE2D;
    RTVDesc.Texture2D.MipSlice = 0;

    hr = device->CreateRenderTargetView(rtt_texture, &RTVDesc, &rtt_view);
    assert(hr==S_OK && rtt_view);

    D3D11_SHADER_RESOURCE_VIEW_DESC SRVdesc;
    ZeroMemory( &SRVdesc, sizeof(SRVdesc) );
    SRVdesc.Format = desc.Format;
    SRVdesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    SRVdesc.Texture2D.MipLevels = desc.MipLevels;
    SRVdesc.Texture2D.MostDetailedMip = 0;
    hr = device->CreateShaderResourceView(rtt_texture, &SRVdesc, &rtt_srv);
    assert(hr==S_OK && rtt_srv);
}

void SetRenderTarget(ID3D11DeviceContext* context, size_t view)
{
    if (view>=numViews) {
        return;
    }

    ID3D11RenderTargetView* rtt_view = rtt_views[view];
    if (rtt_view==0) {
        std::cerr<<"RTT View of Texture is null";
        return;
    }
    static const D3D11_VIEWPORT vp = {0,0,view_width,view_height,0,1};
    context->RSSetViewports(1,&vp);

    context->OMSetRenderTargets(1, &rtt_view, NULL);
}

void SetUpMainView(ID3D11DeviceContext* context)
{
    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = WINWIDTH;
    vp.Height = WINHEIGHT;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    context->RSSetViewports(1, &vp);
    context->OMSetRenderTargets(1, &renderTargetView, depthStencilView);
}

void InitViews()
{
    for(int i = 0; i < numViews; ++i) {
        CreateRTT(rtt_textures[i], rtt_views[i], rtt_srvs[i]);
        Camera* camera = environment->CreateCamera();
        cameras[i] = camera;
    }

    mainCamera = environment->CreateCamera();

    HRESULT hr;
    for(int i = 0; i < numViews; ++i) {
        hr = device->CreateDeferredContext( 0, &contexts[i]);
        assert(hr==S_OK);
    }

    context_main = 0;
    hr = device->CreateDeferredContext( 0, &context_main);
    assert(hr==S_OK);
}

void ReleaseViews()
{
    for(int i = 0; i < numViews; ++i) {
        if (rtt_textures[i]) {
            int k = rtt_textures[i]->Release();
            if (k>0) assert(false);

        }
        if (rtt_views[i]) {
            int k = rtt_views[i]->Release();
            if (k>0) assert(false);
        }
        if (rtt_srvs[i]) {
            int k = rtt_srvs[i]->Release();
            if (k>0) assert(false);
        }
        if (contexts[i]) {
            contexts[i]->ClearState();
            contexts[i]->Flush();
            int k = contexts[i]->Release();
            if (k>0) assert(false);
        }

        CloseHandle(g_hPerSceneRenderDeferredThread[i]);
        CloseHandle(g_hBeginPerSceneRenderDeferredEvent[i]);
        CloseHandle(g_hEndPerSceneRenderDeferredEvent[i]);
    }

    CloseHandle(g_hPerSceneRenderDeferredThreadMain);
    CloseHandle(g_hBeginPerSceneRenderDeferredEventMain);
    CloseHandle(g_hEndPerSceneRenderDeferredEventMain);
    if (context_main) {
        int k = context_main->Release();
        if (k>0) assert(false);
    }
}

struct QuadWindowParams {
    float viewport_width;
    float viewport_height;

    float window_xpos;
    float window_ypos;

    float window_width;
    float window_height;
    float pad[2];
};

struct QuadVertex {
    D3DXVECTOR3 Pos; // Position
    D3DXVECTOR2 Tex; // Texture Coordinate
};

ID3D11Buffer* quadVertexBuffer = 0;
ID3D11Buffer* quadIndexBuffer = 0;
ID3D11Buffer* quadConstantBuffer = 0;
D3D11_INPUT_ELEMENT_DESC quad_layout_desc[] = {
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};

void CreateVertexBufferForQuad(void)
{

    QuadVertex vertices[] = {
        {D3DXVECTOR3(-1,-1, 0), D3DXVECTOR2(0,0)},
        {D3DXVECTOR3(+1,-1, 0), D3DXVECTOR2(1,0)},
        {D3DXVECTOR3(+1,+1, 0), D3DXVECTOR2(1,1)},
        {D3DXVECTOR3(-1,+1, 0), D3DXVECTOR2(0,1)},
    };

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ByteWidth = sizeof( QuadVertex ) * 4;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    desc.StructureByteStride = (UINT)sizeof( QuadVertex );

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = vertices;

    HRESULT hr = device->CreateBuffer(&desc, &InitData, &quadVertexBuffer);
    assert(hr==S_OK);
}

void CreateIndexBufferForQuad(void)
{
    typedef std::vector<short> Indices;
    Indices indices;

    //indices.push_back(0);indices.push_back(1);indices.push_back(2);
    //indices.push_back(0);indices.push_back(2);indices.push_back(3);

    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(1);
    indices.push_back(0);
    indices.push_back(3);
    indices.push_back(2);


    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ByteWidth = sizeof( short ) * 6;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    desc.StructureByteStride = (UINT)sizeof( short );

    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = &indices.front();

    HRESULT hr = device->CreateBuffer(&desc, &InitData, &quadIndexBuffer);
    assert(hr==S_OK);
}

void CreateConstantBufferForQuad(void)
{
    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.ByteWidth = sizeof( QuadWindowParams );
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;

    HRESULT hr =  device->CreateBuffer( &desc, NULL, &quadConstantBuffer ) ;
    assert(hr==S_OK);
}

void CompileShader(const char* shaderSource, size_t dataLen, ID3DBlob*& pID3DBlob, ID3DBlob*& pID3DErrorBlob, char* fn, char* profile)
{
    pID3DBlob = 0;
    pID3DErrorBlob = 0;

    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    HRESULT hr = D3DX11CompileFromMemory(shaderSource, dataLen, NULL, NULL, NULL, fn, profile, dwShaderFlags, NULL, NULL, &pID3DBlob, &pID3DErrorBlob, NULL);
    if (hr!=S_OK) {
        std::string strErr;
        strErr.resize(pID3DErrorBlob->GetBufferSize());
        memcpy(&strErr[0], pID3DErrorBlob->GetBufferPointer(), pID3DErrorBlob->GetBufferSize());
        std::cerr<<"Shader has error: "<<strErr<<std::endl;
    }
    assert(hr==S_OK);
}

ID3D11VertexShader* QuadVS = 0;
ID3DBlob* pID3DBlobVS = 0;
ID3DBlob* pID3DErrorBlobVS = 0;
ID3D11PixelShader*  QuadPS = 0;
ID3DBlob* pID3DBlobPS = 0;
ID3DBlob* pID3DErrorBlobPS = 0;
ID3D11InputLayout* quadInputLayout = 0;

void CreateShadersForQuad(void)
{
    char *shaderSource;
    unsigned int dataLen;
    bool ok = environment->GetResourceLoader()->LoadResource("Quad.hlsl", shaderSource, dataLen, true);
    assert(ok);

    HRESULT hr;

    CompileShader(shaderSource, dataLen, pID3DBlobVS, pID3DErrorBlobVS, "QuadVS", "vs_5_0");
    hr = device->CreateVertexShader(pID3DBlobVS->GetBufferPointer(), pID3DBlobVS->GetBufferSize(), NULL, &QuadVS);
    assert(hr==S_OK);

    CompileShader(shaderSource, dataLen, pID3DBlobPS, pID3DErrorBlobPS, "QuadPS", "ps_5_0");
    hr = device->CreatePixelShader(pID3DBlobPS->GetBufferPointer(), pID3DBlobPS->GetBufferSize(), NULL, &QuadPS);
    assert(hr==S_OK);

    environment->GetResourceLoader()->FreeResource(shaderSource);
}

void CreateInputLayoutForQuad()
{
    HRESULT hr = device->CreateInputLayout(
                     quad_layout_desc
                     , (UINT)2
                     , pID3DBlobVS->GetBufferPointer()
                     , pID3DBlobVS->GetBufferSize()
                     , &quadInputLayout);
    assert(hr==S_OK);
}
ID3D11RasterizerState *quadRasterizerState = 0;
ID3D11BlendState*quadBlendState = 0;
void CreateQuadRasterizerState()
{
    D3D11_RASTERIZER_DESC rdesc;
    rdesc.FillMode = D3D11_FILL_SOLID;
    rdesc.FrontCounterClockwise = TRUE;
    rdesc.DepthBias = 0;
    rdesc.DepthBiasClamp = 0;
    rdesc.SlopeScaledDepthBias = 0;
    rdesc.DepthClipEnable = TRUE;
    rdesc.ScissorEnable = FALSE;
    rdesc.MultisampleEnable = TRUE;
    rdesc.AntialiasedLineEnable = TRUE;
    rdesc.CullMode = D3D11_CULL_NONE;
    device->CreateRasterizerState(&rdesc, &quadRasterizerState);
}
void CreateQuadBlendState()
{
    D3D11_BLEND_DESC desc;
    ZeroMemory( &desc, sizeof(D3D11_BLEND_DESC) );

    desc.AlphaToCoverageEnable = false;
    desc.IndependentBlendEnable = false;
    desc.RenderTarget[0].RenderTargetWriteMask=0x0F;
    desc.RenderTarget[0].BlendEnable = FALSE;

    device->CreateBlendState(&desc, &quadBlendState);
}

void CreateQuadStuff()
{
    CreateShadersForQuad();
    CreateConstantBufferForQuad();
    CreateVertexBufferForQuad();
    CreateIndexBufferForQuad();
    CreateInputLayoutForQuad();
    CreateQuadRasterizerState();
    CreateQuadBlendState();
}

static ID3D11SamplerState* quadSamplerState = 0;

void RenderQuad(int x, int y, int width, int height, ID3D11DeviceContext*& context, int view)
{
    if (context==0)
        return;

    if (quadSamplerState==0) {
        CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        device->CreateSamplerState(&samplerDesc, &quadSamplerState);
    }


    D3D11_MAPPED_SUBRESOURCE MappedResource;
    context->Map( quadConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource ) ;
    QuadWindowParams* windowParams = ( QuadWindowParams* )MappedResource.pData;

    windowParams->window_xpos = (float)x;
    windowParams->window_ypos = (float)y;

    windowParams->viewport_width = (float)WINWIDTH;
    windowParams->viewport_height = (float)WINHEIGHT;

    windowParams->window_width = (float)width;
    windowParams->window_height = (float)height;

    context->Unmap( quadConstantBuffer, 0 );

    context->VSSetConstantBuffers( 0, 1, &quadConstantBuffer );

    context->RSSetState(quadRasterizerState);
    context->OMSetBlendState(quadBlendState, 0, 0xffffffff);

    context->VSSetShader(QuadVS, NULL, 0);
    context->CSSetShader(0, NULL, 0);
    context->DSSetShader(0, NULL, 0);
    context->HSSetShader(0, NULL, 0);
    context->GSSetShader(0, NULL, 0);

    context->PSSetShader(QuadPS, NULL, 0);

    context->PSSetSamplers(0, 1, &quadSamplerState);

    context->PSSetShaderResources( 0, 1, &rtt_srvs[view] );

    context->IASetInputLayout(quadInputLayout);

    UINT stride = sizeof(QuadVertex);
    UINT offset = 0;
    context->IASetVertexBuffers( 0, 1, &quadVertexBuffer, &stride, &offset );

    context->IASetIndexBuffer(quadIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->DrawIndexed(6,0,0);
}

void MakeHeightMap()
{
    Triton::Matrix4 heightMapMatrix;

    ID3D11Texture2D *d3d11Texture2D;

    float *data = new float[1024 * 1024];
    for (int row = 0; row < 1024; row++) {
        for (int col = 0; col < 1024; col++) {
            float height = (float)-row * 0.1f;
            int idx = row * 1024 + col;
            data[idx] = -10.0f;
        }
    }

    D3D11_TEXTURE2D_DESC sTexDesc;
    sTexDesc.Width = 1024;
    sTexDesc.Height = 1024;
    sTexDesc.MipLevels = 1;
    sTexDesc.ArraySize = 1;
    sTexDesc.Format = DXGI_FORMAT_R32_FLOAT;
    sTexDesc.SampleDesc.Count = 1;
    sTexDesc.SampleDesc.Quality = 0;
    sTexDesc.Usage = D3D11_USAGE_IMMUTABLE;
    sTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    sTexDesc.CPUAccessFlags = 0;
    sTexDesc.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA sSubData;
    sSubData.pSysMem = data;
    sSubData.SysMemPitch = (UINT)(1024 * 4);
    sSubData.SysMemSlicePitch = (UINT)(1024 * 1024 * 4);

    HRESULT hr = device->CreateTexture2D(&sTexDesc, &sSubData, &d3d11Texture2D);

    delete [] data;

    Triton::Matrix4 rot(1, 0, 0, 0,
                        0, 0, 1, 0,
                        0, 1, 0, 0,
                        0, 0, 0, 1);

    float s = 1.0f / 1024.0f;
    Triton::Matrix4 scale(s, 0, 0, 0,
                          0, s, 0, 0,
                          0, 0, s, 0,
                          0, 0, 0, 1);

    heightMapMatrix = rot * scale;

    ID3D11ShaderResourceView *srv;
    device->CreateShaderResourceView(d3d11Texture2D, 0, &srv);
    Triton::TextureHandle texHandle = srv;
    environment->SetHeightMap(texHandle, heightMapMatrix);

    environment->SetBelowWaterVisibility(10.0, Triton::Vector3(0, 0, 1));
    Triton::BreakingWavesParameters bwp;
    bwp.SetAmplitude(0);
    environment->SetBreakingWavesParameters(bwp);
}

void InitTriton()
{
    resourceLoader = new Triton::ResourceLoader("..\\..\\resources\\");

    environment = new Triton::Environment();
#ifdef GEOCENTRIC
    Triton::EnvironmentError err = environment->Initialize(Triton::WGS84_ZUP, Triton::DIRECTX_11, resourceLoader, device);
#else
    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP, Triton::DIRECTX_11, resourceLoader, device);
#endif

    if (err != Triton::SUCCEEDED) {
        printf ("Couldn't initialize Triton");
        exit(0);
    }

    environment->SetLicenseCode(TRITON_LICENSE_NAME, TRITON_LICENSE_CODE);

    if (testGodRays) {
        environment->SimulateSeaState(3.0, 0.0, true);
    } else {
        environment->SimulateSeaState(3.0, 0.0, true);
    }

#ifdef GEOCENTRIC
    //    environment->SetWorldUnits(0.001);
#endif

    ocean = Triton::Ocean::Create(environment, WATER_MODEL, true, true);

    //ocean->EnableWireframe(true);
    //ocean->SetDepth(1.0, Triton::Vector3(0.0, 1.0, 0.0));
    ocean->EnableSpray(true);
    ocean->EnableGodRays(true);

#if TEST_WAKES
    Triton::WakeGeneratorParameters params;
    params.sprayEffects = true;
    params.numHullSprays = 5;

    ship0 = new Ship(ocean, params, Vector3(0, 0, 1), 30.0);
#endif

#if TEST_ROTORWASH
    rotorWash = new Triton::RotorWash(ocean, 30.0, true, true);
#endif

    MakeHeightMap();
}

static ID3D11DeviceContext* GetDeviceContext(ID3D11Device* device, void* context, bool& hasRef)
{
    if (context) {
        return static_cast<ID3D11DeviceContext*>(context);
    }
    if (device==0) {
        return 0;
    }
    return immediateContext;
}

static void GetD3DRotMatrix(D3DXMATRIX& d3dRot, double yaw, double pitch)
{
    D3DXMATRIX d3dYaw, d3dPitch;
    D3DXMatrixRotationX(&d3dPitch, (float)pitch);
    D3DXMatrixRotationY(&d3dYaw, (float)(yaw));
    D3DXMatrixMultiply(&d3dRot, &d3dYaw, &d3dPitch);
}

static void GetD3DProj(D3DXMATRIX& d3dProj, ID3D11DeviceContext *deviceContext)
{
    UINT nViewports = 1;
    D3D11_VIEWPORT vp;
    deviceContext->RSGetViewports(&nViewports, &vp);
    D3DXMatrixPerspectiveFovLH(
        &d3dProj,
        45.0f * (D3DX_PI / 180.0f),
        (float)vp.Width / (float)vp.Height,
        2.0f,
        200000.0f);
}

static void GetD3DSkyBoxMatrix(D3DXMATRIX& d3dSkyBoxMatrix, double yaw, double pitch, ID3D11DeviceContext *deviceContext)
{
    D3DXMATRIX d3dRot;
    GetD3DRotMatrix(d3dRot, yaw, pitch);

    D3DXMATRIX d3dProj;
    GetD3DProj(d3dProj, deviceContext);

    D3DXMatrixMultiply(&d3dSkyBoxMatrix, &d3dRot, &d3dProj);
}

static void GetModelViewProjectionMatrices(double pView[16], double pProj[16], const Vector3& theCameraPosition, double yaw, double pitch, ID3D11DeviceContext *deviceContext, float timeDelta)
{
    //
    // Position and aim the camera.
    //
    float inc = timeDelta * 0.5f;

    D3DXMATRIX d3dRot;
    GetD3DRotMatrix(d3dRot, yaw, pitch);

    D3DXMATRIX d3dPos;
    D3DXMatrixTranslation(&d3dPos, (float)-theCameraPosition.x, (float)-theCameraPosition.y, (float)-theCameraPosition.z);

    D3DXMATRIX d3dView;
    D3DXMatrixMultiply(&d3dView, &d3dPos, &d3dRot);

    D3DXMATRIX d3dProj;
    GetD3DProj(d3dProj, deviceContext);

    int i = 0;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            pView[i] = d3dView(row, col);
            pProj[i] = d3dProj(row, col);
            i++;
        }
    }
}

static void SetupTritonCamera(Camera* camera, ID3D11DeviceContext *deviceContext, float timeDelta, const Vector3& theCameraPosition, double yaw, double pitch)
{
    double pView[16];
    double pProj[16];
    GetModelViewProjectionMatrices(pView, pProj, theCameraPosition, yaw, pitch, deviceContext, timeDelta);

    camera->SetCameraMatrix(pView);
    camera->SetProjectionMatrix(pProj);
}

void MoveShip(Ship* ship)
{
    if (ship==0) {
        return;
    }

    double nowMS = (double)timeGetTime();
    double nowS = nowMS * 0.001;
    static double startS = 0;
    if (startS == 0) startS = nowS;

    ship->update(nowS, nowS - startS);
}

void DrawViewsUsingImmediateContext(double millis)
{
    for (size_t i = 0; i < numViews; ++i) {
        SetRenderTarget(immediateContext, i);

        ocean->UpdateSimulation((double)millis * 0.001, cameras[i]);
        ocean->DrawConcurrent((double)millis * 0.001, true, true, true, immediateContext, cameras[i]);
    }

    SetUpMainView(immediateContext);


    ocean->UpdateSimulation((double)millis * 0.001, mainCamera);
    ocean->DrawConcurrent((double)millis * 0.001, true, true, true, immediateContext, mainCamera);
    ocean->PostDrawConcurrent();

    RenderQuad(20, 20, 256, 256, immediateContext, 0);
    RenderQuad(600, 20, 256, 256, immediateContext, 1);
}

void DrawViewsUsingDeferredContexts(double millis)
{
    // Render the views into the rtts using the deferred contexts
    for(size_t i = 0; i < numViews; ++i) {
        ID3D11DeviceContext *context = contexts[i];
        context->ClearState();
        SetRenderTarget(context, i);

        ocean->UpdateSimulation((double)millis * 0.001, cameras[i]);
        ocean->DrawConcurrent((double)millis * 0.001, true, true, true, context, cameras[i]);
        context->FinishCommandList(false, &command_lists[i]);
    }

    for(size_t i = 0; i < numViews; ++i) {
        // Execute the command lists on the immediate context
        immediateContext->ExecuteCommandList( command_lists[i], false );
        // Release it
        SAFE_RELEASE( command_lists[i] );
    }

    immediateContext->ClearState();
    // Render the main view in the immediate context
    SetUpMainView(immediateContext);

    ocean->UpdateSimulation((double)millis * 0.001, mainCamera);
    ocean->DrawConcurrent((double)millis * 0.001, true, true, true, immediateContext, mainCamera);

    ocean->PostDrawConcurrent();

    RenderQuad(20,20,256,256, immediateContext,0);
    RenderQuad(600,20,256,256, immediateContext,1);
}

unsigned int WINAPI _RenderViewThreadProc( LPVOID lpParameter )
{
    // thread local data
    const int view = *( int*)lpParameter;
    ID3D11DeviceContext *context = contexts[view];
    ID3D11CommandList*& pd3dCommandList = command_lists[view];

    for (;;) {
        // Wait for main thread to signal ready
        WaitForSingleObject( g_hBeginPerSceneRenderDeferredEvent[view], INFINITE );

        ID3D11DeviceContext *context = contexts[view];
        SetRenderTarget(context, view);
        DWORD millis = timeGetTime();
        if (cameras[view]) {
            ocean->DrawConcurrent((double)millis * 0.001, true, true, true, context, cameras[view]);
        }
        context->FinishCommandList(false, &command_lists[view]);

        // Tell main thread command list is finished
        SetEvent( g_hEndPerSceneRenderDeferredEvent[view] );
    }
}

unsigned int WINAPI _RenderViewMainThreadProc( LPVOID lpParameter )
{
    // thread local data
    const int view = *( int*)lpParameter;
    ID3D11DeviceContext *context = context_main;
    ID3D11CommandList*& pd3dCommandList = command_list_main;

    for (;;) {
        // Wait for main thread to signal ready
        WaitForSingleObject( g_hBeginPerSceneRenderDeferredEventMain, INFINITE );

        ID3D11DeviceContext *context = context_main;
        SetUpMainView(context);
        DWORD millis = timeGetTime();
        ocean->DrawConcurrent((double)millis * 0.001, true, true, true, context, mainCamera);

        context->FinishCommandList(false, &command_list_main);

        // Tell main thread command list is finished
        SetEvent( g_hEndPerSceneRenderDeferredEventMain );
    }
}

void InitThreadingStuff(void)
{
    for(int i = 0; i < numViews; ++i) {
        g_iPerSceneThreadInstanceData[i] = i;

        g_hBeginPerSceneRenderDeferredEvent[i] = CreateEvent( NULL, FALSE, FALSE, NULL );
        g_hEndPerSceneRenderDeferredEvent[i] = CreateEvent( NULL, FALSE, FALSE, NULL );

        g_hPerSceneRenderDeferredThread[i] = ( HANDLE )_beginthreadex(
                NULL,
                0,
                _RenderViewThreadProc,
                &g_iPerSceneThreadInstanceData[i],
                CREATE_SUSPENDED,
                NULL );

        ResumeThread( g_hPerSceneRenderDeferredThread[i] );
    }

    g_iPerSceneThreadInstanceDataMain = 99;

    g_hBeginPerSceneRenderDeferredEventMain = CreateEvent( NULL, FALSE, FALSE, NULL );
    g_hEndPerSceneRenderDeferredEventMain = CreateEvent( NULL, FALSE, FALSE, NULL );

    g_hPerSceneRenderDeferredThreadMain = ( HANDLE )_beginthreadex(
            NULL,
            0,
            _RenderViewMainThreadProc,
            &g_iPerSceneThreadInstanceDataMain,
            CREATE_SUSPENDED,
            NULL );

    ResumeThread( g_hPerSceneRenderDeferredThreadMain );
}

void DrawViewsUsingDeferredContextsInThreads(double millis)
{
    // Signal all worker threads, then wait for completion
    for ( int iInstance = 0; iInstance < numViews; ++iInstance ) {
        // signal ready for scene kickoff

        if (cameras[iInstance]) {
            ocean->UpdateSimulation((double)millis * 0.001, cameras[iInstance]);
        }

        SetEvent( g_hBeginPerSceneRenderDeferredEvent[iInstance] );
    }


    ocean->UpdateSimulation((double)millis * 0.001, mainCamera);
    SetEvent(g_hBeginPerSceneRenderDeferredEventMain);

    HANDLE end_events[numViews+1];
    for ( int iInstance = 0; iInstance < numViews; ++iInstance ) {
        end_events[iInstance] = g_hEndPerSceneRenderDeferredEvent[iInstance];
    }
    end_events[numViews] = g_hEndPerSceneRenderDeferredEventMain;

    // wait for completion
    WaitForMultipleObjects( numViews+1,
                            end_events,
                            TRUE,
                            INFINITE );

    for(size_t i = 0; i < numViews; ++i) {
        // Execute the command lists on the immediate context
        immediateContext->ExecuteCommandList( command_lists[i], false );
        // Release it
        SAFE_RELEASE( command_lists[i] );
    }
    immediateContext->ExecuteCommandList(command_list_main, false);
    SAFE_RELEASE( command_list_main );

    ocean->PostDrawConcurrent();

    SetUpMainView(immediateContext);
    if (cameras[0]) {
        RenderQuad(20,20,256,256, immediateContext,0);
    }
    if (cameras[1]) {
        RenderQuad(600,20,256,256, immediateContext,1);
    }

    static double lastUpdateTime = millis;
    static double elapsedTime = 0;
    elapsedTime += millis-lastUpdateTime;
    lastUpdateTime = millis;

    //if (elapsedTime>8000) {
    //    if (cameras[1]) {
    //        environment->DestroyCamera(cameras[1]);
    //        cameras[1] = 0;
    //    }
    //}
    //if (elapsedTime>10000) {
    //    if (cameras[0]) {
    //        environment->DestroyCamera(cameras[0]);
    //        cameras[0] = 0;
    //    }
    //}
}


// Main function to draw a frame of animation.
static void RenderFrame(HWND hWnd)
{
    static unsigned int frameCount = 0;
    static DWORD frameTime = timeGetTime();

    static double fps = 0;
    if (frameCount == 100) {
        DWORD now = timeGetTime();
        fps = 100.0 / ((double)(now - frameTime) * 0.001);
        frameTime = now;
        frameCount = 0;

        char buf[1024];
        sprintf_s(buf, 1024, "%5.2f fps\n", fps);
        OutputDebugStringA(buf);
    }
    frameCount++;

    static float lastTime = (float)timeGetTime();

    if (device) {
        float currTime = (float)timeGetTime();
        float timeDelta = (currTime - lastTime) * 0.001f;

#ifdef GEOCENTRIC
        double x = 4698493.0 + yaw, y = 4313093.0 + yaw, z = -46044.0 - yaw;
        Vector3 theCameraPosition = Vector3(-x, -y, -z);
#else

        Vector3 theCameraPosition;
        if (testGodRays) {
            theCameraPosition = Vector3((float)rotorPosition.x, -5, -150);
        } else {
            theCameraPosition = cameraPosition;
        }
#endif

        double pitch = 0;
        if (testGodRays) {
            pitch = 40.0f * (3.14f / 360.0f);
        } else {
            pitch = -15.0f * (3.14f / 360.0f);
        }

        if (testGodRays) {
            yaw = 10.7f;
        }

        D3DXMATRIX skyBoxViewProj;
        GetD3DSkyBoxMatrix(skyBoxViewProj, yaw, pitch, immediateContext);

        float yawOffset = 0.5f;

        Vector3 camPosView0 = theCameraPosition;
        Vector3 camPosView1 = theCameraPosition;

        SetupTritonCamera(mainCamera, immediateContext, timeDelta, theCameraPosition, yaw-3.14, pitch);
        if (cameras[0]) {
            SetupTritonCamera(cameras[0], immediateContext, timeDelta, camPosView0, yaw, pitch);
        }
        if (cameras[1]) {
            SetupTritonCamera(cameras[1], immediateContext, timeDelta, camPosView1, yaw, pitch);
        }

        MoveShip(ship0);

        if (ocean && environment) {

            Triton::Vector3 lightDirection(0.9539f, 0.16765f, -0.24861f);
            Triton::Vector3 lightColor(1.0000000f, 0.93333638f, 0.76101780f);
            environment->SetDirectionalLight(lightDirection, lightColor);

            Triton::Vector3 ambientColor(0.8644f,0.8644f,0.8644f);
            environment->SetAmbientLight(ambientColor);

            Triton::Matrix3 flip(1.0, 0.0, 0.0,    0.0, -1.0, 0.0,   0.0, 0.0, 1.0);
            environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubeMap()), flip);

            DWORD millis = timeGetTime();

#if TEST_ROTORWASH
            if (rotorWash) {
                double nowMS = (double)millis;
                double nowS = nowMS * 0.001;
                rotorWash->Update(rotorPosition, rotorDirection, 44.0, nowS);
            }
#endif
            immediateContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);

            if (drawStrategy==DRAWVIEWSUSINGIMMEDIATECONTEXT) {
                DrawViewsUsingImmediateContext(millis);
            } else if (drawStrategy == DRAWVIEWSUSINGDEFERREDCONTEXTS) {
                DrawViewsUsingDeferredContexts(millis);
            } else if (drawStrategy == DRAWVIEWSUSINGDEFERREDCONTEXTSINTHREADS) {
                DrawViewsUsingDeferredContextsInThreads(millis);
            }
        }

        SetUpMainView(immediateContext);

        // Draw the skybox last:
        skyBox->Draw(skyBoxViewProj);

        swapChain->Present(0, 0);

        // Trigger another redraw.
        InvalidateRect(hWnd, NULL, FALSE);

        lastTime = currTime;
    }
}

static void DeleteShip(Ship*& ship)
{
    if (ship) {
        delete ship;
        ship = 0;
    }
}

static void DestroyD3D()
{
    if (skyBox) {
        delete skyBox;
        skyBox = 0;
    }

#if TEST_WAKES
    DeleteShip(ship0);
#endif

#if TEST_ROTORWASH
    if (rotorWash) {
        delete rotorWash;
        rotorWash = 0;
    }
#endif

    if (ocean) {
        delete ocean;
        ocean = 0;
    }

    if (environment) {
        delete environment;
        environment = 0;
    }

    if (resourceLoader) {
        delete resourceLoader;
        resourceLoader = 0;
    }

    SAFE_RELEASE(quadBlendState);
    SAFE_RELEASE(quadSamplerState);
    SAFE_RELEASE(quadInputLayout);
    SAFE_RELEASE(QuadVS);
    SAFE_RELEASE(QuadPS);
    SAFE_RELEASE(quadVertexBuffer);
    SAFE_RELEASE(quadIndexBuffer);
    SAFE_RELEASE(quadConstantBuffer);
    SAFE_RELEASE(quadSamplerState);
    SAFE_RELEASE(quadRasterizerState);

    ReleaseViews();

    SAFE_RELEASE(depthStencilView);
    SAFE_RELEASE(depthStencilBuffer);
    SAFE_RELEASE(renderTargetView);
    SAFE_RELEASE(swapChain);

    int i = 0;

    immediateContext->ClearState();
    immediateContext->Flush();
    i = immediateContext->Release();
#if 0
    if (i!=0) {
        assert(FALSE);
    }
#endif

#if 1
    ID3D11Debug* DebugDevice = 0;
    device->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&DebugDevice));
    DebugDevice->ReportLiveDeviceObjects( D3D11_RLDO_SUMMARY | D3D11_RLDO_DETAIL );
    DebugDevice->Release();
#endif

#if 0
    i = device->Release();
    if (i!=0) {
        assert(FALSE);
    }
#endif
}

static ID3D11Device *InitD3D(HWND hwnd)
{
    HRESULT hr;

    DXGI_SWAP_CHAIN_DESC sd;
    sd.BufferDesc.Width = WINWIDTH;
    sd.BufferDesc.Height = WINHEIGHT;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;

    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 1;
    sd.OutputWindow = hwnd;
    sd.Windowed = true;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    UINT createDeviceFlags = 0;
    //#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    //#endif
    hr = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
                                       NULL, 0, D3D11_SDK_VERSION, &sd, &swapChain, &device, NULL, &immediateContext);

    if (FAILED(hr)) return 0;

    ID3D11Texture2D *backBuffer;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
    device->CreateRenderTargetView(backBuffer, 0, &renderTargetView);
    backBuffer->Release();

    D3D11_TEXTURE2D_DESC depthStencilDesc;
    depthStencilDesc.Width = WINWIDTH;
    depthStencilDesc.Height = WINHEIGHT;
    depthStencilDesc.MipLevels = 1;
    depthStencilDesc.ArraySize = 1;
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
    depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depthStencilDesc.CPUAccessFlags = 0;
    depthStencilDesc.MiscFlags = 0;

    device->CreateTexture2D(&depthStencilDesc, 0, &depthStencilBuffer);
    device->CreateDepthStencilView(depthStencilBuffer, 0, &depthStencilView);

    SetUpMainView(immediateContext);

    return device;
}

#define IS_CONSOLE 0

#if IS_CONSOLE
int main(void)
{
    _tWinMain(GetModuleHandle(0), 0, 0, 9);
    return 0;
}
#endif

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_DIRECTX11MULTITHREADEDSAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTX11MULTITHREADEDSAMPLE));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

// Standard windows class registration stuff
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDC_DIRECTX11MULTITHREADEDSAMPLE));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_DIRECTX11MULTITHREADEDSAMPLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    RECT rect;
    GetClientRect(hWnd, &rect);

    device = InitD3D(hWnd);
    if (device) {

        InitTriton();
        InitViews();
        InitThreadingStuff();
        CreateQuadStuff();
    } else {
        return FALSE;
    }

    // Create our sky box
    skyBox = new SkyBox(device);
    if (!skyBox->Create()) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId) {
        case IDM_ABOUT:
            DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            DestroyWindow(hWnd);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code here...
        RenderFrame(hWnd);

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        DestroyD3D();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

