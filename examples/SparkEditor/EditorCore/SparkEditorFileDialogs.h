#pragma once

#include <string>

namespace spark_editor
{

std::string SanitizeFileStem(const std::string& name);

bool ShowOpenParticleFileDialog(std::string& outPath);
bool ShowOpenImageFileDialog(std::string& outPath);

bool ShowSaveParticleFileDialog(const std::string& suggestedStem, std::string& outPath);

} // namespace spark_editor
