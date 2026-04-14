#pragma once

#include <SPARK.h>
#include <SPARK_GL.h>
#include <string>
#include <vector>

namespace spark_editor
{

struct DemoTextureSet
{
    GLuint explosion = 0;
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
SPK::Ref<SPK::System> CreateDemoSystem(size_t index, const DemoTextureSet& textures);

} // namespace spark_editor
