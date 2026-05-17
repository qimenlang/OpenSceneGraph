// Copyright (c) 2011 Sundog Software LLC. All rights reserved worldwide.
#pragma once

// A class that renders a sky box in DirectX 11, and exposes its underlying
// cube map for use as an environment map.
class SkyBox
{
public:
    SkyBox(ID3D11Device *pDevice);
    ~SkyBox();

    bool Create();

    bool Draw(const D3DXMATRIX& viewProj);

    ID3D11ShaderResourceView *GetCubeMap() const {
        return cubeMapSRV;
    }

private:
    bool CreateBuffers();
    bool CreateCubeMap();
    bool CreateShader();
    bool CreateStates();

    ID3D11Buffer *vertexBuffer, *indexBuffer, *matrixBuffer;
    ID3D11Device *device;
    ID3D11VertexShader *vertexShader;
    ID3D11PixelShader *pixelShader;
    ID3D11InputLayout *inputLayout;
    ID3D11DepthStencilState *dssState;
    ID3D11RasterizerState *rsState;
    ID3D11SamplerState *cubeMapSamplerState;

    ID3D11Texture2D* cubeMapTexture;
    ID3D11ShaderResourceView *cubeMapSRV;
};