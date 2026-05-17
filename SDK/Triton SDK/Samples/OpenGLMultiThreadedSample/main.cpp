// Copyright (c) 2011-2025 Sundog Software LLC. All rights reserved worldwide.

// Triton OpenGL Multi-Threaded Sample Project
// Illustrates multi-threaded/multiple views rendering in Triton
// There are 4 different 'Draw Strategies'. Please look at enum DrawStrategy in SampleDeclares.h
// to see how each strategy does the rendering

// The Triton.h header includes everything you need
#include "../SamplesCommon/Licenses.h"

// glew stuff
#include "GL/glew.h"
// Various platform-specific headers:
#if defined(__APPLE__)
#include "GLFW/glfw3.h"
# include <sys/timeb.h>
#elif defined(_WIN32)
# include <windows.h>
#include "GLFW/glfw3.h"
# ifndef WGL_EXT_swap_control
typedef int (APIENTRY * PFNWGLSWAPINTERVALEXTPROC)(int);
# endif
#else
#include "GLFW/glfw3.h"
# include <GL/glx.h>
# include <sys/timeb.h>
# ifndef GLX_SGI_swap_control
typedef int(*PFNGLXSWAPINTERVALSGIPROC) (int interval);
# endif
#endif

// Triton includes
#include "Triton.h"
#include "Camera.h"

// Sample includes
#include "skybox.h"
#include "SampleDeclares.h"
#include "QuadRendering.h"
#include "matrixutils.h"

#include <fstream>
#include <sstream>

#undef NDEBUG
#include <cassert>

Triton::Environment *environment = 0;
Triton::Ocean *ocean = 0;
Triton::ResourceLoader *resourceLoader = 0;

// used for multi-threaded, multi context path
GLFWwindow* small_view_offscreen_window[numViews];

double yaw = 0;
double pitch = 0;
double roll = 0;

// wakes
Triton::WakeGenerator *ship = 0;
Triton::Vector3 shipPosition;

// rotor wash
Triton::RotorWash *rotorWash = 0;
Triton::Vector3 rotorPosition(0, 10, 50);
Triton::Vector3 rotorDirection(0, -1, 0);

#if TEST_UNDERWATER
Triton::Vector3 cameraPosition(0, -10, 150);
#else
Triton::Vector3 cameraPosition(0, 30, 150);
#endif

bool testGodRays = false;

static long int frame = 0;

SkyBox *skyBox = 0;

bool quit = false;

using namespace Triton;

RenderToTexture *rtt_textures[numViews] = { 0, 0 };
Stream* rtt_stream[numViews + 1] = { 0, 0, 0 };

Camera* cameras[numViews + 1];

void MoveShip(double millis)
{
    const double shipVelocity = 30.0; // about 10 knots
    const double shipRadius = 200.0;

    double circumference = 2.0 * TRITON_PI * 2.0 * shipRadius;
    double circuitTime = circumference / shipVelocity;

    double nowMS = (double)millis;
    double nowS = nowMS * 0.001;
    static double startS = 0;
    if (startS == 0) startS = nowS;

    shipPosition.y = 0;

    static double lastX = 0, lastZ = 0;
    //shipX = shipRadius * cos(t);
    //shipZ = shipRadius * sin(t);
    shipPosition.z = (startS - nowS) * 50.0;

    Triton::Vector3 direction(shipPosition.x - lastX, 0, shipPosition.z - lastZ);
    direction.Normalize();
    lastX = shipPosition.x;
    lastZ = shipPosition.z;

    ship->Update(shipPosition, direction, shipVelocity, nowS);
}

// Forward declarations
unsigned long getMilliseconds();
void SetVSync(int interval);

void DrawImmediateSingleThreaded(double millis, int frame);
void DrawDeferredSingleThreaded(double millis, int frame);
namespace single_context
{
void DrawDeferredMultiThreadedSingleContext(double millis, int frame);
void InitializeThreads();
void InitializeRTTStuff();
void QuitFromAllThreads(long int frame);
}
namespace multi_contexts
{
void DrawDeferredMultiThreadedMultipleContexts(double millis, int frame);
void InitializeRTTStuff();
void InitializeThreads();
void QuitFromAllThreads(long int frame);
}

static int cubeMapIndex = 0;
bool cycleCubeMap = false;
void nextCubeMap(void)
{
    assert(cycleCubeMap == true);
    ++cubeMapIndex;
    if (cubeMapIndex > 9) cubeMapIndex = 1;

    std::stringstream ss;

    ss << "0" << cubeMapIndex;

#ifdef _WIN32
    std::string strDir = "..\\media\\skyboxes\\" + ss.str() + "\\";
#else
    std::string strDir = "../media/skyboxes/" + ss.str() + "/";
#endif
    skyBox->SetCubeMap(strDir.c_str());
    environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubemap()));
    cycleCubeMap = false;
}

GLFWwindow* main_window_and_context = 0;

// Create the Triton objects at startup, once we have a valid GL context in place
bool InitTriton(Triton::Renderer renderer)
{
#if defined(WIN32) || defined(WIN64)
    resourceLoader = new Triton::ResourceLoader("..\\..\\Resources\\");
#else
    resourceLoader = new Triton::ResourceLoader("../../Resources/");
#endif

    environment = new Triton::Environment();

    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP,
                                   renderer, resourceLoader);

    if (err != Triton::SUCCEEDED) {
        printf("Couldn't initialize Triton");
        return false;
    }

    environment->SetLicenseCode(TRITON_LICENSE_NAME, TRITON_LICENSE_CODE);

    // Force config settings required for multi-threaded support
    // NOTE you will need to ensure any GL state for your app is saved and restored
    // surrounding any Triton drawing; setting avoid-opengl-stalls disables
    // Triton doing this on its own.
    environment->SetConfigOption("avoid-opengl-stalls", "yes");
    environment->SetConfigOption("fft-texture-update-frame-delayed", "yes");

    environment->SimulateSeaState(3.0, 0.0, true);

    ocean = Triton::Ocean::Create(environment, Triton::TESSENDORF, true, true);

    if (ocean) {
        printf("Using FFT Technique: %s\n", ocean->GetFFTName());
    }

    //ocean->EnableWireframe(true);
    //ocean->SetDepth(1.0, Triton::Vector3(0.0, 1.0, 0.0));
    ocean->EnableSpray(true);

#if TEST_UNDERWATER
    ocean->EnableGodRays(true);
#endif

    Triton::WakeGeneratorParameters params;
    params.sprayEffects = true;
    params.numHullSprays = 5;
    ship = new Triton::WakeGenerator(ocean, params);

    rotorWash = new Triton::RotorWash(ocean, 30.0, true, true);

    return (ocean != NULL);
}

void CalculateSkyBoxMatrices(double mv[16], double mv0[16], double mv1[16], double proj[16], double skyBoxMatrix[16])
{
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    double aspect = (double)vp[2] / (double)vp[3];

    // Compute a perspective projection matrix
    BuildPerspProjMat(proj, 45.0f, aspect, 2.0, 200000.0f);

    // Spin the camera to face the sun in our skybox texture
    BuildIdentityMat(mv);
    // ApplyYaw(mv, yaw);
    EulerMatrix(mv, yaw, pitch, roll);

    BuildIdentityMat(mv0);
    EulerMatrix(mv0, yaw, pitch + 0.3, roll); // tilted by 0.3 radians

    BuildIdentityMat(mv1);
    EulerMatrix(mv1, yaw, pitch - 0.3, roll); // tilted by 0.3 radians

    // Capture the viewproj matrix before we apply translation, for use
    // in positioning our sky box:
    Multiply(skyBoxMatrix, mv, proj);

    // Apply the camera translation
    Translate(mv, -cameraPosition.x, -cameraPosition.y, -cameraPosition.z);

    Translate(mv0, -cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
    Translate(mv1, -cameraPosition.x, -cameraPosition.y, -cameraPosition.z);
}


// Compute the matrices for the camera and for rendering our sky box, and
// pass them into Triton so it can properly position and orient the water.
// We compute the projection and view matrices "by hand" to make this sample
// more useful for OpenGL 3.2+ developers.
void SetUpTritonFromMatrices(double mv[16], double mv0[16], double mv1[16], double proj[16])
{
    if (environment == 0) {
        return;
    }

    // Pass the final view and projection matrices into Triton.
    environment->SetCameraMatrix(mv);
    environment->SetProjectionMatrix(proj);

    for (int i = 0; i < numViews; ++i) {
        if (cameras[i]) {
            if (i==0) {
                cameras[i]->SetCameraMatrix(mv0);
            }
            if (i==1) {
                cameras[i]->SetCameraMatrix(mv1);
            }

            cameras[i]->SetProjectionMatrix(proj);
        }
    }

    Triton::Vector3 lightDirection(0.9539f, 0.16765f, -0.24861f);
    Triton::Vector3 lightColor(1.0000000f, 0.93333638f, 0.76101780f);
    environment->SetDirectionalLight(lightDirection, lightColor);

    Triton::Vector3 ambientColor(0.8644f, 0.8644f, 0.8644f);
    environment->SetAmbientLight(ambientColor);

    // Grab the cube map from our sky box and give it to Triton to use as an environment map
    environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubemap()));

#if TEST_UNDERWATER
    float r = 84.f / 255.f, g = 135.f / 255.f, b = 172.f / 255.f;
    environment->SetBelowWaterVisibility(1000, Triton::Vector3(r, g, b));
#endif
}

GLuint quadVB = 0, quadIB = 0;
GLuint quadProgram;

void InitQuadStuff()
{
    CreateQuadProgram(quadProgram);
    createQuadVB(quadVB);
    createQuadIB(quadIB);
}

void clearBasedOnCamera(Triton::Camera* camera)
{
#if TEST_UNDERWATER
    float r = 84.f / 255.f, g = 135.f / 255.f, b = 172.f / 255.f;
    glClearColor(r, g, b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    return;
#endif
    // Clear the depth buffer
    if (ocean->IsCameraAboveWater(camera)) {
        glClearDepth(1.0);
        glClear(GL_DEPTH_BUFFER_BIT);
    } else {
        glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void RenderQuad(int x, int y, int width, int height, int view)
{
    GLint window_params_index = glGetUniformLocation(quadProgram, "window_params");
    GLint viewport_width_index = glGetUniformLocation(quadProgram, "viewport_width");
    GLint viewport_height_index = glGetUniformLocation(quadProgram, "viewport_height");
    GLint tex_index = glGetUniformLocation(quadProgram, "tex");

    //assert(window_params_index!=-1&&viewport_width_index!=-1&&viewport_height_index!=-1);

    RenderToTexture* rtt = rtt_textures[view];

    glActiveTexture(GL_TEXTURE0);
    rtt->bindColorTex();

    glUseProgram(quadProgram);

    glUniform4f(window_params_index, (float)x, (float)y, (float)width, (float)height);
    glUniform1f(viewport_width_index, (float)windowWidth);
    glUniform1f(viewport_height_index, (float)windowHeight);
    glUniform1i(tex_index, 0);

    glBindBuffer(GL_ARRAY_BUFFER, quadVB);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quadIB);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, false, 0, 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, NULL);

    glDisableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glUseProgram(0);

    rtt->unbindColorTex();
}

void _render_small_view_begin(int view, double millis)
{
    rtt_textures[view]->bind();

    glClearColor(0.0f, 0.1f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // Draw the ocean for the current time sample
    // Ensure proper sorting of the waves against each other:
    glDepthFunc(GL_LEQUAL);
}

void _render_small_view_end(int view, double millis)
{
    rtt_textures[view]->unbind();
}

// Draw a frame of animation including the Triton ocean and our skybox.
void DrawFrame(long int frame)
{
    static unsigned long startTime = 0, frameCount = 0;

    // Update our frame timer; output average framerate every 200 frames.
    if (startTime == 0) {
        startTime = getMilliseconds();
    }
    frameCount++;
    if (frameCount % 200 == 0) {
        long elapsed = getMilliseconds() - startTime;
        float fps = (float)frameCount / (float)elapsed * 1000.0f;
        printf("Average FPS: %.2f\n", fps);
    }

#ifdef _WIN32
    static DWORD startMillis = 0;
    DWORD millis = timeGetTime();
    if (startMillis == 0) {
        startMillis = millis;
    }
    millis = millis - startMillis;
#else
    unsigned long millis;
    struct timeb tp;
    ftime(&tp);
    static unsigned long startMillis = 0;
    millis = tp.time * 1000 + tp.millitm;
    millis = millis - startMillis;
    if (startMillis == 0) {
        startMillis = millis;
        millis = 0;
    }
#endif

    MoveShip(millis);

    if (rotorWash) {
        double nowMS = (double)millis;
        double nowS = nowMS * 0.001;
        rotorWash->Update(rotorPosition, rotorDirection, 44.0, nowS);
    }

    if (drawStrategy == DRAW_IMMEDIATE_SINGLE_THREADED) {
        DrawImmediateSingleThreaded(millis, frame);
    } else if (drawStrategy == DRAW_DEFERRED_SINGLE_THREADED) {
        DrawDeferredSingleThreaded(millis, frame);
    } else if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT) {
        single_context::DrawDeferredMultiThreadedSingleContext(millis, frame);
    } else if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        multi_contexts::DrawDeferredMultiThreadedMultipleContexts(millis, frame);
    }
}

// Handle a window resize
void Resize(GLFWwindow* window, GLint width, GLint height)
{
    glViewport(0, 0, width, height);
}

// Clean up our resources
void Destroy()
{
    if (ship) {
        delete ship;
        ship = 0;
    }

    if (rotorWash) {
        delete rotorWash;
        rotorWash = 0;
    }

    if (ocean) delete ocean;
    if (environment) delete environment;
    if (resourceLoader) delete resourceLoader;
    if (skyBox) delete skyBox;

    for (int i = 0; i < numViews; ++i) {
        delete rtt_textures[i];
    }
}

////////////////////////////////////////////////////////////////////////////////
void Keyboard(GLFWwindow* window, int key, int, int action, int)
{
    switch (key) {
    case 27:
        if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT) {
            single_context::QuitFromAllThreads(frame);
        }
        if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
            multi_contexts::QuitFromAllThreads(frame);
        }

        exit(0);
        break;

    case 'd':
    case 'D':
        pitch += 0.01;
        break;

    case 'A':
    case 'a':
        pitch -= 0.01;
        break;

    case 'w':
    case 'W':
        roll += 0.01;
        break;

    case 's':
    case 'S':
        roll -= 0.01;
        break;

    case 'q':
    case 'Q':
        yaw += 0.01;
        break;

    case 'e':
    case 'E':
        yaw -= 0.01;
        break;

    case 'r':
    case 'R':
        //y += 10.0;
        break;

    case 'f':
    case 'F':
        //y -= 10.0;
        break;

    case 'c':
    case 'C':
        if (action == GLFW_PRESS) {
            cycleCubeMap = true;
        }
        break;
    default:
        break;
    }
}

bool filterMessage(const GLchar* message)
{
    std::string strMessage(message);
    if (strMessage.find("Buffer detailed info") != std::string::npos) {
        return true;
    }
    return false;
}

class OpenGLDebugger
{
public:
    virtual void onDebugMessage(GLenum source,
                                GLenum type,
                                GLuint id,
                                GLenum severity,
                                GLsizei length,
                                const GLchar* message) {
        if (filterMessage(message)) {
            return;
        }

        std::cout << message << std::endl;
#ifdef _WIN32
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
#endif
    }
};

void APIENTRY debug_callback(GLenum source,
                             GLenum type,
                             GLuint id,
                             GLenum severity,
                             GLsizei length,
                             const GLchar* message,
                             GLvoid* userParam)
{
    reinterpret_cast<OpenGLDebugger*>(userParam)->onDebugMessage(source, type, id, severity, length, message);
}


Triton::Renderer toTritonRenderer(int glMajor, int glMinor)
{
    if (glMajor == 4) {
        if (glMinor == 5) {
            std::cout << "Triton Renderer is: " << "Triton::OPENGL_4_5" << std::endl;
            return Triton::OPENGL_4_5;
        }
        if (glMinor == 1) {
            std::cout << "Triton Renderer is: " << "Triton::OPENGL_4_1" << std::endl;
            return Triton::OPENGL_4_1;
        }
        if (glMinor == 0) {
            std::cout << "Triton Renderer is: " << "Triton::OPENGL_4_0" << std::endl;
            return Triton::OPENGL_4_0;
        }
    }
    assert(false);
    return Triton::OPENGL_2_0;
}

static OpenGLDebugger opengGLDebugger;
static const bool debugOpenGL = true;

static int OpenGL_Major = 4;
static int OpenGL_Minor = 5;


////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    if (!glfwInit()) {
        std::cerr << "Unable to initialize glfw!" << std::endl;
        return 0;
    }

    // You can explicitly set up the context version like so
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OpenGL_Major);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OpenGL_Minor);

    // For debugging.
    if (debugOpenGL) {
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
    }

    // GLFW_OPENGL_CORE_PROFILE   -> deprecated calls will not work
    // GLFW_OPENGL_COMPAT_PROFILE -> deprecated calls will work
    // Nothing specified -> deprecated calls do not work
#if 0
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#endif

    // As per this: https://www.khronos.org/opengl/wiki/Core_And_Compatibility_in_Contexts
    // "Recommendation: You SHOULD NEVER use the forward compatibility bit. It had a use for GL 3.0, but once 3.1 removed most of the stuff, it stopped having a use."
#if 0
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Set up multi-sampling like so
    //glfwWindowHint(GLFW_SAMPLES, info.samples);

    // Set up stereo rendering like so
    //glfwWindowHint(GLFW_STEREO, info.flags.stereo ? GL_TRUE : GL_FALSE);

    // lose the context on reset?
    //glfwWindowHint(GLFW_CONTEXT_ROBUSTNESS, GLFW_LOSE_CONTEXT_ON_RESET);

    main_window_and_context = glfwCreateWindow(windowWidth, windowHeight, "Triton OpenGL Multi Threaded/Multiple Views Sample", NULL, NULL);
    if (!main_window_and_context) {
        std::cerr << "Failed to create GLFWwindow!" << std::endl;
        return 0;
    }

    glfwMakeContextCurrent(main_window_and_context);
    int32_t major = glfwGetWindowAttrib(main_window_and_context, GLFW_CONTEXT_VERSION_MAJOR);
    int32_t minor = glfwGetWindowAttrib(main_window_and_context, GLFW_CONTEXT_VERSION_MINOR);

    std::cout << "OpenGL version that GLFW set up is: OpenGL " << major << "." << minor << std::endl;

    glfwSetWindowSizeCallback(main_window_and_context, Resize);
    glfwSetKeyCallback(main_window_and_context, Keyboard);

    // We use GLEW mainly for the benefit of our SkyBox class
    glewInit();

    // This is useful only if we use glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE); above
    if (debugOpenGL) {
        glDebugMessageCallback((GLDEBUGPROC)debug_callback, &opengGLDebugger);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    }

    // Enable vertical sync if specified
    if (argc > 1 && strcmp(argv[1], "vsync") == 0) {
        SetVSync(1);
    } else {
        SetVSync(0);
    }

    InitQuadStuff();
    if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        // Create the offscreen windows, which will also create the actual
        // separate OpenGL context
        for (int i = 0; i < numViews; ++i) {
            glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
            small_view_offscreen_window[i] = glfwCreateWindow(view_width, view_height, "small_window", 0, main_window_and_context);
        }
    }

    Triton::Renderer renderer = toTritonRenderer(OpenGL_Major, OpenGL_Minor);
    // Set up Triton's resources
    if (!InitTriton(renderer)) {
        return 0;
    }

    for (int i = 0; i < numViews; ++i) {
        cameras[i] = environment->CreateCamera();
        cameras[i]->SetViewport(0, 0, view_width, view_height);
        std::stringstream ss;
        ss << "cam_view_" << i;
        cameras[i]->SetName(ss.str().c_str());
    }

    cameras[numViews] = environment->GetCamera();
    cameras[numViews]->SetName("Main View");
    cameras[numViews]->SetViewport(0, 0, windowWidth, windowHeight);
    int mainRT = 0;
    cameras[numViews]->SetRenderTarget((void*)mainRT);

    // Create our sky box to render behind Triton's ocean and provide
    // its environment map
    skyBox = new SkyBox();
    skyBox->Create();
    cycleCubeMap = true;
    nextCubeMap();

    bool running = true;

    // For all other strategies other than multiple context strategy, create the rtts
    if (drawStrategy != DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        for (int i = 0; i < numViews; ++i) {
            // These rtts' fbos belong the main window context
            rtt_textures[i] = new RenderToTexture(view_width, view_height);
            cameras[i]->SetRenderTarget((void*)(rtt_textures[i]->GetFBO()));
        }
    } else if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        for (int i = 0; i < numViews; ++i) {
            glfwMakeContextCurrent(small_view_offscreen_window[i]);
            // These rtts' fbos belong the small view window context that was made current in the previous line
            rtt_textures[i] = new RenderToTexture(view_width, view_height);
            cameras[i]->SetRenderTarget((void*)(rtt_textures[i]->GetFBO()));
            rtt_stream[i] = environment->CreateOpenGLStream();
            ocean->Initialize(rtt_stream[i], cameras[i]);
        }
        // reset the main context
        glfwMakeContextCurrent(main_window_and_context);
    }

    if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT) {
        single_context::InitializeThreads();
    } else if (drawStrategy==DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        multi_contexts::InitializeThreads();
    }

    do {
        DrawFrame(frame++);

        glfwSwapBuffers(main_window_and_context);
        glfwPollEvents();

        running &= (glfwGetKey(main_window_and_context, GLFW_KEY_ESCAPE) == GLFW_RELEASE);
        running &= (glfwWindowShouldClose(main_window_and_context) != GL_TRUE);

        if (cycleCubeMap) {
            nextCubeMap();
        }
    } while (running);

    if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT) {
        single_context::QuitFromAllThreads(frame);
    }
    if (drawStrategy == DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT) {
        multi_contexts::QuitFromAllThreads(frame);
    }

    Destroy();

    glfwDestroyWindow(main_window_and_context);
    glfwTerminate();

    return 0;
}

unsigned long getMilliseconds()
{
#ifdef _WIN32
    return timeGetTime();
#else
    struct timeb tp;
    ftime(&tp);
    static time_t startTime = 0;
    if (startTime == 0) startTime = tp.time;
    return (((tp.time - startTime) * 1000) + tp.millitm);
#endif
}

// Enable / disable vsync for framerate measurements (use the novsync command line argument.)
void SetVSync(int interval)
{
    printf("Disabling vsync...\n");
#if defined(__APPLE__)
    return;
#elif defined(_WIN32)
    PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
        (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(interval);
    }
#else
    PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
        (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
    if (glXSwapIntervalSGI) {
        glXSwapIntervalSGI(interval);
    }
#endif
}
