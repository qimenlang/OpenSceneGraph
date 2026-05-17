#include "skybox.h"

// Each vertex only contains x,y,z position values, which
// double as the texture coordinates.
struct Vertex {
    float x, y, z;
};

// The 8 points of the sky box
Vertex vertices[8] = {
    { 1,  1, -1 },
    {-1,  1, -1 },
    {-1, -1, -1 },
    { 1, -1, -1 },

    { 1,  1,  1 },
    {-1,  1,  1 },
    {-1, -1,  1 },
    { 1, -1,  1 }
};

// 2 triangles each for each of the 6 faces of the
// sky box, all winding inward.
unsigned short indices[] = {
    0, 1, 3,
    1, 2, 3,
    0, 4, 1,
    1, 4, 5,
    5, 4, 7,
    5, 7, 6,
    1, 5, 6,
    1, 6, 2,
    0, 3, 7,
    0, 7, 4,
    2, 6, 7,
    2, 7, 3
};

// The effect source; transforms each point to be infinitely distant
// and looks up the cube map texture.
const char *shader =
    "uniform float4x4 gViewProj : VIEWPROJECTION; \n"
    "uniform float4x4 gWorld : WORLD; \n"
    "texture skyBox; \n"

    "samplerCUBE skySampler = sampler_state \n"
    "{ \n"
    "   texture = <skyBox>; \n"
    "   magfilter = LINEAR; \n"
    "   minfilter = LINEAR; \n"
    "   mipfilter = LINEAR; \n"
    "   AddressU = CLAMP; \n"
    "   AddressV = CLAMP; \n"
    "   AddressW = CLAMP; \n"
    "}; \n"

    "struct PS_INPUT_SKY \n"
    "{ \n"
    "   float4 Pos : POSITION; \n"
    "   float3 Tex : TEXCOORD0; \n"
    "}; \n"

    "PS_INPUT_SKY VS_Sky(float3 pos: POSITION) \n"
    "{ \n"
    "   PS_INPUT_SKY output = (PS_INPUT_SKY)0; \n"
    "   float4 hPos = float4(pos, 0); \n"
    "   output.Pos = mul(hPos, gWorld); \n"
    "   output.Pos = mul(output.Pos, gViewProj).xyww; \n"
    "   output.Tex = pos.xyz; \n"
    "   return output; \n"
    "} \n"

    "float4 PS_Sky(PS_INPUT_SKY input) : COLOR0 \n"
    "{ \n"
    "   return texCUBE(skySampler, input.Tex); \n"
    "} \n"

    "technique Default \n"
    "{ \n"
    "   pass P1 \n"
    "   { \n"
    "       VertexShader = compile vs_3_0 VS_Sky(); \n"
    "       PixelShader = compile ps_3_0 PS_Sky(); \n"
    "   } \n"
    "} \n";

// Clean up resources.
SkyBox::~SkyBox()
{
    if (fx) fx->Release();
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
    if (cubeMap) cubeMap->Release();
}

// Create resources.
bool SkyBox::Create()
{
    HRESULT hRet;

    hRet = device->CreateVertexBuffer( sizeof(Vertex) * 8, 0, D3DFVF_XYZ, D3DPOOL_MANAGED,
                                       &vertexBuffer, NULL );
    if ( FAILED( hRet ) ) {
        ::MessageBox(NULL, L"Failed to create skybox vertex buffer", L"Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    void *pVertices = NULL;

    vertexBuffer->Lock( 0, sizeof(Vertex) * 8, (void**)&pVertices, 0 );
    memcpy( pVertices, vertices, sizeof(Vertex) * 8 );
    vertexBuffer->Unlock();

    hRet = device->CreateIndexBuffer(3 * 12 * sizeof(unsigned short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                     D3DPOOL_MANAGED, &indexBuffer, NULL);
    if (FAILED(hRet)) {
        ::MessageBox(NULL, L"Failed to create skybox index buffer", L"Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    void *pIndices = NULL;
    indexBuffer->Lock(0, 3 * 12 * sizeof(unsigned short), (void**)&pIndices, 0);
    memcpy(pIndices, indices, 3 * 12 * sizeof(unsigned short));
    indexBuffer->Unlock();

    hRet = D3DXCreateCubeTextureFromFile(device, L"..\\skybox.dds", &cubeMap);
    if ( FAILED(hRet) ) {
        ::MessageBox(NULL, L"Failed to open skybox.dds", L"Error", MB_OK | MB_ICONSTOP);
        return false;
    }

    ID3DXBuffer *compilationErrors;
    hRet = D3DXCreateEffect(
               device,
               shader,
               strlen(shader),
               NULL,
               NULL,
               0,
               NULL,
               &fx,
               &compilationErrors
           );

    if (FAILED(hRet)) {
        ::MessageBox(NULL, L"Failed to load skybox shader", L"Error", MB_OK | MB_ICONSTOP);
        if (compilationErrors) {
            OutputDebugStringA((char *)compilationErrors->GetBufferPointer());
        }
        return false;
    }

    return true;
}

// Draw the skybox using the given camera matrices
void SkyBox::Draw(const D3DXMATRIX& viewProj, const D3DXMATRIX& world)
{
    device->SetRenderState( D3DRS_ZWRITEENABLE, false );
    device->SetRenderState( D3DRS_LIGHTING, false );
    device->SetRenderState( D3DRS_ZFUNC, D3DCMP_LESSEQUAL );
    device->SetRenderState( D3DRS_CULLMODE, D3DCULL_CW );

    HRESULT hr;
    hr = fx->SetMatrix("gViewProj", &viewProj);
    hr |= fx->SetMatrix("gWorld", &world);
    hr |= fx->SetTexture("skyBox", cubeMap);

    device->SetFVF( D3DFVF_XYZ );
    device->SetStreamSource( 0, vertexBuffer, 0, sizeof(Vertex));
    device->SetIndices(indexBuffer);

    UINT passes;
    fx->Begin(&passes, 0);
    fx->BeginPass(0);

    device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 8, 0, 12);

    fx->EndPass();
    fx->End();

    device->SetStreamSource(0, NULL, 0, 0);
    device->SetIndices(NULL);
}