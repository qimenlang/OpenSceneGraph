// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.
#undef NDEBUG
#include <cassert>
#include "SampleDeclares.h"

extern Triton::Environment *environment;
extern Triton::Ocean *ocean;

extern SkyBox *skyBox;
extern Triton::Camera* cameras[numViews+1];

extern RenderToTexture *rtt_textures[numViews];

extern Triton::Stream* rtt_stream[numViews+1];

#include "GLFW/glfw3.h"

//extern GLFWwindow* main_view_render_thread_context;
extern GLFWwindow* main_window_and_context;

// used for multi-threaded, multi context path
extern GLFWwindow* small_view_offscreen_window[numViews];

extern bool quit;

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace multi_contexts
{
std::thread thread_for_view[numViews+1];
bool begin_render_view[numViews + 1] = { false, false, false };
bool end_render_view[numViews + 1] = { false, false, false };

std::condition_variable cv;
std::mutex _mutex;

std::atomic<long int> numRenders;

struct FrameParameters {
    double millis;
    int frame;
    double skyBoxMatrix[16];
};

FrameParameters params;

#define DRAWDEFERREDMULTITHREADED_DEBUG 0

void SignalViewsToStartRendering()
{
    // signal threads to start rendering
    {
        std::lock_guard<std::mutex> lk(_mutex);
        for (int i = 0; i < numViews+1; ++i) {
            begin_render_view[i] = true;
        }
    }
    cv.notify_all();
}

bool allFinished()
{
    bool _allFinished = true;
    for (int i = 0; i < numViews+1; ++i) {
        _allFinished &= end_render_view[i];
    }
    return _allFinished;
}

void WaitForAllViewsToFinishRendering()
{
    // wait for the worker threads
    std::unique_lock<std::mutex> lk(_mutex);
    //while (!allFinished())
    //{
    cv.wait(lk, allFinished);
    //}
    for (int i = 0; i < numViews+1; ++i) {
        end_render_view[i] = false;
    }
}


void QuitFromAllThreads(long int frame)
{
    quit = true;
    cv.notify_all();
    for (int i = 0; i < numViews+1; ++i) {
        thread_for_view[i].join();
    }

    assert(numRenders / (numViews + 1) == frame);
}


void render_view(int view)
{
    // If its not the main view
    // make the context current
    if (view!=2) {
        glfwMakeContextCurrent(small_view_offscreen_window[view]);
    }

    for (;;) {
        // Wait for main thread to signal ready
        {
            std::unique_lock<std::mutex> lk(_mutex);

            cv.wait(lk, [view] { return begin_render_view[view] || quit; });
            if (quit) {
                break;
            }
        }

        if ( (b_render_small_views && (view!=2))) {
            // Draw (generate stream) for this small view
            ocean->UpdateSimulation((double)params.millis * 0.001, cameras[view]);
            ocean->DrawConcurrent((double)params.millis * 0.001, true, true, true, rtt_stream[view], cameras[view]);
            _render_small_view_begin(view, params.millis);

            // Execute this stream, in this context
            environment->ExecuteOpenGLStream(rtt_stream[view]);
            environment->ResetOpenGLStream(rtt_stream[view]);

            if (draw_sky && skyBox && ocean->IsCameraAboveWater(cameras[view])) {
                skyBox->Draw(params.skyBoxMatrix);
            }

            _render_small_view_end(view, params.millis);
            // block till all the commands above are finished
            glFinish();
        } else {
            // thread for the main view. We are not going to draw here (because we draw immediately after the 2 views have done), but we can if we want to.
            // optionally, we can do some work here which has nothing to do with rendering
        }

        ++numRenders;

#if DRAWDEFERREDMULTITHREADED_DEBUG
        std::cout << "Rendered view: " << view << std::endl;
#endif
        {
            std::unique_lock<std::mutex> lk(_mutex);
            end_render_view[view] = true;
            begin_render_view[view] = false;
            lk.unlock();
            cv.notify_all();
        }
    }
}

void InitializeThreads()
{
    numRenders = 0;
    for (int i = 0; i < numViews; ++i) {
        thread_for_view[i] = std::thread(render_view, i);
    }
    thread_for_view[numViews] = std::thread(render_view, numViews);
}

void DrawDeferredMultiThreadedMultipleContexts(double millis, int frame)
{
    if (environment == 0 || ocean == 0) {
        return;
    }

    params.millis = millis;
    params.frame = frame;

    // Compute the modelview and perspective matrices, and a pass them into Triton.
    // Compute the viewproj matrix for our skybox at the same time.
    double mv[16], proj[16], skyBoxMatrix[16];
    double mv0[16];
    double mv1[16];
    CalculateSkyBoxMatrices(mv, mv0, mv1, proj, skyBoxMatrix);

    memcpy(params.skyBoxMatrix, skyBoxMatrix, 16*sizeof(double));

    SetUpTritonFromMatrices(mv, mv0, mv1, proj);

    // Update the simulation
    ocean->UpdateSimulation((double)millis * 0.001);

#if DRAWDEFERREDMULTITHREADED_DEBUG
    std::cout << "Signaling..." << std::endl;
#endif
    SignalViewsToStartRendering();
#if DRAWDEFERREDMULTITHREADED_DEBUG
    std::cout << "waiting..." << std::endl;
#endif
    WaitForAllViewsToFinishRendering();
#if DRAWDEFERREDMULTITHREADED_DEBUG
    std::cout << "Back in main!" << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
    std::cout << std::endl;
#endif
    assert(numRenders / (numViews + 1) == frame + 1);

    if(numRenders / (numViews + 1) != frame + 1) {
        std::cout << "view: 0 : " << end_render_view[0] << std::endl;
        std::cout << "view: 1 : " << end_render_view[1] << std::endl;
        std::cout << "main : " << end_render_view[2]<< std::endl;
        std::cout << "num renders: " << numRenders << std::endl;
        std::cout << "frame: " << frame+1 << std::endl;
        assert(false);
    }
    // do the main stream now
    glViewport(0, 0, windowWidth, windowHeight);
    clearBasedOnCamera(environment->GetCamera());
    // Draw the ocean for the current time sample
    // Ensure proper sorting of the waves against each other:
    glDepthFunc(GL_LEQUAL);

    // Draw the main view, immediately
    ocean->DrawConcurrent((double)params.millis * 0.001, true, true, true);

    ocean->PostDrawConcurrent();

    if (b_render_small_views) {
        glDisable(GL_BLEND);
        RenderQuad(20, 20, 256, 256, 0);
        RenderQuad(600, 20, 256, 256, 1);
    }
    // Draw our skybox last to take advantage of early depth culling
    if (draw_sky && skyBox && ocean->IsCameraAboveWater()) {
        skyBox->Draw(skyBoxMatrix);
    }
    //std::cout << std::endl << std::endl;
}
}