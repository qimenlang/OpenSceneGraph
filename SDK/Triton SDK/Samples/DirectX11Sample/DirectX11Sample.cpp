// DirectX11 Sample Application for Triton
// Copyright (c) 2011-2016 Sundog Software, LLC. All rights reserved worldwide.

#include "stdafx.h"
#include "../SamplesCommon/Licenses.h"
#include "DirectX11Sample.h"

using namespace Triton;

ID3D11Debug *d3dDebug = 0;
static bool debugDXResourceLeaks = false;

// The 3 main Triton objects you need:
static Triton::Environment *environment = 0;
static Triton::Ocean *ocean = 0;
static Triton::ResourceLoader *resourceLoader = 0;

// A separate class that renders the sample's sky box and provides
// us with our cube map for environment mapping with Triton
static SkyBox *skyBox = 0;

// Camera properties
static float aspectRatio, yaw = 0;

// DX11 stuff:
static ID3D11Device *device = 0;
static ID3D11DeviceContext *deviceContext = 0;
static IDXGISwapChain *swapChain = 0;

static ID3D11Texture2D *depthStencilBuffer = 0;
static  ID3D11DepthStencilView *depthStencilView = 0;
static ID3D11RenderTargetView *renderTargetView = 0;

// Windows stuff:
#define MAX_LOADSTRING 100
#define WINWIDTH 800
#define WINHEIGHT 600
HINSTANCE hInst;                                // current instance
TCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

// Create Triton's resources:
bool InitTriton()
{
    // We use the default resource loader here that just reads files from your redistributed resources
    // folder, but you could extend the ResourceLoader class to hook into your own resource manager as well
    resourceLoader = new Triton::ResourceLoader("..\\..\\resources\\");

    // Create our environment
    environment = new Triton::Environment();

    // Initialize a flat-earth Y-is-up DirectX11 environment for Triton with our resource loader
    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP, Triton::DIRECTX_11,
                                   resourceLoader, device);

    if (err != Triton::SUCCEEDED) {
        ::MessageBoxA(NULL, "Failed to initialize Triton - is the resource path passed in to "
                      "the ResouceLoader constructor valid?", "Triton error", MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    // Substitute your own license name and code here, or the app will terminate after 5 minutes. Visit
    // www.sundog-soft.com to purchase a license if you're so inclined.
    environment->SetLicenseCode(TRITON_LICENSE_NAME, TRITON_LICENSE_CODE);

    // Add wind at 10 m/s
    Triton::WindFetch wf;
    wf.SetWind(15.0, 0.2);
    environment->AddWindFetch(wf);

    // Create an FFT-based ocean using the environment we created.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    ocean = Triton::Ocean::Create(environment, Triton::JONSWAP, false, false, Triton::GOOD);

    if (ocean) {
        // Disabling spray will help performance
        ocean->EnableSpray(true);
    }

    return (ocean != NULL);
}

// Compute the modelview, projection, and skybox matrices - and pass the camera matrices into Triton so it can
// render the ocean consistent with your camera
static void SetModelviewProjectionMatrix(ID3D11Device *device, float timeDelta, D3DXMATRIX *skyBoxViewProj)
{
    UNREFERENCED_PARAMETER(device);
    UNREFERENCED_PARAMETER(timeDelta);
    // Spin the camera around
    //float inc = timeDelta * 0.1f;
    //yaw += inc;

    // Create our view matrix
    D3DXMATRIX Rot, Yaw, Pitch;
    D3DXMatrixRotationX(&Pitch, 0);
    D3DXMatrixRotationY(&Yaw, yaw);
    D3DXMatrixMultiply(&Rot, &Yaw, &Pitch);
    D3DXMATRIX Pos;
    D3DXMatrixTranslation(&Pos, 0, -15, 0);
    D3DXMATRIX View;
    D3DXMatrixMultiply(&View, &Pos, &Rot);

    // Create our projection matrix.
    UINT nViewports = 1;
    D3D11_VIEWPORT vp;
    deviceContext->RSGetViewports(&nViewports, &vp);

    D3DXMATRIX Proj;
    D3DXMatrixPerspectiveFovRH(
        &Proj,
        45.0f * (D3DX_PI / 180.0f),
        (float)vp.Width / (float)vp.Height,
        2.0f,
        100000.0f);

    // Compute the combined matrix for the skybox while we're here.
    D3DXMatrixMultiply(skyBoxViewProj, &Rot, &Proj);

    // Now, feed the camera matrices into Triton:
    double pView[16], pProj[16];
    int i = 0;
    for (int row = 0; row < 4; row++) {
        for (int col = 0; col < 4; col++) {
            pView[i] = View(row, col);
            pProj[i] = Proj(row, col);
            i++;
        }
    }
    environment->SetCameraMatrix(pView);
    environment->SetProjectionMatrix(pProj);
}

// Main function to draw a frame of animation.
static void RenderFrame(HWND hWnd)
{
    static float lastTime = (float)timeGetTime();

    if (device) {
        float currTime = (float)timeGetTime();
        float timeDelta = (currTime - lastTime) * 0.001f;

        // Compute the camera matrices and pass them into Triton:
        D3DXMATRIX skyBoxViewProj;
        SetModelviewProjectionMatrix(device, timeDelta, &skyBoxViewProj);

        // Clear the depth buffer
        deviceContext->ClearDepthStencilView(depthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0);

        // Light and draw the ocean:
        if (ocean && environment) {
            // Sun position and color taken from the cube map used for the skybox
            Triton::Vector3 lightPosition(0, 1.0 / sqrt(2.0), 1.0 / sqrt(2.0));
            environment->SetDirectionalLight(lightPosition, Triton::Vector3(1.0, 1.0, 1.0));

            // Ambient color based on the zenith color in the cube map
            environment->SetAmbientLight(Triton::Vector3(0.5, 0.5, 0.5));

            // Grab the same cube map used for rendering the sky, and give it to Triton for creating
            // reflections:
            Triton::Matrix3 flip(1.0, 0.0, 0.0,    0.0, -1.0, 0.0,   0.0, 0.0, 1.0);
            environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubeMap()), flip);

            // Draw the ocean for the current time sample:
            static DWORD startMillis = 0;
            DWORD millis = timeGetTime();
            if (startMillis == 0) {
                startMillis = millis;
            }
            millis = millis - startMillis;
            ocean->Draw((double)millis * 0.001);
        }

        // Draw the skybox last:
        skyBox->Draw(skyBoxViewProj);

        // Present our frame:
        swapChain->Present(0, 0);

        // Trigger another redraw.
        InvalidateRect(hWnd, NULL, FALSE);

        lastTime = currTime;
    }
}

// Clean up our resources.
static void DestroyD3D()
{
    if (skyBox) {
        delete skyBox;
        skyBox = 0;
    }

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

    if (depthStencilBuffer) {
        depthStencilBuffer->Release();
    }
    if (depthStencilView) {
        depthStencilView->Release();
    }

    if (renderTargetView) {
        renderTargetView->Release();
    }

    if (swapChain) {
        swapChain->Release();
    }

    deviceContext->ClearState();
    deviceContext->Flush();
    deviceContext->Release();
    deviceContext = 0;

    if (device) {
        device->Release();
        device = 0;
    }

    if (debugDXResourceLeaks) {
        d3dDebug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);
        d3dDebug->Release();
        d3dDebug = 0;
    }
}

// Standard DX11 initialization:
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

    if (debugDXResourceLeaks) {
        createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }

    hr = D3D11CreateDeviceAndSwapChain(0, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags,
                                       NULL, 0, D3D11_SDK_VERSION, &sd, &swapChain, &device, NULL, &deviceContext);

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

    deviceContext->OMSetRenderTargets(1, &renderTargetView, depthStencilView);

    D3D11_VIEWPORT vp;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = WINWIDTH;
    vp.Height = WINHEIGHT;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;

    deviceContext->RSSetViewports(1, &vp);

    if (debugDXResourceLeaks) {
        device->QueryInterface(__uuidof(ID3D11Debug), (void**)&d3dDebug);
    }

    return device;
}

// Standard main message loop
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
    LoadString(hInstance, IDC_DIRECTX11SAMPLE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DIRECTX11SAMPLE));

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
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DIRECTX11SAMPLE));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_DIRECTX11SAMPLE);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

// Set up our window, DX11, Triton, and our Sky Box:
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance; // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd) {
        return FALSE;
    }

    // Create DX11
    device = InitD3D(hWnd);
    if (device) {
        // If successful, set up Triton
        if (!InitTriton()) {
            return FALSE;
        }
    } else {
        return FALSE;
    }

    // Create our sky box
    skyBox = new SkyBox(device);
    if (!skyBox->Create()) {
        return FALSE;
    }

    // Unveil our window
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

// Standard Windows message processing
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

        // Our drawing function:
        RenderFrame(hWnd);

        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        // Our cleanup function:
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
