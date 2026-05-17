// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.

#pragma once

#include "Triton.h"
#include "RenderToTexture.h"
#include "skybox.h"
#include <string.h>

const int numViews = 2;
const int view_width = 256;
const int view_height = 256;

const int windowWidth = 1425;
const int windowHeight = 705;

void CalculateSkyBoxMatrices(double mv[16], double mv0[16], double mv1[16], double proj[16], double skyBoxMatrix[16]);
void SetUpTritonFromMatrices(double mv[16], double mv0[16], double mv1[16], double proj[16]);

void clearBasedOnCamera(Triton::Camera* camera);

void _render_small_view_begin(int view, double millis);
void _render_small_view_end(int view, double millis);

void QuitFromAllThreads(long int frame);
void InitThreadingStuff();

void RenderQuad(int x, int y, int width, int height, int view);

void InitStreamsIfNeeded(void);

enum DrawStrategy {
    // Draw all views, immediately, in a single thread
    DRAW_IMMEDIATE_SINGLE_THREADED = 0

                                     // Each view has a corresponding deferred stream
                                     // Each view is drawn into the deferred stream, in a single thread
                                     // All streams are executed(i.e. the view drawn), in a single thread
                                     // , and hence in a serialized fashion
                                     , DRAW_DEFERRED_SINGLE_THREADED = 1

                                             // Each view has a corresponding deferred stream
                                             // Each view is drawn into the deferred stream, in its own thread
                                             // These threads don't have (or require) any OpenGL context
                                             // All streams are executed(i.e. the view drawn), in the main thread, which is the only one
                                             // that has the OpenGL context
                                             , DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT = 2

                                                     // Each small view has a corresponding deferred stream
                                                     // Each small view is drawn into the deferred stream, in its own thread
                                                     // These threads each have their own context
                                                     // Each small view stream is also executed in its own thread
                                                     // Just as an illustration, the main view is drawn immediately in the main thread,
                                                     // but similar to DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT, you can do the stream generation for the main view
                                                     // (DrawConcurrent) in another thread, and then execute this on the main thread

                                                     // Note that even if you actually render (execute the command list or draw immediately) in multiple threads, you aren't going to
                                                     // get any performance gains, because internally the GPU is synchronizing its gl calls. We also actually do a blocking call
                                                     // (using glFinish). Furthermore, context switching that takes place implicitly slows down
                                                     // performance.  Therefore, the optimal strategy is
                                                     // DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT (doing work/command list generation for each view in a separate thread using
                                                     // DrawConcurrent, with no contexts at all, and then executing all command lists on the main thread

                                                     // This is provided for completeness however, because some engines (e.g. OpenSceneGraph support multi-threaded rendering)
                                                     , DRAW_DEFERRED_MULTI_THREADED_MULTI_CONTEXT = 3
};

const DrawStrategy drawStrategy = DRAW_DEFERRED_MULTI_THREADED_SINGLE_CONTEXT;

const bool b_render_small_views = true;

const bool  draw_sky = true;

#define TEST_UNDERWATER 0


