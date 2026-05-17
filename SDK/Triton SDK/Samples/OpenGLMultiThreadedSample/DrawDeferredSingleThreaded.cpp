// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.
#include <cassert>
#include "SampleDeclares.h"

extern Triton::ResourceLoader *resourceLoader;
extern Triton::Environment *environment;
extern Triton::Ocean *ocean;

extern SkyBox *skyBox;
extern Triton::Camera* cameras[2];

extern RenderToTexture *rtt_textures[numViews];
extern Triton::Stream* rtt_stream[numViews];

void InitStreamsIfNeeded(void)
{
    if (rtt_stream[0] == 0) {
        for (int i = 0; i < numViews + 1; ++i) {
            rtt_stream[i] = environment->CreateOpenGLStream();
            ocean->Initialize(rtt_stream[i], cameras[i]);
        }
    }
}

void DrawDeferredSingleThreaded(double millis, int frame)
{
    if (environment == 0 || ocean == 0) {
        return;
    }

    InitStreamsIfNeeded();

    // Compute the modelview and perspective matrices, and a pass them into Triton.
    // Compute the viewproj matrix for our skybox at the same time.
    double mv[16], proj[16], skyBoxMatrix[16];
    double mv0[16];
    double mv1[16];
    CalculateSkyBoxMatrices(mv, mv0, mv1, proj, skyBoxMatrix);

    SetUpTritonFromMatrices(mv, mv0, mv1, proj);

    // draw into the deferred context, but don't actually draw
    for (int view = 0; view < numViews; ++view) {
        assert(rtt_stream[view]);
        _render_small_view_begin(view, millis);
        // Update the simulation
        ocean->UpdateSimulation((double)millis * 0.001, cameras[view]);
        ocean->DrawConcurrent((double)millis * 0.001, true, true, true, rtt_stream[view], cameras[view]);
        _render_small_view_end(view, millis);
    }
    // actually draw(execute the stream) into the quads
    for (int view = 0; view < numViews; ++view) {
        assert(rtt_stream[view]);
        _render_small_view_begin(view, millis);
        environment->ExecuteOpenGLStream(rtt_stream[view]);
        environment->ResetOpenGLStream(rtt_stream[view]);

        if (draw_sky && skyBox && ocean->IsCameraAboveWater(cameras[view])) {
            skyBox->Draw(skyBoxMatrix);
        }

        _render_small_view_end(view, millis);
    }

    // do the main view/stream now
    glViewport(0, 0, windowWidth, windowHeight);
    clearBasedOnCamera(environment->GetCamera());
    // Draw the ocean for the current time sample
    // Ensure proper sorting of the waves against each other:
    glDepthFunc(GL_LEQUAL);

    // Update the simulation
    ocean->UpdateSimulation((double)millis * 0.001, cameras[numViews]);
    ocean->DrawConcurrent((double)millis * 0.001, true, true, true, rtt_stream[numViews], cameras[numViews]);

    environment->ExecuteOpenGLStream(rtt_stream[numViews]);
    environment->ResetOpenGLStream(rtt_stream[numViews]);

    ocean->PostDrawConcurrent();

    glDisable(GL_BLEND);
    RenderQuad(20, 20, 256, 256, 0);
    RenderQuad(600, 20, 256, 256, 1);

    // Draw our skybox last to take advantage of early depth culling
    if (draw_sky && skyBox && ocean->IsCameraAboveWater()) {
        skyBox->Draw(skyBoxMatrix);
    }
}
