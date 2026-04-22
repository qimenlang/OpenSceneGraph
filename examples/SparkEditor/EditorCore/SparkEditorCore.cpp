#include "SparkEditorCore.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <iostream>

namespace spark_editor
{
namespace
{
Property makeBool(const std::string& key, const std::string& label, bool value)
{
    Property p;
    p.key = key;
    p.label = label;
    p.type = PropertyType::Bool;
    p.value = value;
    return p;
}

Property makeInt(const std::string& key, const std::string& label, int value, float speed, int minV, int maxV)
{
    Property p;
    p.key = key;
    p.label = label;
    p.type = PropertyType::Int;
    p.value = value;
    p.speed = speed;
    p.minValue = static_cast<float>(minV);
    p.maxValue = static_cast<float>(maxV);
    return p;
}

Property makeFloat(const std::string& key, const std::string& label, float value, float speed, float minV, float maxV)
{
    Property p;
    p.key = key;
    p.label = label;
    p.type = PropertyType::Float;
    p.value = value;
    p.speed = speed;
    p.minValue = minV;
    p.maxValue = maxV;
    return p;
}

Property makeVec3(const std::string& key, const std::string& label, const SPK::Vector3D& value, float speed, float minV, float maxV)
{
    Property p;
    p.key = key;
    p.label = label;
    p.type = PropertyType::Vec3;
    p.value = value;
    p.speed = speed;
    p.minValue = minV;
    p.maxValue = maxV;
    return p;
}

Property makeColor4(const std::string& key, const std::string& label, const SPK::Color& value)
{
    Property p;
    p.key = key;
    p.label = label;
    p.type = PropertyType::Color4;
    p.value = value;
    return p;
}

const Property* findPropertyConst(const std::vector<Property>& props, const std::string& key)
{
    for (std::vector<Property>::const_iterator it = props.begin(); it != props.end(); ++it)
    {
        if (it->key == key)
            return &(*it);
    }
    return NULL;
}

Property* findProperty(std::vector<Property>& props, const std::string& key)
{
    for (std::vector<Property>::iterator it = props.begin(); it != props.end(); ++it)
    {
        if (it->key == key)
            return &(*it);
    }
    return NULL;
}

bool readBool(const std::vector<Property>& props, const std::string& key, bool fallback)
{
    const Property* p = findPropertyConst(props, key);
    return (p && p->type == PropertyType::Bool) ? std::get<bool>(p->value) : fallback;
}

int readInt(const std::vector<Property>& props, const std::string& key, int fallback)
{
    const Property* p = findPropertyConst(props, key);
    return (p && p->type == PropertyType::Int) ? std::get<int>(p->value) : fallback;
}

float readFloat(const std::vector<Property>& props, const std::string& key, float fallback)
{
    const Property* p = findPropertyConst(props, key);
    return (p && p->type == PropertyType::Float) ? std::get<float>(p->value) : fallback;
}

SPK::Vector3D readVec3(const std::vector<Property>& props, const std::string& key, const SPK::Vector3D& fallback)
{
    const Property* p = findPropertyConst(props, key);
    return (p && p->type == PropertyType::Vec3) ? std::get<SPK::Vector3D>(p->value) : fallback;
}

SPK::Color readColor4(const std::vector<Property>& props, const std::string& key, const SPK::Color& fallback)
{
    const Property* p = findPropertyConst(props, key);
    return (p && p->type == PropertyType::Color4) ? std::get<SPK::Color>(p->value) : fallback;
}

const char* emitterTypeName(const SPK::Emitter* emitter)
{
    if (dynamic_cast<const SPK::SphericEmitter*>(emitter))
        return "SphericEmitter";
    if (dynamic_cast<const SPK::StraightEmitter*>(emitter))
        return "StraightEmitter";
    if (dynamic_cast<const SPK::NormalEmitter*>(emitter))
        return "NormalEmitter";
    if (dynamic_cast<const SPK::RandomEmitter*>(emitter))
        return "RandomEmitter";
    if (dynamic_cast<const SPK::StaticEmitter*>(emitter))
        return "StaticEmitter";
    return "Emitter";
}

void zoneRuntimeTypeName(SPK::Zone* z, std::string& out)
{
    out.clear();
    if (!z)
        return;
    if (dynamic_cast<SPK::Sphere*>(z))
        out = "Sphere";
    else if (dynamic_cast<SPK::Box*>(z))
        out = "Box";
    else if (dynamic_cast<SPK::Cylinder*>(z))
        out = "Cylinder";
    else if (dynamic_cast<SPK::Plane*>(z))
        out = "Plane";
    else if (dynamic_cast<SPK::Ring*>(z))
        out = "Ring";
    else if (dynamic_cast<SPK::Point*>(z))
        out = "Point";
    else
        out = "Zone";
}

/** Fills properties to match each zone type's spark_description (plus Zone::position from SPK_Zone.h). */
void extractEmitterZoneProperties(SPK::Zone* z, std::string& typeName, std::vector<Property>& out)
{
    out.clear();
    typeName.clear();
    if (!z)
        return;
    zoneRuntimeTypeName(z, typeName);

    const float vSpeed = 0.01f;
    const float vLim = 5000.0f;
    const float fSpeed = 0.01f;

    out.push_back(makeVec3("zone.position", "Position", z->getPosition(), vSpeed, -vLim, vLim));

    if (SPK::Sphere* s = dynamic_cast<SPK::Sphere*>(z))
        out.push_back(makeFloat("zone.radius", "Radius", s->getRadius(), fSpeed, 0.0f, 1.0e6f));
    else if (SPK::Box* b = dynamic_cast<SPK::Box*>(z))
    {
        out.push_back(makeVec3("zone.dimensions", "Dimensions", b->getDimensions(), vSpeed, 0.0f, vLim));
        out.push_back(makeVec3("zone.axisFront", "Axis (front / Z)", b->getZAxis(), vSpeed, -1.0f, 1.0f));
        out.push_back(makeVec3("zone.axisUp", "Axis (up / Y)", b->getYAxis(), vSpeed, -1.0f, 1.0f));
    }
    else if (SPK::Cylinder* c = dynamic_cast<SPK::Cylinder*>(z))
    {
        out.push_back(makeVec3("zone.axis", "Axis", c->getAxis(), vSpeed, -1.0f, 1.0f));
        out.push_back(makeFloat("zone.height", "Height", c->getHeight(), fSpeed, 0.0f, vLim));
        out.push_back(makeFloat("zone.radius", "Radius", c->getRadius(), fSpeed, 0.0f, vLim));
    }
    else if (SPK::Plane* pl = dynamic_cast<SPK::Plane*>(z))
        out.push_back(makeVec3("zone.normal", "Normal", pl->getNormal(), vSpeed, -1.0f, 1.0f));
    else if (SPK::Ring* r = dynamic_cast<SPK::Ring*>(z))
    {
        out.push_back(makeVec3("zone.normal", "Normal", r->getNormal(), vSpeed, -1.0f, 1.0f));
        out.push_back(makeFloat("zone.radiusMin", "Radius Min", r->getMinRadius(), fSpeed, 0.0f, vLim));
        out.push_back(makeFloat("zone.radiusMax", "Radius Max", r->getMaxRadius(), fSpeed, 0.0f, vLim));
    }
}

void applyEmitterZoneProperties(SPK::Zone* zone, const std::vector<Property>& props)
{
    if (!zone || props.empty())
        return;

    zone->setPosition(readVec3(props, "zone.position", zone->getPosition()));

    if (SPK::Sphere* s = dynamic_cast<SPK::Sphere*>(zone))
        s->setRadius(readFloat(props, "zone.radius", s->getRadius()));
    else if (SPK::Box* b = dynamic_cast<SPK::Box*>(zone))
    {
        b->setDimensions(readVec3(props, "zone.dimensions", b->getDimensions()));
        const SPK::Vector3D front = readVec3(props, "zone.axisFront", b->getZAxis());
        const SPK::Vector3D up = readVec3(props, "zone.axisUp", b->getYAxis());
        b->setAxis(front, up);
    }
    else if (SPK::Cylinder* c = dynamic_cast<SPK::Cylinder*>(zone))
    {
        c->setAxis(readVec3(props, "zone.axis", c->getAxis()));
        c->setHeight(readFloat(props, "zone.height", c->getHeight()));
        c->setRadius(readFloat(props, "zone.radius", c->getRadius()));
    }
    else if (SPK::Plane* pl = dynamic_cast<SPK::Plane*>(zone))
        pl->setNormal(readVec3(props, "zone.normal", pl->getNormal()));
    else if (SPK::Ring* r = dynamic_cast<SPK::Ring*>(zone))
    {
        r->setNormal(readVec3(props, "zone.normal", r->getNormal()));
        float rMin = readFloat(props, "zone.radiusMin", r->getMinRadius());
        float rMax = readFloat(props, "zone.radiusMax", r->getMaxRadius());
        r->setRadius(rMin, rMax);
    }
}

bool drawProperty(Property& prop, const std::string& scopeId)
{
    const std::string label = prop.label + "##" + scopeId + "." + prop.key;
    switch (prop.type)
    {
    case PropertyType::Bool:
    {
        bool v = std::get<bool>(prop.value);
        if (ImGui::Checkbox(label.c_str(), &v))
        {
            prop.value = v;
            return true;
        }
        return false;
    }
    case PropertyType::Int:
    {
        int v = std::get<int>(prop.value);
        if (ImGui::DragInt(label.c_str(), &v, prop.speed, static_cast<int>(prop.minValue), static_cast<int>(prop.maxValue)))
        {
            prop.value = v;
            return true;
        }
        return false;
    }
    case PropertyType::Float:
    {
        float v = std::get<float>(prop.value);
        if (ImGui::DragFloat(label.c_str(), &v, prop.speed, prop.minValue, prop.maxValue))
        {
            prop.value = v;
            return true;
        }
        return false;
    }
    case PropertyType::Vec3:
    {
        SPK::Vector3D v = std::get<SPK::Vector3D>(prop.value);
        float f[3] = {v.x, v.y, v.z};
        if (ImGui::DragFloat3(label.c_str(), f, prop.speed, prop.minValue, prop.maxValue))
        {
            prop.value = SPK::Vector3D(f[0], f[1], f[2]);
            return true;
        }
        return false;
    }
    case PropertyType::Color4:
    {
        SPK::Color c = std::get<SPK::Color>(prop.value);
        float rgba[4] = {c.r / 255.0f, c.g / 255.0f, c.b / 255.0f, c.a / 255.0f};
        if (ImGui::ColorEdit4(label.c_str(), rgba))
        {
            auto toByte = [](float x) -> int {
                x = std::clamp(x, 0.0f, 1.0f);
                return static_cast<int>(x * 255.0f + 0.5f);
            };
            prop.value = SPK::Color(toByte(rgba[0]), toByte(rgba[1]), toByte(rgba[2]), toByte(rgba[3]));
            return true;
        }
        return false;
    }
    }
    return false;
}

void sanitizeEmitterTankFlow(float& flow, int& tankMin, int& tankMax)
{
    if (tankMin > tankMax)
    {
        // Keep user intent when increasing min beyond max.
        tankMax = tankMin;
    }
    // SPARK requires min/max tank to share the same sign.
    if (tankMin < 0 || tankMax < 0)
    {
        tankMin = -1;
        tankMax = -1;
    }

    // Keep legal pairs for SPK assertions:
    // - if tank is infinite (-1,-1), flow must stay non-negative.
    if (tankMin < 0 && tankMax < 0 && flow < 0.0f)
        flow = 0.0f;

    // - if flow is negative, tank must be finite (>= 0).
    if (flow < 0.0f && (tankMin < 0 || tankMax < 0))
    {
        tankMin = 0;
        tankMax = std::max(0, tankMax);
    }
}

} // namespace

void SparkEditorCore::setSourceFilePath(const std::string& sourcePath)
{
    data_.sourceFilePath = sourcePath;
}

const char* SparkEditorCore::modifierTypeName(const SPK::Modifier* modifier) const
{
    if (dynamic_cast<const SPK::Gravity*>(modifier))
        return "Gravity";
    if (dynamic_cast<const SPK::Friction*>(modifier))
        return "Friction";
    if (dynamic_cast<const SPK::Obstacle*>(modifier))
        return "Obstacle";
    if (dynamic_cast<const SPK::LinearForce*>(modifier))
        return "LinearForce";
    if (dynamic_cast<const SPK::Rotator*>(modifier))
        return "Rotator";
    if (dynamic_cast<const SPK::Destroyer*>(modifier))
        return "Destroyer";
    if (dynamic_cast<const SPK::EmitterAttacher*>(modifier))
        return "EmitterAttacher";
    if (dynamic_cast<const SPK::Collider*>(modifier))
        return "Collider";
    if (dynamic_cast<const SPK::PointMass*>(modifier))
        return "PointMass";
    if (dynamic_cast<const SPK::RandomForce*>(modifier))
        return "RandomForce";
    if (dynamic_cast<const SPK::Vortex*>(modifier))
        return "Vortex";
    return "Modifier";
}

const char* SparkEditorCore::interpolatorTypeName(const SPK::Interpolator<float>* interpolator) const
{
    if (dynamic_cast<const SPK::FloatRandomInitializer*>(interpolator))
        return "FloatRandomInitializer";
    if (dynamic_cast<const SPK::FloatSimpleInterpolator*>(interpolator))
        return "FloatSimpleInterpolator";
    return "FloatInterpolator";
}

bool SparkEditorCore::extractFromSystem(SPK::System& system)
{
    data_.groups.clear();
    data_.emitters.clear();
    data_.modifiers.clear();
    data_.interpolators.clear();
    data_.renderers.clear();

    static const std::array<std::pair<SPK::Param, const char*>, 5> kParams = {{
        {SPK::PARAM_SCALE, "scale"},
        {SPK::PARAM_MASS, "mass"},
        {SPK::PARAM_ANGLE, "angle"},
        {SPK::PARAM_TEXTURE_INDEX, "textureIndex"},
        {SPK::PARAM_ROTATION_SPEED, "rotationSpeed"}
    }};

    for (size_t gi = 0; gi < system.getNbGroups(); ++gi)
    {
        SPK::Ref<SPK::Group> groupRef = system.getGroup(gi);
        if (!groupRef)
            continue;

        SPK::Group* group = groupRef.get();
        GroupNode groupNode;
        groupNode.groupIndex = static_cast<int>(gi);
        groupNode.title = "Group " + std::to_string(gi);
        groupNode.properties.push_back(makeInt("capacity", "Capacity", static_cast<int>(group->getCapacity()), 1.0f, 1, 1000000));
        groupNode.properties.push_back(makeFloat("lifeMin", "Life Min", group->getMinLifeTime(), 0.01f, 0.0f, 120.0f));
        groupNode.properties.push_back(makeFloat("lifeMax", "Life Max", group->getMaxLifeTime(), 0.01f, 0.0f, 120.0f));
        groupNode.properties.push_back(makeFloat("radiusGraphical", "Graphical Radius", group->getGraphicalRadius(), 0.01f, 0.0f, 100.0f));
        groupNode.properties.push_back(makeFloat("radiusPhysical", "Physical Radius", group->getPhysicalRadius(), 0.01f, 0.0f, 100.0f));
        groupNode.properties.push_back(makeBool("immortal", "Immortal", group->isImmortal()));
        groupNode.properties.push_back(makeBool("still", "Still", group->isStill()));
        groupNode.properties.push_back(makeBool("sorting", "Sorting", group->isSortingEnabled()));
        data_.groups.push_back(groupNode);

        SPK::Ref<SPK::Renderer> rendererRef = group->getRenderer();
        if (rendererRef)
        {
            SPK::Renderer* renderer = rendererRef.get();
            RendererNode r;
            r.groupIndex = static_cast<int>(gi);
            r.typeName = "Renderer";
            r.title = r.typeName;
            r.properties.push_back(makeBool("active", "Active", renderer->isActive()));
            r.properties.push_back(makeInt("renderingOptions", "Rendering Options", static_cast<int>(renderer->getRenderingOptions()), 1.0f, 0, 65535));

            data_.renderers.push_back(r);
        }

        for (size_t ei = 0; ei < group->getNbEmitters(); ++ei)
        {
            SPK::Ref<SPK::Emitter> emitterRef = group->getEmitter(ei);
            if (!emitterRef)
                continue;

            SPK::Emitter* emitter = emitterRef.get();
            EmitterNode e;
            e.groupIndex = static_cast<int>(gi);
            e.emitterIndex = static_cast<int>(ei);
            e.typeName = emitterTypeName(emitter);
            e.title = e.typeName;
            e.properties.push_back(makeBool("active", "Active", emitter->isActive()));
            e.properties.push_back(makeFloat("flow", "Flow", emitter->getFlow(), 0.05f, -1.0f, 500.0f));
            e.properties.push_back(makeInt("tankMin", "Tank Min", emitter->getMinTank(), 1.0f, -1, 100000));
            e.properties.push_back(makeInt("tankMax", "Tank Max", emitter->getMaxTank(), 1.0f, -1, 100000));
            e.properties.push_back(makeFloat("forceMin", "Force Min", emitter->getForceMin(), 0.01f, -1000.0f, 1000.0f));
            e.properties.push_back(makeFloat("forceMax", "Force Max", emitter->getForceMax(), 0.01f, -1000.0f, 1000.0f));
            e.properties.push_back(makeBool("fullZone", "Use Full Zone", emitter->isFullZone()));
            if (SPK::StraightEmitter* straight = dynamic_cast<SPK::StraightEmitter*>(emitter))
                e.properties.push_back(makeVec3("direction", "Direction", straight->getDirection(), 0.01f, -1.0f, 1.0f));
            if (SPK::SphericEmitter* spheric = dynamic_cast<SPK::SphericEmitter*>(emitter))
            {
                e.properties.push_back(makeVec3("direction", "Direction", spheric->getDirection(), 0.01f, -1.0f, 1.0f));
                e.properties.push_back(makeFloat("angleMin", "Angle Min", spheric->getAngleMin(), 0.01f, 0.0f, 6.28318f));
                e.properties.push_back(makeFloat("angleMax", "Angle Max", spheric->getAngleMax(), 0.01f, 0.0f, 6.28318f));
            }
            if (SPK::NormalEmitter* normal = dynamic_cast<SPK::NormalEmitter*>(emitter))
                e.properties.push_back(makeBool("inverted", "Inverted Normal", normal->isInverted()));

            SPK::Zone* zone = emitter->getZone().get();
            extractEmitterZoneProperties(zone, e.zoneTypeName, e.zoneProperties);
            data_.emitters.push_back(e);
        }

        for (size_t mi = 0; mi < group->getNbModifiers(); ++mi)
        {
            SPK::Ref<SPK::Modifier> modifierRef = group->getModifier(mi);
            if (!modifierRef)
                continue;
            SPK::Modifier* modifier = modifierRef.get();

            ModifierNode m;
            m.groupIndex = static_cast<int>(gi);
            m.modifierIndex = static_cast<int>(mi);
            m.typeName = modifierTypeName(modifier);
            m.title = m.typeName;

            if (SPK::Gravity* gravity = dynamic_cast<SPK::Gravity*>(modifier))
                m.properties.push_back(makeVec3("value", "Value", gravity->getValue(), 0.01f, -500.0f, 500.0f));
            else if (SPK::Friction* friction = dynamic_cast<SPK::Friction*>(modifier))
                m.properties.push_back(makeFloat("value", "Value", friction->getValue(), 0.001f, 0.0f, 5.0f));
            else if (SPK::Obstacle* obstacle = dynamic_cast<SPK::Obstacle*>(modifier))
            {
                m.properties.push_back(makeFloat("bouncingRatio", "Bouncing Ratio", obstacle->getBouncingRatio(), 0.01f, 0.0f, 2.0f));
                m.properties.push_back(makeFloat("friction", "Friction", obstacle->getFriction(), 0.01f, 0.0f, 2.0f));
            }
            else if (SPK::LinearForce* linearForce = dynamic_cast<SPK::LinearForce*>(modifier))
            {
                m.properties.push_back(makeVec3("value", "Value", linearForce->getValue(), 0.01f, -500.0f, 500.0f));
                m.properties.push_back(makeFloat("coef", "Coef", linearForce->getCoef(), 0.01f, -100.0f, 100.0f));
            }
            else if (SPK::Collider* collider = dynamic_cast<SPK::Collider*>(modifier))
                m.properties.push_back(makeFloat("elasticity", "Elasticity", collider->getElasticity(), 0.01f, 0.0f, 3.0f));
            else if (SPK::PointMass* pointMass = dynamic_cast<SPK::PointMass*>(modifier))
            {
                m.properties.push_back(makeVec3("position", "Position", pointMass->getPosition(), 0.01f, -500.0f, 500.0f));
                m.properties.push_back(makeFloat("mass", "Mass", pointMass->getMass(), 0.01f, -1000.0f, 1000.0f));
                m.properties.push_back(makeFloat("offset", "Offset", pointMass->getOffset(), 0.001f, 0.0001f, 1000.0f));
            }
            else if (SPK::RandomForce* randomForce = dynamic_cast<SPK::RandomForce*>(modifier))
            {
                m.properties.push_back(makeVec3("minVector", "Min Vector", randomForce->getMinVector(), 0.01f, -500.0f, 500.0f));
                m.properties.push_back(makeVec3("maxVector", "Max Vector", randomForce->getMaxVector(), 0.01f, -500.0f, 500.0f));
                m.properties.push_back(makeFloat("minPeriod", "Min Period", randomForce->getMinPeriod(), 0.01f, 0.001f, 100.0f));
                m.properties.push_back(makeFloat("maxPeriod", "Max Period", randomForce->getMaxPeriod(), 0.01f, 0.001f, 100.0f));
            }
            else if (SPK::Vortex* vortex = dynamic_cast<SPK::Vortex*>(modifier))
            {
                m.properties.push_back(makeVec3("position", "Position", vortex->getPosition(), 0.01f, -500.0f, 500.0f));
                m.properties.push_back(makeVec3("direction", "Direction", vortex->getDirection(), 0.01f, -1.0f, 1.0f));
                m.properties.push_back(makeFloat("rotationSpeed", "Rotation Speed", vortex->getRotationSpeed(), 0.01f, -1000.0f, 1000.0f));
                m.properties.push_back(makeFloat("attractionSpeed", "Attraction Speed", vortex->getAttractionSpeed(), 0.01f, -1000.0f, 1000.0f));
                m.properties.push_back(makeBool("rotationAngular", "Angular Rotation", vortex->isRotationSpeedAngular()));
                m.properties.push_back(makeBool("attractionLinear", "Linear Attraction", vortex->isAttractionSpeedLinear()));
                m.properties.push_back(makeFloat("eyeRadius", "Eye Radius", vortex->getEyeRadius(), 0.01f, 0.0f, 500.0f));
                m.properties.push_back(makeBool("killParticles", "Kill In Eye", vortex->isParticleKillingEnabled()));
            }
            else if (SPK::EmitterAttacher* attacher = dynamic_cast<SPK::EmitterAttacher*>(modifier))
            {
                m.properties.push_back(makeBool("enableOrientation", "Enable Orientation", attacher->isEmitterOrientationEnabled()));
                m.properties.push_back(makeBool("enableRotation", "Enable Rotation", attacher->isEmitterRotationEnabled()));
            }

            data_.modifiers.push_back(m);
        }

        SPK::Ref<SPK::ColorInterpolator> colorRef = group->getColorInterpolator();
        if (colorRef)
        {
            InterpolatorNode i;
            i.groupIndex = static_cast<int>(gi);
            i.param = -1;
            i.typeName = "ColorInterpolator";
            i.title = "ColorInterpolator";
            if (SPK::ColorSimpleInterpolator* colorSimple = dynamic_cast<SPK::ColorSimpleInterpolator*>(colorRef.get()))
            {
                i.typeName = "ColorSimpleInterpolator";
                i.properties.push_back(makeColor4("birth", "Birth", colorSimple->getBirthValue()));
                i.properties.push_back(makeColor4("death", "Death", colorSimple->getDeathValue()));
            }
            data_.interpolators.push_back(i);
        }

        for (size_t pi = 0; pi < kParams.size(); ++pi)
        {
            SPK::Ref<SPK::FloatInterpolator> interpRef = group->getParamInterpolator(kParams[pi].first);
            if (!interpRef)
                continue;

            InterpolatorNode i;
            i.groupIndex = static_cast<int>(gi);
            i.param = static_cast<int>(kParams[pi].first);
            i.typeName = interpolatorTypeName(interpRef.get());
            i.title = std::string(kParams[pi].second) + "Interpolator";
            if (SPK::FloatRandomInitializer* rnd = dynamic_cast<SPK::FloatRandomInitializer*>(interpRef.get()))
            {
                i.properties.push_back(makeFloat("min", "Min", rnd->getMinValue(), 0.01f, -1000.0f, 1000.0f));
                i.properties.push_back(makeFloat("max", "Max", rnd->getMaxValue(), 0.01f, -1000.0f, 1000.0f));
            }
            else if (SPK::FloatSimpleInterpolator* simp = dynamic_cast<SPK::FloatSimpleInterpolator*>(interpRef.get()))
            {
                i.properties.push_back(makeFloat("birth", "Birth", simp->getBirthValue(), 0.01f, -1000.0f, 1000.0f));
                i.properties.push_back(makeFloat("death", "Death", simp->getDeathValue(), 0.01f, -1000.0f, 1000.0f));
            }
            data_.interpolators.push_back(i);
        }
    }

    return true;
}

void SparkEditorCore::applyToSystem(SPK::System& system) const
{
    for (size_t gi = 0; gi < data_.groups.size(); ++gi)
    {
        const GroupNode& g = data_.groups[gi];
        if (g.groupIndex < 0 || static_cast<size_t>(g.groupIndex) >= system.getNbGroups())
            continue;
        SPK::Ref<SPK::Group> groupRef = system.getGroup(static_cast<size_t>(g.groupIndex));
        if (!groupRef)
            continue;
        SPK::Group* group = groupRef.get();

        const float lifeMin = readFloat(g.properties, "lifeMin", group->getMinLifeTime());
        const float lifeMax = readFloat(g.properties, "lifeMax", group->getMaxLifeTime());
        const float minLife = lifeMin < lifeMax ? lifeMin : lifeMax;
        const float maxLife = lifeMin < lifeMax ? lifeMax : lifeMin;
        int capacity = readInt(g.properties, "capacity", static_cast<int>(group->getCapacity()));
        if (capacity < 1)
            capacity = 1;
        const int minCapacity = std::max(1, static_cast<int>(group->getNbParticles()));
        if (capacity < minCapacity)
            capacity = minCapacity;
        if (static_cast<size_t>(capacity) != group->getCapacity())
            group->reallocate(static_cast<size_t>(capacity));
        group->setLifeTime(minLife, maxLife);
        group->setImmortal(readBool(g.properties, "immortal", group->isImmortal()));
        group->setStill(readBool(g.properties, "still", group->isStill()));
        group->enableSorting(readBool(g.properties, "sorting", group->isSortingEnabled()));
        group->setGraphicalRadius(readFloat(g.properties, "radiusGraphical", group->getGraphicalRadius()));
        group->setPhysicalRadius(readFloat(g.properties, "radiusPhysical", group->getPhysicalRadius()));
    }

    for (size_t ei = 0; ei < data_.emitters.size(); ++ei)
    {
        const EmitterNode& e = data_.emitters[ei];
        if (e.groupIndex < 0 || static_cast<size_t>(e.groupIndex) >= system.getNbGroups())
            continue;
        SPK::Ref<SPK::Group> groupRef = system.getGroup(static_cast<size_t>(e.groupIndex));
        if (!groupRef || e.emitterIndex < 0 || static_cast<size_t>(e.emitterIndex) >= groupRef->getNbEmitters())
            continue;
        SPK::Emitter* emitter = groupRef->getEmitter(static_cast<size_t>(e.emitterIndex)).get();
        if (!emitter)
            continue;

        emitter->setActive(readBool(e.properties, "active", emitter->isActive()));
        float flow = readFloat(e.properties, "flow", emitter->getFlow());
        int tankMin = readInt(e.properties, "tankMin", emitter->getMinTank());
        int tankMax = readInt(e.properties, "tankMax", emitter->getMaxTank());
        sanitizeEmitterTankFlow(flow, tankMin, tankMax);
        emitter->setTank(tankMin, tankMax);
        emitter->setFlow(flow);
        float forceMin = readFloat(e.properties, "forceMin", emitter->getForceMin());
        float forceMax = readFloat(e.properties, "forceMax", emitter->getForceMax());
        if (forceMin > forceMax)
            std::swap(forceMin, forceMax);
        emitter->setForce(forceMin, forceMax);
        emitter->setUseFullZone(readBool(e.properties, "fullZone", emitter->isFullZone()));
        if (SPK::StraightEmitter* straight = dynamic_cast<SPK::StraightEmitter*>(emitter))
            straight->setDirection(readVec3(e.properties, "direction", straight->getDirection()));
        if (SPK::SphericEmitter* spheric = dynamic_cast<SPK::SphericEmitter*>(emitter))
        {
            spheric->setDirection(readVec3(e.properties, "direction", spheric->getDirection()));
            float angleMin = readFloat(e.properties, "angleMin", spheric->getAngleMin());
            float angleMax = readFloat(e.properties, "angleMax", spheric->getAngleMax());
            if (angleMin > angleMax)
                std::swap(angleMin, angleMax);
            spheric->setAngles(angleMin, angleMax);
        }
        if (SPK::NormalEmitter* normal = dynamic_cast<SPK::NormalEmitter*>(emitter))
            normal->setInverted(readBool(e.properties, "inverted", normal->isInverted()));

        SPK::Zone* zone = emitter->getZone().get();
        if (zone && !e.zoneProperties.empty())
            applyEmitterZoneProperties(zone, e.zoneProperties);
    }

    for (size_t mi = 0; mi < data_.modifiers.size(); ++mi)
    {
        const ModifierNode& m = data_.modifiers[mi];
        if (m.groupIndex < 0 || static_cast<size_t>(m.groupIndex) >= system.getNbGroups())
            continue;
        SPK::Ref<SPK::Group> groupRef = system.getGroup(static_cast<size_t>(m.groupIndex));
        if (!groupRef || m.modifierIndex < 0 || static_cast<size_t>(m.modifierIndex) >= groupRef->getNbModifiers())
            continue;
        SPK::Modifier* modifier = groupRef->getModifier(static_cast<size_t>(m.modifierIndex)).get();
        if (!modifier)
            continue;

        if (SPK::Gravity* gravity = dynamic_cast<SPK::Gravity*>(modifier))
            gravity->setValue(readVec3(m.properties, "value", gravity->getValue()));
        else if (SPK::Friction* friction = dynamic_cast<SPK::Friction*>(modifier))
            friction->setValue(readFloat(m.properties, "value", friction->getValue()));
        else if (SPK::Obstacle* obstacle = dynamic_cast<SPK::Obstacle*>(modifier))
        {
            obstacle->setBouncingRatio(readFloat(m.properties, "bouncingRatio", obstacle->getBouncingRatio()));
            obstacle->setFriction(readFloat(m.properties, "friction", obstacle->getFriction()));
        }
        else if (SPK::LinearForce* linearForce = dynamic_cast<SPK::LinearForce*>(modifier))
        {
            linearForce->setValue(readVec3(m.properties, "value", linearForce->getValue()));
            linearForce->setCoef(readFloat(m.properties, "coef", linearForce->getCoef()));
        }
        else if (SPK::Collider* collider = dynamic_cast<SPK::Collider*>(modifier))
            collider->setElasticity(readFloat(m.properties, "elasticity", collider->getElasticity()));
        else if (SPK::PointMass* pointMass = dynamic_cast<SPK::PointMass*>(modifier))
        {
            pointMass->setPosition(readVec3(m.properties, "position", pointMass->getPosition()));
            pointMass->setMass(readFloat(m.properties, "mass", pointMass->getMass()));
            pointMass->setOffset(readFloat(m.properties, "offset", pointMass->getOffset()));
        }
        else if (SPK::RandomForce* randomForce = dynamic_cast<SPK::RandomForce*>(modifier))
        {
            randomForce->setVectors(
                readVec3(m.properties, "minVector", randomForce->getMinVector()),
                readVec3(m.properties, "maxVector", randomForce->getMaxVector()));
            float minPeriod = readFloat(m.properties, "minPeriod", randomForce->getMinPeriod());
            float maxPeriod = readFloat(m.properties, "maxPeriod", randomForce->getMaxPeriod());
            if (minPeriod > maxPeriod)
                std::swap(minPeriod, maxPeriod);
            randomForce->setPeriods(minPeriod, maxPeriod);
        }
        else if (SPK::Vortex* vortex = dynamic_cast<SPK::Vortex*>(modifier))
        {
            vortex->setPosition(readVec3(m.properties, "position", vortex->getPosition()));
            vortex->setDirection(readVec3(m.properties, "direction", vortex->getDirection()));
            vortex->setRotationSpeed(readFloat(m.properties, "rotationSpeed", vortex->getRotationSpeed()));
            vortex->setAttractionSpeed(readFloat(m.properties, "attractionSpeed", vortex->getAttractionSpeed()));
            vortex->setRotationSpeedAngular(readBool(m.properties, "rotationAngular", vortex->isRotationSpeedAngular()));
            vortex->setAttractionSpeedLinear(readBool(m.properties, "attractionLinear", vortex->isAttractionSpeedLinear()));
            vortex->setEyeRadius(readFloat(m.properties, "eyeRadius", vortex->getEyeRadius()));
            vortex->enableParticleKilling(readBool(m.properties, "killParticles", vortex->isParticleKillingEnabled()));
        }
        else if (SPK::EmitterAttacher* attacher = dynamic_cast<SPK::EmitterAttacher*>(modifier))
        {
            attacher->enableEmitterOrientation(readBool(m.properties, "enableOrientation", attacher->isEmitterOrientationEnabled()));
            attacher->enableEmitterRotation(readBool(m.properties, "enableRotation", attacher->isEmitterRotationEnabled()));
        }
    }

    for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
    {
        const InterpolatorNode& i = data_.interpolators[ii];
        if (i.groupIndex < 0 || static_cast<size_t>(i.groupIndex) >= system.getNbGroups())
            continue;
        SPK::Ref<SPK::Group> groupRef = system.getGroup(static_cast<size_t>(i.groupIndex));
        if (!groupRef)
            continue;

        if (i.param == -1)
        {
            SPK::Ref<SPK::ColorInterpolator> colorRef = groupRef->getColorInterpolator();
            if (SPK::ColorSimpleInterpolator* simpleColor = dynamic_cast<SPK::ColorSimpleInterpolator*>(colorRef.get()))
            {
                const SPK::Color birth = readColor4(i.properties, "birth", simpleColor->getBirthValue());
                const SPK::Color death = readColor4(i.properties, "death", simpleColor->getDeathValue());
                simpleColor->setValues(birth, death);
            }
            continue;
        }

        SPK::Ref<SPK::FloatInterpolator> interpRef = groupRef->getParamInterpolator(static_cast<SPK::Param>(i.param));
        if (!interpRef)
            continue;
        if (SPK::FloatRandomInitializer* rnd = dynamic_cast<SPK::FloatRandomInitializer*>(interpRef.get()))
        {
            float minV = readFloat(i.properties, "min", rnd->getMinValue());
            float maxV = readFloat(i.properties, "max", rnd->getMaxValue());
            if (minV > maxV)
                std::swap(minV, maxV);
            rnd->setValues(minV, maxV);
        }
        else if (SPK::FloatSimpleInterpolator* simp = dynamic_cast<SPK::FloatSimpleInterpolator*>(interpRef.get()))
        {
            simp->setValues(
                readFloat(i.properties, "birth", simp->getBirthValue()),
                readFloat(i.properties, "death", simp->getDeathValue()));
        }
    }

    for (size_t ri = 0; ri < data_.renderers.size(); ++ri)
    {
        const RendererNode& r = data_.renderers[ri];
        if (r.groupIndex < 0 || static_cast<size_t>(r.groupIndex) >= system.getNbGroups())
            continue;
        SPK::Ref<SPK::Group> groupRef = system.getGroup(static_cast<size_t>(r.groupIndex));
        if (!groupRef)
            continue;
        SPK::Ref<SPK::Renderer> rendererRef = groupRef->getRenderer();
        if (!rendererRef)
            continue;
        SPK::Renderer* renderer = rendererRef.get();

        renderer->setActive(readBool(r.properties, "active", renderer->isActive()));
        int renderingOptions = readInt(r.properties, "renderingOptions", static_cast<int>(renderer->getRenderingOptions()));
        if (renderingOptions < 0)
            renderingOptions = 0;
        renderer->setRenderingOptions(static_cast<unsigned int>(renderingOptions));
    }
}

void SparkEditorCore::drawImGui(SPK::Ref<SPK::System>& system)
{
    if (!system)
    {
        ImGui::TextUnformatted("System not loaded.");
        return;
    }

    bool changed = false;

    auto drawEmitterNode = [&changed, &system](EmitterNode& e) {
        if (!ImGui::TreeNode((e.title + "##emitter").c_str()))
            return;

        Property* flowProp = findProperty(e.properties, "flow");
        Property* tankMinProp = findProperty(e.properties, "tankMin");
        Property* tankMaxProp = findProperty(e.properties, "tankMax");

        for (size_t p = 0; p < e.properties.size(); ++p)
        {
            Property& prop = e.properties[p];
            if (prop.key == "flow" || prop.key == "tankMin" || prop.key == "tankMax")
                continue;
            changed = drawProperty(prop, e.title) || changed;
        }

        if (flowProp && flowProp->type == PropertyType::Float)
        {
            float flowV = std::get<float>(flowProp->value);
            const std::string flowLabel = flowProp->label + "##" + e.title + "." + flowProp->key;
            float flowMin = flowProp->minValue;
            if (tankMinProp && tankMaxProp &&
                tankMinProp->type == PropertyType::Int &&
                tankMaxProp->type == PropertyType::Int)
            {
                const int minV = std::get<int>(tankMinProp->value);
                const int maxV = std::get<int>(tankMaxProp->value);
                if (minV < 0 && maxV < 0)
                    flowMin = 0.0f;
            }
            if (ImGui::DragFloat(flowLabel.c_str(), &flowV, flowProp->speed, flowMin, flowProp->maxValue))
            {
                flowProp->value = flowV;
                changed = true;
            }
        }

        if (tankMinProp && tankMaxProp &&
            tankMinProp->type == PropertyType::Int &&
            tankMaxProp->type == PropertyType::Int)
        {
            int minV = std::get<int>(tankMinProp->value);
            int maxV = std::get<int>(tankMaxProp->value);
            float flowV = flowProp && flowProp->type == PropertyType::Float ? std::get<float>(flowProp->value) : 0.0f;

            const int tankMinUiMin = (flowV < 0.0f) ? 0 : static_cast<int>(tankMinProp->minValue);
            const int tankMaxUiMin = (flowV < 0.0f) ? 0 : static_cast<int>(tankMaxProp->minValue);

            bool tankChanged = false;
            const std::string minLabel = tankMinProp->label + "##" + e.title + "." + tankMinProp->key;
            const std::string maxLabel = tankMaxProp->label + "##" + e.title + "." + tankMaxProp->key;

            if (ImGui::DragInt(minLabel.c_str(),
                               &minV,
                               tankMinProp->speed,
                               tankMinUiMin,
                               static_cast<int>(tankMinProp->maxValue)))
                tankChanged = true;

            if (ImGui::DragInt(maxLabel.c_str(),
                               &maxV,
                               tankMaxProp->speed,
                               tankMaxUiMin,
                               static_cast<int>(tankMaxProp->maxValue)))
                tankChanged = true;

            const int oldMin = std::get<int>(tankMinProp->value);
            const int oldMax = std::get<int>(tankMaxProp->value);

            if (tankChanged || (flowProp && flowProp->type == PropertyType::Float))
            {
                sanitizeEmitterTankFlow(flowV, minV, maxV);

                if (flowProp && flowProp->type == PropertyType::Float)
                    flowProp->value = flowV;
                tankMinProp->value = minV;
                tankMaxProp->value = maxV;

                changed = changed || tankChanged || minV != oldMin || maxV != oldMax;
            }
        }

        const std::string zoneTreeLabel = "Zone (" + e.zoneTypeName + ")##emitterZone." + std::to_string(e.groupIndex) + "." + std::to_string(e.emitterIndex);
        if (ImGui::TreeNode(zoneTreeLabel.c_str()))
        {
            if (e.zoneTypeName.empty())
                ImGui::TextDisabled("(no zone)");
            else
            {
                SPK::Zone* liveZone = NULL;
                if (e.groupIndex >= 0 && static_cast<size_t>(e.groupIndex) < system->getNbGroups())
                {
                    SPK::Ref<SPK::Group> gr = system->getGroup(static_cast<size_t>(e.groupIndex));
                    if (gr && e.emitterIndex >= 0 && static_cast<size_t>(e.emitterIndex) < gr->getNbEmitters())
                    {
                        SPK::Emitter* em = gr->getEmitter(static_cast<size_t>(e.emitterIndex)).get();
                        if (em)
                            liveZone = em->getZone().get();
                    }
                }
                if (liveZone && liveZone->isShared())
                    ImGui::TextDisabled("(shared zone — edits affect all references)");

                const std::string zoneScope = e.title + "Zone";
                for (size_t zi = 0; zi < e.zoneProperties.size(); ++zi)
                    changed = drawProperty(e.zoneProperties[zi], zoneScope) || changed;
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
    };

    auto drawSimpleNode = [&changed](const std::string& treeId, const std::string& title, std::vector<Property>& properties) {
        if (!ImGui::TreeNode((title + treeId).c_str()))
            return;
        if (properties.empty())
            ImGui::TextUnformatted("No exposed editable properties for this type yet.");
        for (size_t p = 0; p < properties.size(); ++p)
            changed = drawProperty(properties[p], title) || changed;
        ImGui::TreePop();
    };

    if (ImGui::CollapsingHeader("Particle System Tree", ImGuiTreeNodeFlags_DefaultOpen))
    {
        for (size_t i = 0; i < data_.groups.size(); ++i)
        {
            GroupNode& g = data_.groups[i];
            if (!ImGui::TreeNode((g.title + "##group").c_str()))
                continue;

            if (ImGui::TreeNode(("Group Properties##groupProps." + g.title).c_str()))
            {
                SPK::Group* liveGroup = NULL;
                if (g.groupIndex >= 0 && static_cast<size_t>(g.groupIndex) < system->getNbGroups())
                {
                    SPK::Ref<SPK::Group> gr = system->getGroup(static_cast<size_t>(g.groupIndex));
                    if (gr)
                        liveGroup = gr.get();
                }

                if (liveGroup)
                    ImGui::Text("Particles: %zu", liveGroup->getNbParticles());
                else
                    ImGui::TextDisabled("Particles: (n/a)");

                for (size_t p = 0; p < g.properties.size(); ++p)
                {
                    const std::string& key = g.properties[p].key;
                    if (key == "immortal" || key == "still" || key == "sorting")
                        continue;
                    if (key == "capacity")
                    {
                        Property& capProp = g.properties[p];
                        if (capProp.type == PropertyType::Int)
                        {
                            int v = std::get<int>(capProp.value);
                            const int minCap = liveGroup ? std::max(1, static_cast<int>(liveGroup->getNbParticles())) : 1;
                            if (v < minCap)
                            {
                                v = minCap;
                                capProp.value = v;
                                changed = true;
                            }
                            const std::string label = capProp.label + "##" + g.title + "." + capProp.key;
                            if (ImGui::DragInt(label.c_str(), &v, capProp.speed, minCap, static_cast<int>(capProp.maxValue)))
                            {
                                if (v < minCap)
                                    v = minCap;
                                capProp.value = v;
                                changed = true;
                            }
                        }
                        continue;
                    }
                    changed = drawProperty(g.properties[p], g.title) || changed;
                }


                Property* propImmortal = findProperty(g.properties, "immortal");
                Property* propStill = findProperty(g.properties, "still");
                Property* propSorting = findProperty(g.properties, "sorting");
                if (propImmortal && propImmortal->type == PropertyType::Bool)
                    changed = drawProperty(*propImmortal, g.title) || changed;
                if (propStill && propStill->type == PropertyType::Bool)
                {
                    ImGui::SameLine();
                    changed = drawProperty(*propStill, g.title) || changed;
                }
                if (propSorting && propSorting->type == PropertyType::Bool)
                {
                    ImGui::SameLine();
                    changed = drawProperty(*propSorting, g.title) || changed;
                }

                ImGui::TreePop();
            }

            bool hasRenderer = false;
            for (size_t ri = 0; ri < data_.renderers.size(); ++ri)
            {
                RendererNode& renderer = data_.renderers[ri];
                if (renderer.groupIndex != g.groupIndex)
                    continue;
                hasRenderer = true;
                drawSimpleNode("##renderer", renderer.title, renderer.properties);
            }
            if (!hasRenderer)
                ImGui::TextDisabled("Renderer: (none)");

            bool hasEmitters = false;
            if (ImGui::TreeNode(("Emitters##emitters." + g.title).c_str()))
            {
                for (size_t ei = 0; ei < data_.emitters.size(); ++ei)
                {
                    EmitterNode& e = data_.emitters[ei];
                    if (e.groupIndex != g.groupIndex)
                        continue;
                    hasEmitters = true;
                    drawEmitterNode(e);
                }
                if (!hasEmitters)
                    ImGui::TextDisabled("(none)");
                ImGui::TreePop();
            }

            bool hasModifiers = false;
            if (ImGui::TreeNode(("Modifiers##modifiers." + g.title).c_str()))
            {
                for (size_t mi = 0; mi < data_.modifiers.size(); ++mi)
                {
                    ModifierNode& m = data_.modifiers[mi];
                    if (m.groupIndex != g.groupIndex)
                        continue;
                    hasModifiers = true;
                    drawSimpleNode("##modifier", m.title, m.properties);
                }
                if (!hasModifiers)
                    ImGui::TextDisabled("(none)");
                ImGui::TreePop();
            }

            bool hasInterpolators = false;
            if (ImGui::TreeNode(("Interpolators##interps." + g.title).c_str()))
            {
                for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
                {
                    InterpolatorNode& interp = data_.interpolators[ii];
                    if (interp.groupIndex != g.groupIndex)
                        continue;
                    hasInterpolators = true;
                    drawSimpleNode("##interp", interp.title, interp.properties);
                }
                if (!hasInterpolators)
                    ImGui::TextDisabled("(none)");
                ImGui::TreePop();
            }

            ImGui::TreePop();
        }
    }

    if (changed)
        applyToSystem(*system);

    ImGui::Text("Source File : %s", data_.sourceFilePath.c_str());
}

} // namespace spark_editor
