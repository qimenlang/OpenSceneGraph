// Copyright 2011 Sundog Software LLC. All rights reserved worldwide.

#include <d3dx9.h>

// A class that draws a skybox using a cube map texture.
class SkyBox
{
public:
    // Constructor takes the device to render to.
    SkyBox(LPDIRECT3DDEVICE9 pDevice) : device(pDevice), cubeMap(0), vertexBuffer(0), fx(0) {}

    // Cleans up resources.
    ~SkyBox();

    // Creates underlying resources
    bool Create();

    // Call at the end of the frame, with the camera rotation/translation matrix, and
    // a world matrix for positioning the skybox at the camera position.
    void Draw(const D3DXMATRIX& viewproj, const D3DXMATRIX& world);

    // Retrieves the underlying cube map texture.
    LPDIRECT3DCUBETEXTURE9 GetCubeMap() const {
        return cubeMap;
    }

private:
    LPDIRECT3DDEVICE9 device;
    LPDIRECT3DCUBETEXTURE9 cubeMap;
    LPDIRECT3DVERTEXBUFFER9 vertexBuffer;
    LPDIRECT3DINDEXBUFFER9 indexBuffer;
    ID3DXEffect *fx;
};