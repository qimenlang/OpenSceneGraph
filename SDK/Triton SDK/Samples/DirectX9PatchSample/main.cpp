// Triton DirectX 9 Patch Sample Application
// Illustrates integration of Triton with a CubeMap-based sky box in a simple Win32 app
// Instead of drawing its own infinite ocean, this sample shades user-drawn geometry as water
// using Triton.

// Copyright (c) 2012-2016 Sundog Software, LLC. All rights reserved worldwide.

#include "../SamplesCommon/Licenses.h"
#include <Windows.h>
#include <MMSystem.h>
#include <d3dx9.h>
#include "Triton.h"
#include "skybox.h"

#define WINWIDTH 800
#define WINHEIGHT 600

// Static pointers to the Triton objects we need:
static Triton::ResourceLoader *resourceLoader = 0;
static Triton::Environment *environment = 0;
static Triton::Ocean *ocean = 0;

// The sky for this demo is encapsulated in the SkyBox class
// It exposes its underlying cube map which we use for reflections
// in Triton.
static SkyBox *skyBox = 0;

// Global Variables for Windows:
static HINSTANCE hInst;
static TCHAR szTitle[] = L"Triton DX9 Sample";
static TCHAR szWindowClass[] = L"TRITONDX9SAMPLE";

// Camera properties:
static float aspectRatio, yaw = 0, pitch = 0;

// Direct3D9 stuff:
static IDirect3DDevice9 *device = 0;
static IDirect3D9 *d3d9 = 0;
static D3DPRESENT_PARAMETERS d3dpp;

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

// Instantiates the Triton objects
bool InitTriton()
{
    // We create a default resource loader here; you could extend this class to hook
    // into your own resource manager.
    resourceLoader = new Triton::ResourceLoader("..\\..\\resources\\");

    // Create a DX9 environment in a flat-Earth, Y-is-up coordinate system
    environment = new Triton::Environment();
    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP, Triton::DIRECTX_9,
                                   resourceLoader, device);
    if (err != Triton::SUCCEEDED) {
        ::MessageBoxA(NULL, "Failed to initialize Triton - is the resource path passed in to "
                      "the ResouceLoader constructor valid?", "Triton error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    // Substitute your own license information here once you've purchased a license
    // (visit http://www.sundog-soft.com/ to do so if you're so inclined)
    environment->SetLicenseCode(TRITON_LICENSE_NAME, TRITON_LICENSE_CODE);

    // Add wind at 10 meters per second blowing North
    Triton::WindFetch wf;
    wf.SetWind(10.0, 0.0);
    environment->AddWindFetch(wf);

    // Create the Ocean itself using the environment we've created, and FFT waves.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    ocean = Triton::Ocean::Create(environment, Triton::JONSWAP);

    if (ocean) ocean->EnableSpray(false);

    return (ocean != NULL);
}

// Data for our grid that we'll render as water:
IDirect3DVertexBuffer9 * vertexBuffer = 0;
IDirect3DIndexBuffer9 * indexBuffer = 0;
IDirect3DVertexDeclaration9 *vertexDeclaration = 0;

const D3DVERTEXELEMENT9 declaration[] = {
    {
        0, 0,
        D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT,
        D3DDECLUSAGE_POSITION, 0
    },
    D3DDECL_END()
};

typedef struct Vertex_S {
    float x, y, z, w;
} Vertex;

int nVerts, nIndices;

// Set up the verts and indices for our mesh, which we'll draw but have Triton shade:
void InitPatch()
{
    HRESULT hr = device->CreateVertexDeclaration(declaration, &vertexDeclaration);

    nVerts = 50 * 50;
    Vertex *v = new Vertex[nVerts];
    for (int row = 0; row < 50; row++) {
        for (int col = 0; col < 50; col++) {
            int idx = row * 50 + col;
            v[idx].x = (float)col * 1.0f - 25.0f;
            v[idx].z = (float)row * 1.0f - 25.0f;
            v[idx].y = 0.0f;
            v[idx].w = 1.0;
        }
    }

    hr = device->CreateVertexBuffer( nVerts * sizeof(Vertex),
                                     0,
                                     0,
                                     D3DPOOL_MANAGED,
                                     &vertexBuffer, 0);

    if (hr == D3D_OK) {
        Vertex *vertices;
        hr = vertexBuffer->Lock(0, nVerts * sizeof(Vertex), (void **)&vertices, 0);
        if (hr == D3D_OK) {
            memcpy(vertices, v, nVerts * sizeof(Vertex));
            vertexBuffer->Unlock();
        }
    }

    delete[] v;

    // Create the index array
    unsigned int indicesPerStrip = (50 * 2 + 2);
    nIndices = indicesPerStrip * 49;

    UINT16 *i = new UINT16[nIndices];

    unsigned int vIdx = 0, iIdx = 0;

    for (unsigned int stripRow = 0; stripRow < 49; stripRow++) {
        i[iIdx++] = vIdx;
        for (unsigned int col = 0; col < 50; col++) {
            i[iIdx++] = vIdx;
            i[iIdx++] = vIdx + 50;
            vIdx++;
        }
        i[iIdx++] = vIdx - 1 + 50;
    }

    hr = device->CreateIndexBuffer(nIndices * sizeof(UINT16), D3DUSAGE_WRITEONLY,
                                   D3DFMT_INDEX16,
                                   D3DPOOL_MANAGED,
                                   &indexBuffer, 0);
    if (hr == D3D_OK) {
        UINT32 *indices;
        hr = indexBuffer->Lock(0, nIndices * sizeof(UINT16), (void **)&indices, 0);
        if (hr == D3D_OK) {
            memcpy(indices, i, nIndices * sizeof(UINT16));
            indexBuffer->Unlock();
        }
    }

    delete[] i;
}

// Standard Direct3D9 initialization stuff:
static IDirect3DDevice9 *InitD3D(HWND hwnd)
{
    d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (!d3d9) {
        ::MessageBox(0, TEXT("Couldn't create Direct3D9 interface! Make sure DirectX 9 is installed."), 0, 0);
        return 0;
    }

    D3DCAPS9 caps;
    d3d9->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps);
    int vp = 0;
    if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) {
        vp = D3DCREATE_HARDWARE_VERTEXPROCESSING;
    } else {
        vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    }

    d3dpp.BackBufferWidth       = WINWIDTH;
    d3dpp.BackBufferHeight      = WINHEIGHT;
    d3dpp.BackBufferFormat      = D3DFMT_A8R8G8B8;
    d3dpp.BackBufferCount       = 1;
    d3dpp.MultiSampleType       = D3DMULTISAMPLE_NONE;
    d3dpp.MultiSampleQuality    = 0;
    d3dpp.SwapEffect            = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow         = hwnd;
    d3dpp.Windowed              = TRUE;
    d3dpp.EnableAutoDepthStencil= TRUE;
    d3dpp.AutoDepthStencilFormat= D3DFMT_D24S8;
    d3dpp.Flags                 = 0;
    d3dpp.FullScreen_RefreshRateInHz = D3DPRESENT_RATE_DEFAULT;
    d3dpp.PresentationInterval  = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9 *device = 0;

    UINT AdapterToUse=D3DADAPTER_DEFAULT;
    D3DDEVTYPE DeviceType=D3DDEVTYPE_HAL;

    HRESULT hr = d3d9->CreateDevice( AdapterToUse, DeviceType, hwnd, vp, &d3dpp, &device);

    if (FAILED(hr)) {
        ::MessageBox(0, TEXT("CreateDevice failed! Make sure your graphics drivers are up to date."), 0, 0);
        return 0;
    }

    return device;
}

// Be sure to destroy the Triton objects when done to prevent resource and memory leaks.
static void DestroyD3D()
{
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

    if (skyBox) {
        delete skyBox;
        skyBox = 0;
    }

    if (vertexBuffer) {
        vertexBuffer->Release();
    }

    if (indexBuffer) {
        indexBuffer->Release();
    }

    if (vertexDeclaration) {
        vertexDeclaration->Release();
    }

    if (device) {
        device->Release();
        device = 0;
    }

    if (d3d9) {
        d3d9->Release();
        d3d9 = 0;
    }
}

// Here we construct the view and projection matrices which are used to orient Triton's
// ocean as well as this demo's sky box.
static void SetModelviewProjectionMatrix(IDirect3DDevice9 *device, D3DXMATRIX *viewProj, D3DXMATRIX *world)
{
    // Create the camera's rotation matrix
    D3DXMATRIX Rot, Yaw, Pitch;
    D3DXMatrixRotationX(&Pitch, pitch);
    D3DXMatrixRotationY(&Yaw, yaw);
    D3DXMatrixMultiply(&Rot, &Yaw, &Pitch);

    // And the camera's translation
    D3DXMATRIX Pos;
    D3DXMatrixTranslation(&Pos, 0, -20, -80);

    // The world matrix is used to position the skybox in this demo
    *world = Pos;

    // Create the combined view matrix
    D3DXMATRIX view;
    D3DXMatrixMultiply(&view, &Pos, &Rot);

    device->SetTransform(D3DTS_VIEW, &view);

    // Set projection matrix.
    D3DVIEWPORT9 vp;
    device->GetViewport(&vp);

    D3DXMATRIX proj;
    D3DXMatrixPerspectiveFovRH(
        &proj,
        45.0 * (D3DX_PI / 180.0),
        (float)vp.Width / (float)vp.Height,
        10.0f,
        100000.0f);
    device->SetTransform(D3DTS_PROJECTION, &proj);

    D3DXMatrixMultiply(viewProj, &view, &proj);

    // Set view and projection matrices with Triton
    if (environment) {
        double pView[16], pProj[16];
        int i = 0;
        for (int row = 0; row < 4; row++) {
            for (int col = 0; col < 4; col++) {
                pView[i] = view(row, col);
                pProj[i] = proj(row, col);
                i++;
            }
        }

        environment->SetCameraMatrix(pView);
        environment->SetProjectionMatrix(pProj);
    }
}

// Handle a lost device
static void ResetDevice()
{
    if (device && ocean) {
        ocean->D3D9DeviceLost();

        device->Reset(&d3dpp);

        ocean->D3D9DeviceReset();
    }
}

// Main function to draw a frame of animation.
static void RenderFrame(HWND hWnd)
{
    HRESULT hr;
    static bool deviceLost = false;

    if (device) {
        // Deal with lost devices (monitor resolution change, user locked and unlocked the OS, etc)
        if (deviceLost) {
            if( FAILED( hr = device->TestCooperativeLevel() ) ) {
                if( hr == D3DERR_DEVICENOTRESET ) {
                    ResetDevice();
                    deviceLost = false;
                }
            }
        }

        // Compute the camera and projection matrices, and pass them into Triton:
        D3DXMATRIX viewProj, world;
        SetModelviewProjectionMatrix(device, &viewProj, &world);

        device->BeginScene();

        // Clear the depth buffer only - the skybox will clear the framebuffer
        device->Clear(0, 0, D3DCLEAR_ZBUFFER, D3DCOLOR_RGBA(0, 255, 0, 255), 1.0f, 0);

        if (ocean && environment) {
            // Sun position and color taken from the cube map used for the skybox
            Triton::Vector3 lightPosition(0, 1.0 / sqrt(2.0), 1.0 / sqrt(2.0));
            environment->SetDirectionalLight(lightPosition, Triton::Vector3(1.0, 1.0, 1.0));

            // Ambient color based on the zenith color in the cube map
            environment->SetAmbientLight(Triton::Vector3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0) * 1.5);

            // Use the same cube map used by our sky box for reflections. This is optional; without a cube
            // map Triton will just reflect the ambient color set above. However passing in a real
            // environment map to Triton makes things look a lot better.
            environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubeMap()));

            // Draw the water at the current time point
            static DWORD startMillis = 0;
            DWORD millis = timeGetTime();
            if (startMillis == 0) {
                startMillis = millis;
            }
            millis = millis - startMillis;
            double time = (double)millis * 0.001;

            // Set the simulated depth:
            Triton::Vector3 seaFloorNormal(0, 1, 0);
            ocean->SetDepth(10.0f, seaFloorNormal);

            // Set the proper depth test:
            device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

            // Set our cull mode and buffer:
            device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
            device->SetIndices(indexBuffer);
            device->SetVertexDeclaration(vertexDeclaration);
            device->SetRenderState(D3DRS_ZENABLE, TRUE);
            device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

            // Set up a model matrix to translate the patch
            double modelMatrix[16];
            int i = 0;
            D3DXMATRIX Pos;
            D3DXMatrixTranslation(&Pos, 0, 2, 0);
            for (int row = 0; row < 4; row++) for (int col = 0; col < 4; col++) {
                    modelMatrix[i++] = Pos(row, col);
                }

            // Set the simulated sea level to match the position of this patch
            environment->SetSeaLevel(2);

            // Update the ocean simulation once per frame; you could do this from
            // another thread if you want to.
            ocean->UpdateSimulation(time);

            // Have Triton set all of the state necessary to render our geometry as water:
            ocean->SetPatchShader(time, 16, 0, false, modelMatrix);

            device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));

            // Draw our mesh, shaded by Triton
            device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, nVerts,
                                         0, nIndices - 2);

            // Restore the previous state.
            ocean->UnsetPatchShader();

            // Continue to draw additional water patches in the same manner, just be
            // sure to call UpdateSimulation() only once per frame.
        }

        // Draw the sky box last to take advantage of early depth test
        skyBox->Draw(viewProj, world);

        device->EndScene();

        hr = device->Present(0, 0, 0, 0);
        if (hr == D3DERR_DEVICELOST) {
            deviceLost = true;
        }

        // Trigger another redraw.
        InvalidateRect(hWnd, NULL, FALSE);

    }
}

// Standard Windows main loop
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    MSG msg;

    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int) msg.wParam;
}

// Standard Windows class registration stuff:
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, L"DirectX9Sample.ico");
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, L"small.ico");

    return RegisterClassEx(&wcex);
}

// Set up our window and instantiate our sky box and Triton
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance;

    // Create our window
    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, WINWIDTH, WINHEIGHT, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    // Create our Direct3D device
    device = InitD3D(hWnd);
    if (device) {

        // Create the Triton objects
        if (!InitTriton()) {
            return FALSE;
        }

        // Create our mesh that Triton will shade
        InitPatch();

        // Create our sky box
        skyBox = new SkyBox(device);
        skyBox->Create();

    } else {
        return FALSE;
    }

    // Unveil our window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//  Processes messages for the main window.
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
    case WM_COMMAND:
        // Handle standard commands
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    case WM_PAINT:
        // Draw a frame of animation
        hdc = BeginPaint(hWnd, &ps);
        RenderFrame(hWnd);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        // Clean things up and quit
        DestroyD3D();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
