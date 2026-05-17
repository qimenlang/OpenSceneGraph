// Triton DirectX 9 Sample Application
// Illustrates integration of Triton with a CubeMap-based sky box in a simple Win32 app
// Copyright (c) 2011-2016 Sundog Software, LLC. All rights reserved worldwide.

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
static float aspectRatio, yaw = 0, pitch = 0.2f;

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

    // Add wind at 15 meters per second blowing roughly from the North
    Triton::WindFetch wf;
    wf.SetWind(10.0, 0.3);
    environment->AddWindFetch(wf);

    // Create the Ocean itself using the environment we've created, and FFT waves.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    ocean = Triton::Ocean::Create(environment, Triton::JONSWAP, false, false, Triton::GOOD);

    if (ocean) {
        // Disabling spray can help performance.
        ocean->EnableSpray(true);
    }

    return (ocean != NULL);
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
    D3DXMatrixTranslation(&Pos, 0, -15, 0);

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

            Triton::Vector3 ambient = Triton::Vector3(0.5, 0.5, 0.5);
            environment->SetAmbientLight(ambient);

            // Use the same cube map used by our sky box for reflections. This is optional; without a cube
            // map Triton will just reflect the ambient color set above. However passing in a real
            // environment map to Triton makes things look a lot better.
            Triton::Matrix3 flip(1.0, 0.0, 0.0,    0.0, -1.0, 0.0,   0.0, 0.0, 1.0);
            environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubeMap()), flip);

            // Set the proper depth function:
            device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);

            // Draw the ocean at the current time point
            static DWORD startMillis = 0;
            DWORD millis = timeGetTime();
            if (startMillis == 0) {
                startMillis = millis;
            }
            millis = millis - startMillis;
            ocean->Draw((double)millis * 0.001);
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
