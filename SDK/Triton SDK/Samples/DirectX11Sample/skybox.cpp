// Copyright (c) 2011 Sundog Software LLC. All rights reserved worldwide.

#include "stdafx.h"

typedef struct Vertex_S {
    float x, y, z;
} Vertex;

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

SkyBox::SkyBox(ID3D11Device *pDevice) : device(pDevice), vertexBuffer(0), indexBuffer(0),
    vertexShader(0), pixelShader(0), inputLayout(0), matrixBuffer(0), dssState(0), rsState(0),
    cubeMapSamplerState(0), cubeMapTexture(0), cubeMapSRV(0)
{

}

SkyBox::~SkyBox()
{
    if (vertexBuffer) vertexBuffer->Release();
    if (indexBuffer) indexBuffer->Release();
    if (vertexShader) vertexShader->Release();
    if (pixelShader) pixelShader->Release();
    if (inputLayout) inputLayout->Release();
    if (matrixBuffer) matrixBuffer->Release();
    if (dssState) dssState->Release();
    if (rsState) rsState->Release();
    if (cubeMapTexture) cubeMapTexture->Release();
    if (cubeMapSamplerState) cubeMapSamplerState->Release();
    if (cubeMapSRV) cubeMapSRV->Release();
}

bool SkyBox::Create()
{
    return CreateBuffers() && CreateCubeMap() && CreateShader() && CreateStates();
}

// Create our states for disabling culling and setting our depth test flags
bool SkyBox::CreateStates()
{
    D3D11_RASTERIZER_DESC cmdesc;
    ZeroMemory(&cmdesc, sizeof(D3D11_RASTERIZER_DESC));
    cmdesc.FillMode = D3D11_FILL_SOLID;
    cmdesc.CullMode = D3D11_CULL_NONE;
    HRESULT hr = device->CreateRasterizerState(&cmdesc, &rsState);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_DESC dssDesc;
    ZeroMemory(&dssDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
    dssDesc.DepthEnable = true;
    dssDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dssDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
    hr = device->CreateDepthStencilState(&dssDesc, &dssState);
    if (FAILED(hr)) return false;

    return true;
}

// Create our vertex and index buffers
bool SkyBox::CreateBuffers()
{
    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_DEFAULT;
    vbd.ByteWidth = sizeof(Vertex) * 8;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;

    D3D11_SUBRESOURCE_DATA srd;
    srd.pSysMem = vertices;
    srd.SysMemPitch = srd.SysMemSlicePitch = 0;
    HRESULT hr = device->CreateBuffer(&vbd, &srd, &vertexBuffer);
    if (FAILED(hr)) return false;

    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_DEFAULT;
    ibd.ByteWidth = sizeof(UINT16) * 12 * 3;
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;

    srd.pSysMem = indices;
    hr = device->CreateBuffer(&ibd, &srd, &indexBuffer);
    if (FAILED(hr)) return false;

    return true;
}

typedef struct MatrixBuffer_S {
    D3DXMATRIX gViewProj;
} MatrixBufferType;

// The effect source; transforms each point to be infinitely distant
// and looks up the cube map texture.
const char *shader =
    "cbuffer MatrixBuffer \n"
    "{ \n"
    "   float4x4 gViewProj : VIEWPROJECTION; \n"
    "} \n"

    "TextureCube skyBox; \n"

    "SamplerState linearClamp; \n "

    "struct PS_INPUT_SKY \n"
    "{ \n"
    "   float4 Pos : SV_POSITION; \n"
    "   float3 Tex : TEXCOORD0; \n"
    "}; \n"

    "PS_INPUT_SKY VS_Sky(float3 pos: POSITION) \n"
    "{ \n"
    "   PS_INPUT_SKY output = (PS_INPUT_SKY)0; \n"
    "   float4 hPos = float4(pos, 0); \n"
    "   output.Pos = hPos; \n"
    "   output.Pos = mul(output.Pos, gViewProj).xyww; \n"
    "   output.Tex = pos.xyz; \n"
    "   return output; \n"
    "} \n"

    "float4 PS_Sky(PS_INPUT_SKY input) : SV_TARGET \n"
    "{ \n"
    "   return skyBox.Sample(linearClamp, input.Tex); \n"
    "} \n";

bool SkyBox::CreateShader()
{
    // Input format
    D3D11_INPUT_ELEMENT_DESC polygonLayout[1];
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    // Load the vertex shader
    DWORD shaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;
    ID3D10Blob *compilationErrors = 0, *effectBlob = 0;

    HRESULT hr = D3DX11CompileFromMemory(shader, strlen(shader), NULL, NULL, NULL, "VS_Sky", "vs_4_0", shaderFlags,
                                         0, NULL, &effectBlob, &compilationErrors, NULL);
    if (FAILED(hr)) {
        if (compilationErrors) {
            OutputDebugStringA((char *)compilationErrors->GetBufferPointer());
            compilationErrors->Release();
        }
        return false;
    }

    hr = device->CreateVertexShader(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), NULL, &vertexShader);
    if (FAILED(hr)) return false;

    // Create the input layout
    hr = device->CreateInputLayout(polygonLayout, 1, effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), &inputLayout);
    if (FAILED(hr)) return false;

    effectBlob->Release();

    // Load the pixel shader
    hr = D3DX11CompileFromMemory(shader, strlen(shader), NULL, NULL, NULL, "PS_Sky", "ps_4_0", shaderFlags,
                                 0, NULL, &effectBlob, &compilationErrors, NULL);
    if (FAILED(hr)) {
        if (compilationErrors) {
            OutputDebugStringA((char *)compilationErrors->GetBufferPointer());
            compilationErrors->Release();
        }
        return false;
    }

    hr = device->CreatePixelShader(effectBlob->GetBufferPointer(), effectBlob->GetBufferSize(), NULL, &pixelShader);
    if (FAILED(hr)) return false;

    effectBlob->Release();

    // Create our constant buffer
    D3D11_BUFFER_DESC matrixBufferDesc;
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    matrixBufferDesc.MiscFlags = 0;
    matrixBufferDesc.StructureByteStride = 0;

    hr = device->CreateBuffer(&matrixBufferDesc, NULL, &matrixBuffer);
    if (FAILED(hr)) return false;

    // Create our sampler state
    CD3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;

    hr = device->CreateSamplerState(&samplerDesc, &cubeMapSamplerState);

    return (SUCCEEDED(hr));
}

bool SkyBox::Draw(const D3DXMATRIX& viewProj)
{
    ID3D11DeviceContext *context;
    device->GetImmediateContext(&context);
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    HRESULT hr = context->Map(matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(hr)) {
        context->Release();
        return false;
    }

    MatrixBufferType *type = (MatrixBufferType *)(mappedResource.pData);

    D3DXMATRIX transposed;
    D3DXMatrixTranspose(&transposed, &viewProj);
    type->gViewProj = transposed;

    context->Unmap(matrixBuffer, 0);

    context->VSSetConstantBuffers(0, 1, &matrixBuffer);

    unsigned int stride;
    unsigned int offset;
    stride = sizeof(Vertex);
    offset = 0;
    context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
    context->IASetIndexBuffer(indexBuffer, DXGI_FORMAT_R16_UINT, 0);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    context->IASetInputLayout(inputLayout);
    context->VSSetShader(vertexShader, NULL, 0);
    context->PSSetShader(pixelShader, NULL, 0);

    context->OMSetDepthStencilState(dssState, 0);
    context->RSSetState(rsState);

    context->PSSetSamplers(0, 1, &cubeMapSamplerState);
    context->PSSetShaderResources(0, 1, &cubeMapSRV);

    context->DrawIndexed(12 * 3, 0, 0);

    context->VSSetShader(NULL, NULL, 0);
    context->PSSetShader(NULL, NULL, 0);
    context->OMSetDepthStencilState(NULL, 0);
    context->RSSetState(NULL);

    context->Release();
    return true;
}

bool SkyBox::CreateCubeMap()
{
    D3DX11_IMAGE_LOAD_INFO loadSMInfo;
    loadSMInfo.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;

    HRESULT hr = D3DX11CreateTextureFromFile(device, L"..\\skybox.dds",
                 &loadSMInfo, 0, (ID3D11Resource**)&cubeMapTexture, 0);
    if (FAILED(hr)) return false;

    D3D11_TEXTURE2D_DESC SMTextureDesc;
    cubeMapTexture->GetDesc(&SMTextureDesc);

    D3D11_SHADER_RESOURCE_VIEW_DESC SMViewDesc;
    SMViewDesc.Format = SMTextureDesc.Format;
    SMViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
    SMViewDesc.TextureCube.MipLevels = SMTextureDesc.MipLevels;
    SMViewDesc.TextureCube.MostDetailedMip = 0;

    hr = device->CreateShaderResourceView(cubeMapTexture, &SMViewDesc, &cubeMapSRV);
    if (FAILED(hr)) return false;

    return true;
}