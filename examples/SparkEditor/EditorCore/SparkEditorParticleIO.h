#pragma once

#include <SPARK.h>
#include <string>

namespace spark_editor
{

/** Register a GL texture id as coming from SparkRes/<fileName> (used when saving). */
void RegisterDemoTextureFile(unsigned int textureId, const std::string& sparkResFileName);
bool LoadTextureForEditor(const std::string& imagePath, unsigned int& outTextureId);

/**
 * Before IO::Manager::save: sets each GL quad/point renderer's SPKObject name to the texture image path
 * string to persist (relative file name beside @a savePath when the texture came from SparkRes), copies
 * those assets next to @a savePath. Renderer tuning is persisted via SPARK IO / spk_attribute on GL renderers.
 */
bool PrepareParticleSaveWithDemoTextures(const SPK::Ref<SPK::System>& system, const std::string& savePath);

/**
 * After load: for GL quad (TEXTURE_MODE_2D) or point sprite renderers, reads getName() as the texture image
 * path (absolute, or relative to the directory of @a loadedParticlePath), loads the image and setTexture.
 * @a loadedParticlePath is the path passed to IO::Manager::load.
 */
void BindLoadedParticleTextures(const SPK::Ref<SPK::System>& system, const std::string& loadedParticlePath);

} // namespace spark_editor
