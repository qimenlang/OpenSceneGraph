// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.

#pragma once

#include "GL/glew.h"

class RenderToTexture
{
public:
    RenderToTexture(int width, int height);
    ~RenderToTexture();

    void bind() const;
    void unbind() const;

    void bindColorTex() const;
    void unbindColorTex() const;

    GLuint GetFBO(void) const;
private:
    GLuint fbo;
    GLuint color_texture;
    GLuint depth_texture;

    int width;
    int height;
};
