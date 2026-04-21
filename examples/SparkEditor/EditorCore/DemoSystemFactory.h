#pragma once

#include <SPARK.h>
#include <SPARK_GL.h>
#include <string>
#include <vector>

namespace spark_editor
{

struct DemoTextureSet
{
    /** SPKExplosion.cpp: explosion.bmp as GL_ALPHA, GL_CLAMP, no mipmap. */
    GLuint explosion = 0;
    /** SPKTest.cpp: same file as GL_LUMINANCE, GL_CLAMP, with mipmaps. */
    GLuint explosionSPKTest = 0;
    GLuint flash = 0;
    GLuint spark1 = 0;
    GLuint spark2 = 0;
    GLuint wave = 0;
    GLuint flare = 0;
    GLuint point = 0;
    GLuint ball = 0;
    GLuint waterdrops = 0;
};

const std::vector<std::string>& GetDemoSystemNames();
/** Creates the demo matching @a demoName (must match an entry in GetDemoSystemNames()). */
SPK::Ref<SPK::System> CreateDemoSystem(const std::string& demoName, const DemoTextureSet& textures);

} // namespace spark_editor
