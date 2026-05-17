// Triton OpenGL Patch Sample Project
// Illustrates integration of Triton with an OpenGL-based application, where Triton
// shades a patch of user-drawn geometry to turn it into water.

// Copyright (c) 2012-2025 Sundog Software LLC. All rights reserved worldwide.

#include "../SamplesCommon/Licenses.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/timeb.h>
#endif

// The Triton.h header includes everything you need
#include "Triton.h"

#include "GL/glew.h"
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include "GL/freeglut.h"
#endif

#include <math.h>
#include <cassert>
#include <sstream>
#include "skybox.h"

// The three main Triton objects you need:
static Triton::ResourceLoader *resourceLoader = 0;
static Triton::Environment *environment = 0;
static Triton::Ocean *ocean = 0;

// And as a test we will also add a rotor wash effect
Triton::RotorWash* rotorWash = 0;

// Our simple skybox class that exposes an environment map, which
// Triton can use for water reflections.
static SkyBox *skyBox = 0;

// Camera properties:
static double yaw = 2 * TRITON_PI, pitch = 0, roll = 0;
static double x = 0, y = 10, z = 20;

// Function prototypes from matrixutils.cpp:
extern void BuildPerspProjMat(double *m, double fov, double aspect, double znear, double zfar);
extern void BuildIdentityMat(double *m);
extern void Translate( GLdouble *m, GLdouble x, GLdouble y, GLdouble z );
extern void Multiply( GLdouble *mOut, GLdouble *m1, GLdouble *m2);
extern void ApplyYaw( GLdouble *m, GLdouble yaw);
extern void EulerMatrix( double m[16], double yaw, double pitch, double roll );

// Create the Triton objects at startup, once we have a valid GL context in place
bool InitTriton()
{
    // We use the default resource loader that just loads files from disk. You'll need
    // to redistribute the resources folder if using this. You can also extend the
    // ResourceLoader class to hook into your own resource manager if you wish.
#if defined(WIN32) || defined(WIN64)
    resourceLoader = new Triton::ResourceLoader("..\\..\\Resources\\");
#else
    resourceLoader = new Triton::ResourceLoader("../../Resources/");
#endif
    // Create an environment for the water, with a flat-Earth coordinate system with Y
    // pointing up and using an OpenGL 2.0 capable context.
    environment = new Triton::Environment();
    Triton::EnvironmentError err = environment->Initialize(Triton::FLAT_YUP,
                                   Triton::OPENGL_2_0, resourceLoader);

    if (err != Triton::SUCCEEDED) {
#ifdef _WIN32
        ::MessageBoxA(NULL, "Failed to initialize Triton - is the resource path passed in to "
                      "the ResouceLoader constructor valid?", "Triton error", MB_OK | MB_ICONEXCLAMATION);
#else
        printf("Failed to initialize Triton.\n");
#endif
        return false;
    }

    // Substitute your own license name and code, otherwise the app will terminate after
    // 5 minutes. Visit www.sundog-soft.com to purchase a license if you're so inclined.
    environment->SetLicenseCode(TRITON_LICENSE_NAME, TRITON_LICENSE_CODE);

    // Set up wind of 10 m/s blowing North
    Triton::WindFetch wf;
    wf.SetWind(20.0, 0.0);
    environment->AddWindFetch(wf);

    // Finally, create the Ocean object using the environment we've created.
    // If NULL is returned, something's wrong - enable the enable-debug-messages option
    // in resources/triton.config to get more details on the problem.
    ocean = Triton::Ocean::Create(environment, Triton::JONSWAP);

    if (ocean) {
        printf("Using FFT Technique: %s\n", ocean->GetFFTName());
    }

    // Create a rotor wash effect and enable spray effects from waves
    rotorWash = new Triton::RotorWash(ocean, 10.0, true, true);
    ocean->EnableSpray(true);

    return (ocean != NULL);
}

/** Data associated with our 50x50 vertex mesh that we'll have Triton shade as water. */
GLuint vboID, idxID, vertexArray;
unsigned int gridResolution = 50;
int nIndices, nVerts;

typedef struct Vertex_S {
    float x, y, z, w;
} Vertex;

bool InitPatch()
{
    vboID = idxID = 0;

    int nVerts = gridResolution * gridResolution;
    // Create the vertex array in normalized device coordinates
    Vertex *v = new Vertex[nVerts];
    if (!v) {
        return false;
    }

    unsigned int idx = 0;

    for (unsigned int row = 0; row < gridResolution; row++) {
        for (unsigned int col = 0; col < gridResolution; col++) {
            float x = ((float)row * 1.0f);
            float y = ((float)col * 1.0f);

            v[idx].x = x - 25.0f;
            v[idx].z = -(25.0f + y);
            v[idx].y = 0;
            v[idx].w = 1.0;

            idx++;
        }
    }

    // Create the index array
    unsigned int stripRows = gridResolution - 1;
    unsigned int indicesPerStrip = gridResolution * 2 + 2;
    nIndices = indicesPerStrip * stripRows;

    GLuint *i = new GLuint[nIndices];
    if (!i) {
        return false;
    }

    unsigned int vIdx = 0, iIdx = 0;

    for (unsigned int stripRow = 0; stripRow < stripRows; stripRow++) {
        i[iIdx++] = vIdx;
        for (unsigned int col = 0; col < gridResolution; col++) {
            i[iIdx++] = vIdx;
            i[iIdx++] = vIdx + gridResolution;
            vIdx++;
        }
        i[iIdx++] = vIdx - 1 + gridResolution;
    }

    glGenVertexArrays(1, &vertexArray);

    // Upload both to the GPU
    glGenBuffers(1, &vboID);
    glBindBuffer(GL_ARRAY_BUFFER, vboID);
    glBufferData(GL_ARRAY_BUFFER, gridResolution * gridResolution * sizeof(Vertex),
                 (void *)v, GL_STATIC_DRAW_ARB);

    glGenBuffers(1, &idxID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, nIndices * sizeof(GLuint),
                 (void *)i, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // Clean up
    delete[] v;
    delete[] i;

    return true;
}


// Compute the matrices for the camera and for rendering our sky box, and
// pass them into Triton so it can properly position and orient the water.
// We compute the projection and view matrices "by hand" to make this sample
// more useful for OpenGL 3.2+ developers.
void GrabMatrices(double *skyBoxMatrix)
{
    double mv[16], proj[16];
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);
    double aspect = (double)vp[2] / (double)vp[3];

    // Compute a perspective projection matrix
    BuildPerspProjMat(proj, 30.0, aspect, 10.0, 100000.0);

    // Spin the camera to face the sun in our skybox texture
    BuildIdentityMat(mv);
    // ApplyYaw(mv, yaw);
    EulerMatrix( mv, yaw, pitch, roll );

    // Capture the viewproj matrix before we apply translation, for use
    // in positioning our sky box:
    Multiply(skyBoxMatrix, mv, proj);

    // Apply the camera translation
    Translate(mv, -x, -y, -z);

    // Pass the final view and projection matrices into Triton.
    if (environment) {
        environment->SetCameraMatrix(mv);
        environment->SetProjectionMatrix(proj);
    }
}

static int cubeMapIndex = 0;
void nextCubeMap(void)
{
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
}

// Draw a frame of animation including the Triton ocean and our skybox.
void DrawFrame()
{
    double skyBoxMatrix[16];

    // Compute the modelview and perspective matrices, and a pass them into Triton.
    // Compute the viewproj matrix for our skybox at the same time.
    GrabMatrices(skyBoxMatrix);

    // Clear the depth buffer
    glClearDepth(1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Now light and draw the ocean patch:
    if (environment) {
        // The sun position is roughly where it is in our skybox texture:
        Triton::Vector3 lightPosition(0, 1.0 / sqrt(2.0), 1.0 / sqrt(2.0));
        environment->SetDirectionalLight(lightPosition, Triton::Vector3(1.0, 1.0, 1.0));

        // Ambient color based on the zenith color in the cube map
        environment->SetAmbientLight(Triton::Vector3(67.0 / 255.0, 102.0 / 255.0, 104.0 / 255.0) * 1.5);

        // Grab the cube map from our sky box and give it to Triton to use as an environment map
        environment->SetEnvironmentMap((Triton::TextureHandle)(skyBox->GetCubemap()));

        // Draw the ocean patch for the current time sample
        if (ocean) {
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

            // Convert the time to seconds, which is what Triton wants
            double time = (double)millis * 0.001;

            // Update our rotor wash
            if (rotorWash) rotorWash->Update(Triton::Vector3(0, 5, -55), Triton::Vector3(0,-1,0), 100.0, time);

            // Our patch is at a Y of -1 meter
            environment->SetSeaLevel(-1);

            // Update the ocean simulation once per frame; you could do this from
            // another thread if you want to.
            ocean->UpdateSimulation(time);

            // Bind our vertex and index arrays to draw our mesh
            glBindBuffer(GL_ARRAY_BUFFER, vboID);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, idxID);
            glEnableClientState(GL_VERTEX_ARRAY);
            glVertexPointer(4, GL_FLOAT, 0, 0);
            glDepthFunc(GL_LEQUAL);

            // Set up a model matrix to translate the patch
            double modelMatrix[16];
            BuildIdentityMat(modelMatrix);
            Translate(modelMatrix, 0, -1, -10);

            // Have Triton set all the state required to render our mesh as water

            // Define the bounds within spray particles will be generated
            Triton::TBoundingBox patchBounds;
            patchBounds.m_vMax = Triton::Vector3(250, 100, 250);
            patchBounds.m_vMin = Triton::Vector3(-250, -100, -250);

            ocean->SetPatchShader(time, 16, 0, false, modelMatrix);

            // Draw our mesh - Triton will make it look like water...
            glEnable(GL_CULL_FACE);
            glFrontFace(GL_CW);
            glDrawElements(GL_TRIANGLE_STRIP, nIndices, GL_UNSIGNED_INT, 0);

            // Restore the previous state.
            ocean->UnsetPatchShader(time, &patchBounds, false);

            // Do another pass for decals, if needed. Note the "true" being passed on
            // the decalPass parameters below.
            if (ocean->SetPatchShader(time, 16, 0, false, modelMatrix, true)) {
                glEnable(GL_CULL_FACE);
                glFrontFace(GL_CW);
                glDrawElements(GL_TRIANGLE_STRIP, nIndices, GL_UNSIGNED_INT, 0);
            }
            ocean->UnsetPatchShader(time, &patchBounds, true);

            // Draw as many more ocean patches as you want in the same manner,
            // just be sure to only call UpdateSimulation() once per frame.
        }
    }

    // Draw our skybox last to take advantage of early depth culling
    if (skyBox) {
        skyBox->Draw(skyBoxMatrix);
    }

    // Reveal the frame and ask for another one
    glutSwapBuffers();
    glutPostRedisplay();
}

// Handle a window resize
void Resize(GLint width, GLint height)
{
    glViewport(0, 0, width, height);
}

// Clean up our resources
void Destroy()
{
    if (ocean) delete ocean;
    if (environment) delete environment;
    if (resourceLoader) delete resourceLoader;
    if (skyBox) delete skyBox;

    if (vboID) {
        glDeleteBuffers(1, &vboID);
    }

    if (idxID) {
        glDeleteBuffers(1, &idxID);
    }
}

////////////////////////////////////////////////////////////////////////////////
void Keyboard(unsigned char key, int, int)
{
    switch(key) {
    case 27:
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
        y += 10.0;
        break;

    case 'f':
    case 'F':
        y -= 10.0;
        break;
    case 'c':
    case 'C':
        nextCubeMap();
        break;
    default:
        break;
    }

    glutPostRedisplay();
}
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char **argv)
{
    // Set up our window
    glutInit(&argc, (char **)argv);
    glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Triton OpenGL Sample");

    // We use GLEW mainly for the benefit of our SkyBox class
    glewInit();

    // Set up Triton's resources
    if (!InitTriton()) {
        return 0;
    }

    // Set up our little patch of geometry that Triton will transform into water
    InitPatch();

    // Create our sky box to render behind Triton's ocean and provide
    // its environment map
    skyBox = new SkyBox();
    skyBox->Create();
    nextCubeMap();

    // Some freeglut callbacks
    glutDisplayFunc(DrawFrame);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(Keyboard);

#ifdef FREEGLUT
    glutCloseFunc(Destroy);
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
#endif

    // Off we go:
    glutMainLoop();

    return 0;
}
