// Copyright (c) 2011-2018 Sundog Software LLC. All rights reserved worldwide.

#include "RenderToTexture.h"

RenderToTexture::RenderToTexture(int _width, int _height)
    : width(_width), height(_height)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glGenTextures(1, &color_texture);
    glBindTexture(GL_TEXTURE_2D, color_texture);
    glTexStorage2D(GL_TEXTURE_2D, 9, GL_RGBA8, width, height);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &depth_texture);
    glBindTexture(GL_TEXTURE_2D, depth_texture);
    glTexStorage2D(GL_TEXTURE_2D, 9, GL_DEPTH_COMPONENT32F, width, height);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_texture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_texture, 0);

    static const GLenum draw_buffers[] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, draw_buffers);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
RenderToTexture::~RenderToTexture()
{
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &color_texture);
    glDeleteTextures(1, &depth_texture);
}

void RenderToTexture::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glViewport(0,0, width, height);
}
void RenderToTexture::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderToTexture::bindColorTex() const
{
    glBindTexture(GL_TEXTURE_2D, color_texture);
}
void RenderToTexture::unbindColorTex() const
{
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLuint RenderToTexture::GetFBO(void) const
{
    return fbo;
}