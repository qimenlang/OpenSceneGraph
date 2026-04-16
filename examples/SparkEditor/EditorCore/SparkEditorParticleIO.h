#pragma once

#include <SPARK.h>
#include <string>

namespace spark_editor
{

/** Register a GL texture id as coming from SparkRes/<fileName> (used when saving). */
void RegisterDemoTextureFile(unsigned int textureId, const std::string& sparkResFileName);

/**
 * Before IO::Manager::save: sets each GL renderer's SPKObject name to the texture basename,
 * and copies SparkRes textures into the directory of @a savePath.
 */
bool PrepareParticleSaveWithDemoTextures(const SPK::Ref<SPK::System>& system, const std::string& savePath);

/**
 * After load: for GL quad/point renderers with a non-empty name, loads <dir>/<name> as a 2D texture
 * and calls setTexture. @a loadedParticlePath is the path passed to IO::Manager::load.
 */
void BindLoadedParticleTextures(const SPK::Ref<SPK::System>& system, const std::string& loadedParticlePath);

} // namespace spark_editor
