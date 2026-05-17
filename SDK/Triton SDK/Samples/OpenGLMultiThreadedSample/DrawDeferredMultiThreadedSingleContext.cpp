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

extern bool quit;

#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

namespace single_context
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
    for (;;) {
        // Wait for main thread to signal ready
        {
            std::unique_lock<std::mutex> lk(_mutex);

            cv.wait(lk, [view] { return begin_render_view[view] || quit; });
            if (quit) {
                break;
            }
        }

        // This will do the stream/packet generation
        // For DrawConcurrent, we don't need a context at all!
        // Its simply generating all the commands for this view,
        // to be executed later on the main thread
        if ( (b_render_small_views && (view!=2)) || view==2) {
            // Update the simulation
            ocean->UpdateSimulation((double)params.millis * 0.001, cameras[view]);
            ocean->DrawConcurrent((double)params.millis * 0.001, true, true, true, rtt_stream[view], cameras[view]);
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

void InitStreamsIfNeeded(void)
{
    if (rtt_stream[0] == 0) {
        for (int i = 0; i < numViews + 1; ++i) {
            rtt_stream[i] = environment->CreateOpenGLStream();
            ocean->Initialize(rtt_stream[i], cameras[i]);
        }
    }
}

void DrawDeferredMultiThreadedSingleContext(double millis, int frame)
{
    if (environment == 0 || ocean == 0) {
        return;
    }

    // Here we are in main thread, and also with the main window context
    // Create a stream for doing the stream/packet generation work for each view
    // including the main view
    InitStreamsIfNeeded();

    params.millis = millis;
    params.frame = frame;

    // Compute the modelview and perspective matrices, and a pass them into Triton.
    // Compute the viewproj matrix for our skybox at the same time.
    double mv[16], proj[16], skyBoxMatrix[16];
    double mv0[16];
    double mv1[16];
    CalculateSkyBoxMatrices(mv, mv0, mv1, proj, skyBoxMatrix);

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
    // At this point each of the small views and the main view have generated the stream/command list
    // so execute them into the rtt
    if (b_render_small_views) {
        for (int view = 0; view < numViews; ++view) {
            // bind the fbo
            _render_small_view_begin(view, millis);
            environment->ExecuteOpenGLStream(rtt_stream[view]);
            environment->ResetOpenGLStream(rtt_stream[view]);

            // Draw our skybox last to take advantage of early depth culling
            if (draw_sky && skyBox && ocean->IsCameraAboveWater(cameras[view])) {
                skyBox->Draw(skyBoxMatrix);
            }

            // un-bind the fbo
            _render_small_view_end(view, millis);
        }
    }

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

    // execute the main view stream as well
    environment->ExecuteOpenGLStream(rtt_stream[numViews]);
    environment->ResetOpenGLStream(rtt_stream[numViews]);

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