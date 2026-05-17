// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.
#undef NDEBUG
#include <cassert>
#include "SampleDeclares.h"

extern Triton::Environment *environment;
extern Triton::Ocean *ocean;

extern SkyBox *skyBox;
extern Triton::Camera* cameras[2];

void DrawImmediateSingleThreaded(double millis, int frame)
{
    if (environment == 0 || ocean == 0) {
        return;
    }


    // Compute the modelview and perspective matrices, and a pass them into Triton.
    // Compute the viewproj matrix for our skybox at the same time.
    double mv[16], proj[16], skyBoxMatrix[16];
    double mv0[16];
    double mv1[16];
    CalculateSkyBoxMatrices(mv, mv0, mv1, proj, skyBoxMatrix);

    SetUpTritonFromMatrices(mv, mv0, mv1, proj);

    if (b_render_small_views) {
        for (int view = 0; view < numViews; ++view) {
            _render_small_view_begin(view, millis);

            // Update the simulation
            ocean->UpdateSimulation((double)millis * 0.001, cameras[view]);
            ocean->DrawConcurrent((double)millis * 0.001, true, true, true, 0, cameras[view]);

            //Draw our skybox last to take advantage of early depth culling
            if (draw_sky && skyBox && ocean->IsCameraAboveWater(cameras[view])) {
                skyBox->Draw(skyBoxMatrix);
            }

            _render_small_view_end(view, millis);
        }
    }

    // do the main view now
    glViewport(0, 0, windowWidth, windowHeight);
    clearBasedOnCamera(environment->GetCamera());
    // Draw the ocean for the current time sample
    // Ensure proper sorting of the waves against each other:
    glDepthFunc(GL_LEQUAL);

    // Update the simulation
    ocean->UpdateSimulation((double)millis * 0.001, environment->GetCamera());
    ocean->DrawConcurrent((double)millis * 0.001, true, true, true, 0, environment->GetCamera());

    ocean->PostDrawConcurrent();

    if (b_render_small_views) {
        glDisable(GL_BLEND);
        RenderQuad(20, 20, 256, 256, 0);
        RenderQuad(600, 20, 256, 256, 1);
    }

    //Draw our skybox last to take advantage of early depth culling
    if (draw_sky && skyBox && ocean->IsCameraAboveWater(environment->GetCamera())) {
        skyBox->Draw(skyBoxMatrix);
    }
}
