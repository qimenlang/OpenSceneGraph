#pragma once

#include <SPARK.h>
#include <string>
#include <variant>
#include <vector>

namespace spark_editor
{

enum class PropertyType
{
    Bool,
    Int,
    Float,
    Vec3,
    Color4
};

struct Property
{
    std::string key;
    std::string label;
    PropertyType type = PropertyType::Float;
    std::variant<bool, int, float, SPK::Vector3D, SPK::Color> value;
    float speed = 0.01f;
    float minValue = 0.0f;
    float maxValue = 1.0f;
};

struct GroupNode
{
    int groupIndex = -1;
    std::string title;
    std::vector<Property> properties;
};

struct EmitterNode
{
    int groupIndex = -1;
    int emitterIndex = -1;
    std::string typeName;
    std::string title;
    std::vector<Property> properties;
    /** Runtime class name of emitter->getZone() (Point, Sphere, Box, …). */
    std::string zoneTypeName;
    /** Editable zone fields per SPARK spark_description setters for that zone type, plus Zone::position. */
    std::vector<Property> zoneProperties;
};

struct ModifierNode
{
    int groupIndex = -1;
    int modifierIndex = -1;
    std::string typeName;
    std::string title;
    std::vector<Property> properties;
};

struct InterpolatorNode
{
    int groupIndex = -1;
    int param = -1; // -1 means color interpolator
    std::string typeName;
    std::string title;
    std::vector<Property> properties;
};

struct RendererNode
{
    int groupIndex = -1;
    std::string typeName;
    std::string title;
    std::vector<Property> properties;
};

struct SystemEditableData
{
    std::string sourceFilePath;
    std::vector<GroupNode> groups;
    std::vector<EmitterNode> emitters;
    std::vector<ModifierNode> modifiers;
    std::vector<InterpolatorNode> interpolators;
    std::vector<RendererNode> renderers;
};

/** Per-group transient UI for "New … + Add" rows (not serialized). */
struct GroupAddUiState
{
    int modifierKind = 0;
    int emitterKind = 0;
    int rendererKind = 0;
    int colorInterpKind = 0;
    int floatInterpParam = 0;
    int floatInterpKind = 0;
};

class SparkEditorCore
{
public:
    void setSourceFilePath(const std::string& sourcePath);
    bool extractFromSystem(SPK::System& system);
    void drawImGui(SPK::Ref<SPK::System>& system);
    /** Call from Spark drawable drawImplementation (GL context current) to upload queued texture. */
    void applyQueuedQuadTextureLoad(SPK::Ref<SPK::System>& system);

private:
    /** After ImGui tree: file dialog only; GPU upload is deferred to applyQueuedQuadTextureLoad. */
    bool processDeferredQuadTexturePick(SPK::Ref<SPK::System>& system);
    void applyToSystem(SPK::System& system) const;
    const char* modifierTypeName(const SPK::Modifier* modifier) const;
    const char* interpolatorTypeName(const SPK::Interpolator<float>* interpolator) const;

    SystemEditableData data_;
    std::vector<GroupAddUiState> groupAddUi_;
    bool pendingQuadTexturePick_ = false;
    int pendingQuadTextureGroupIndex_ = -1;
    bool queuedQuadTextureLoad_ = false;
    int queuedQuadTextureGroupIndex_ = -1;
    std::string queuedQuadTexturePath_;
};

} // namespace spark_editor
