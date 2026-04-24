#include "SparkEditorCore.h"

#include "Rendering/OpenGL/SPK_GL_LineRenderer.h"
#include "Rendering/OpenGL/SPK_GL_PointRenderer.h"
#include "Rendering/OpenGL/SPK_GL_QuadRenderer.h"

#include <imgui.h>
#include <algorithm>
#include <array>
#include <cmath>
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
        tankMax = (std::max)(0, tankMax);
    }
}

void extractColorGraphProperties(SPK::ColorGraphInterpolator* graph, InterpolatorNode& i)
{
    i.properties.push_back(makeInt("grInterpType", "Graph X type (0=life,1=age,2=param,3=vel)", static_cast<int>(graph->getType()), 1.0f, 0, 3));
    i.properties.push_back(makeInt("grDriveParam", "Graph drive param (if type=param)", static_cast<int>(graph->getInterpolatorParam()), 1.0f, 0, 4));
    i.properties.push_back(makeBool("grLoop", "Graph loop", graph->isLoopingEnabled()));
    i.properties.push_back(makeFloat("grOffsetXVar", "Graph offset X variation", graph->getOffsetXVariation(), 0.01f, -100.0f, 100.0f));
    i.properties.push_back(makeFloat("grScaleXVar", "Graph scale X variation", graph->getScaleXVariation(), 0.01f, 0.0f, 0.99f));
    const unsigned int n = graph->getNbEntries();
    for (unsigned int e = 0; e < n; ++e)
    {
        const std::string base = "grE" + std::to_string(e);
        i.properties.push_back(makeFloat(base + "_x", "Graph pt " + std::to_string(e) + " X", graph->getX(e), 0.01f, -1.0e6f, 1.0e6f));
        i.properties.push_back(makeColor4(base + "_y0", "Graph pt " + std::to_string(e) + " Y0", graph->getY0(e)));
        i.properties.push_back(makeColor4(base + "_y1", "Graph pt " + std::to_string(e) + " Y1", graph->getY1(e)));
    }
}

void applyColorGraphProperties(const std::vector<Property>& props, SPK::ColorGraphInterpolator* graph)
{
    int typeI = readInt(props, "grInterpType", static_cast<int>(graph->getType()));
    typeI = std::clamp(typeI, 0, 3);
    const SPK::InterpolationType itype = static_cast<SPK::InterpolationType>(typeI);
    int paramI = readInt(props, "grDriveParam", static_cast<int>(graph->getInterpolatorParam()));
    paramI = std::clamp(paramI, 0, 4);
    const SPK::Param driveParam = static_cast<SPK::Param>(paramI);
    if (itype == SPK::INTERPOLATOR_PARAM)
        graph->setType(itype, driveParam);
    else
        graph->setType(itype);
    graph->enableLooping(readBool(props, "grLoop", graph->isLoopingEnabled()));
    graph->setOffsetXVariation(readFloat(props, "grOffsetXVar", graph->getOffsetXVariation()));
    float scaleVar = readFloat(props, "grScaleXVar", graph->getScaleXVariation());
    if (std::fabs(scaleVar) >= 1.0f)
        scaleVar = std::copysign(0.99f, scaleVar >= 0.0f ? 1.0f : -1.0f);
    graph->setScaleXVariation(scaleVar);

    const unsigned int n = graph->getNbEntries();
    for (unsigned int e = 0; e < n; ++e)
    {
        const std::string base = "grE" + std::to_string(e);
        const Property* px = findPropertyConst(props, base + "_x");
        if (!px || px->type != PropertyType::Float)
            continue;
        graph->setX(e, readFloat(props, base + "_x", graph->getX(e)));
        graph->setY0(e, readColor4(props, base + "_y0", graph->getY0(e)));
        graph->setY1(e, readColor4(props, base + "_y1", graph->getY1(e)));
    }
}

void extractFloatGraphProperties(SPK::FloatGraphInterpolator* graph, InterpolatorNode& i)
{
    i.properties.push_back(makeInt("grInterpType", "Graph X type (0=life,1=age,2=param,3=vel)", static_cast<int>(graph->getType()), 1.0f, 0, 3));
    i.properties.push_back(makeInt("grDriveParam", "Graph drive param (if type=param)", static_cast<int>(graph->getInterpolatorParam()), 1.0f, 0, 4));
    i.properties.push_back(makeBool("grLoop", "Graph loop", graph->isLoopingEnabled()));
    i.properties.push_back(makeFloat("grOffsetXVar", "Graph offset X variation", graph->getOffsetXVariation(), 0.01f, -100.0f, 100.0f));
    i.properties.push_back(makeFloat("grScaleXVar", "Graph scale X variation", graph->getScaleXVariation(), 0.01f, 0.0f, 0.99f));
    const unsigned int n = graph->getNbEntries();
    for (unsigned int e = 0; e < n; ++e)
    {
        const std::string base = "grE" + std::to_string(e);
        i.properties.push_back(makeFloat(base + "_x", "Graph pt " + std::to_string(e) + " X", graph->getX(e), 0.01f, -1.0e6f, 1.0e6f));
        i.properties.push_back(makeFloat(base + "_y0", "Graph pt " + std::to_string(e) + " Y0", graph->getY0(e), 0.01f, -1.0e6f, 1.0e6f));
        i.properties.push_back(makeFloat(base + "_y1", "Graph pt " + std::to_string(e) + " Y1", graph->getY1(e), 0.01f, -1.0e6f, 1.0e6f));
    }
}

void applyFloatGraphProperties(const std::vector<Property>& props, SPK::FloatGraphInterpolator* graph)
{
    int typeI = readInt(props, "grInterpType", static_cast<int>(graph->getType()));
    typeI = std::clamp(typeI, 0, 3);
    const SPK::InterpolationType itype = static_cast<SPK::InterpolationType>(typeI);
    int paramI = readInt(props, "grDriveParam", static_cast<int>(graph->getInterpolatorParam()));
    paramI = std::clamp(paramI, 0, 4);
    const SPK::Param driveParam = static_cast<SPK::Param>(paramI);
    if (itype == SPK::INTERPOLATOR_PARAM)
        graph->setType(itype, driveParam);
    else
        graph->setType(itype);
    graph->enableLooping(readBool(props, "grLoop", graph->isLoopingEnabled()));
    graph->setOffsetXVariation(readFloat(props, "grOffsetXVar", graph->getOffsetXVariation()));
    float scaleVar = readFloat(props, "grScaleXVar", graph->getScaleXVariation());
    if (std::fabs(scaleVar) >= 1.0f)
        scaleVar = std::copysign(0.99f, scaleVar >= 0.0f ? 1.0f : -1.0f);
    graph->setScaleXVariation(scaleVar);

    const unsigned int n = graph->getNbEntries();
    for (unsigned int e = 0; e < n; ++e)
    {
        const std::string base = "grE" + std::to_string(e);
        const Property* px = findPropertyConst(props, base + "_x");
        if (!px || px->type != PropertyType::Float)
            continue;
        graph->setX(e, readFloat(props, base + "_x", graph->getX(e)));
        graph->setY0(e, readFloat(props, base + "_y0", graph->getY0(e)));
        graph->setY1(e, readFloat(props, base + "_y1", graph->getY1(e)));
    }
}

std::string makeImGuiComboItems(const char* const* labels, int n)
{
    std::string s;
    for (int i = 0; i < n; ++i)
    {
        s += labels[i];
        s.push_back('\0');
    }
    s.push_back('\0');
    return s;
}

SPK::Ref<SPK::ColorInterpolator> spawnColorInterpolator(int kind)
{
    switch (kind)
    {
    case 0:
        return SPK::ColorSimpleInterpolator::create(0xFFFFFFFF, 0xFF000000);
    case 1:
        return SPK::ColorDefaultInitializer::create(0xFFFFFFFF);
    case 2:
        return SPK::ColorRandomInitializer::create(0xFF606060, 0xFFFFFFFF);
    case 3:
        return SPK::ColorRandomInterpolator::create(0xFFAAAAAA, 0xFFFFFFFF, 0xFF000000, 0xFF444444);
    case 4:
    {
        SPK::Ref<SPK::ColorGraphInterpolator> g = SPK::ColorGraphInterpolator::create();
        g->addEntry(0.0f, 0xFFFFFFFF);
        g->addEntry(1.0f, 0xFF000000);
        return g;
    }
    default:
        return SPK::ColorSimpleInterpolator::create(0xFFFFFFFF, 0xFF000000);
    }
}

SPK::Ref<SPK::FloatInterpolator> spawnFloatInterpolator(int kind)
{
    switch (kind)
    {
    case 0:
        return SPK::FloatSimpleInterpolator::create(0.0f, 1.0f);
    case 1:
        return SPK::FloatDefaultInitializer::create(1.0f);
    case 2:
        return SPK::FloatRandomInitializer::create(0.0f, 1.0f);
    case 3:
        return SPK::FloatRandomInterpolator::create(0.0f, 1.0f, 0.0f, 1.0f);
    case 4:
    {
        SPK::Ref<SPK::FloatGraphInterpolator> g = SPK::FloatGraphInterpolator::create();
        g->addEntry(0.0f, 0.0f, 1.0f);
        g->addEntry(1.0f, 1.0f, 0.0f);
        return g;
    }
    default:
        return SPK::FloatSimpleInterpolator::create(0.0f, 1.0f);
    }
}

SPK::Ref<SPK::Modifier> spawnModifier(int kind)
{
    switch (kind)
    {
    case 0:
        return SPK::Gravity::create(SPK::Vector3D(0.0f, -1.0f, 0.0f));
    case 1:
        return SPK::Friction::create(0.2f);
    case 2:
        return SPK::Obstacle::create(SPK::Sphere::create(SPK::Vector3D(), 2.0f), 0.8f, 0.9f);
    case 3:
        return SPK::LinearForce::create(SPK::Vector3D(0.0f, 0.5f, 0.0f), SPK::Sphere::create(SPK::Vector3D(), 5.0f), SPK::ZONE_TEST_INSIDE);
    case 4:
        return SPK::Collider::create(0.8f);
    case 5:
        return SPK::PointMass::create(SPK::Vector3D(), 10.0f, 0.1f);
    case 6:
        return SPK::RandomForce::create(SPK::Vector3D(-0.2f, -0.2f, -0.2f), SPK::Vector3D(0.2f, 0.2f, 0.2f), 0.5f, 1.5f);
    case 7:
        return SPK::Vortex::create(SPK::Vector3D(), SPK::Vector3D(0.0f, 1.0f, 0.0f), 1.0f, 0.0f);
    case 8:
        return SPK::EmitterAttacher::create(SPK_NULL_REF, SPK_NULL_REF, false, false);
    case 9:
        return SPK::Destroyer::create(SPK::Sphere::create(SPK::Vector3D(), 50.0f), SPK::ZONE_TEST_INSIDE);
    case 10:
        return SPK::Rotator::create();
    default:
        return SPK::Gravity::create();
    }
}

SPK::Ref<SPK::Emitter> spawnEmitter(int kind)
{
    SPK::Ref<SPK::Point> point = SPK::Point::create();
    switch (kind)
    {
    case 0:
        return SPK::StaticEmitter::create(point, true, -1, 20.0f);
    case 1:
        return SPK::RandomEmitter::create(point, true, -1, 25.0f, 0.2f, 0.8f);
    case 2:
        return SPK::StraightEmitter::create(SPK::Vector3D(0.0f, 1.0f, 0.0f), point, true, -1, 20.0f, 0.2f, 0.8f);
    case 3:
        return SPK::SphericEmitter::create(SPK::Vector3D(0.0f, 1.0f, 0.0f), 0.0f, 3.14159f * 0.5f, point, true, -1, 25.0f, 0.2f, 0.8f);
    case 4:
        return SPK::NormalEmitter::create(point, true, -1, 20.0f, 0.2f, 0.8f);
    default:
        return SPK::RandomEmitter::create(point);
    }
}

SPK::Ref<SPK::Renderer> spawnRenderer(int kind)
{
    switch (kind)
    {
    case 0:
        return SPK::GL::GLQuadRenderer::create(1.0f, 1.0f);
    case 1:
        return SPK::GL::GLPointRenderer::create(3.0f);
    case 2:
        return SPK::GL::GLLineRenderer::create(0.15f, 2.0f);
    default:
        return SPK::GL::GLQuadRenderer::create(1.0f, 1.0f);
    }
}

const char* rendererTypeName(const SPK::Renderer* renderer)
{
    if (dynamic_cast<const SPK::GL::GLQuadRenderer*>(renderer))
        return "GLQuadRenderer";
    if (dynamic_cast<const SPK::GL::GLPointRenderer*>(renderer))
        return "GLPointRenderer";
    if (dynamic_cast<const SPK::GL::GLLineRenderer*>(renderer))
        return "GLLineRenderer";
    return "Renderer";
}

int rendererSpawnKind(const SPK::Renderer* renderer)
{
    if (dynamic_cast<const SPK::GL::GLQuadRenderer*>(renderer))
        return 0;
    if (dynamic_cast<const SPK::GL::GLPointRenderer*>(renderer))
        return 1;
    if (dynamic_cast<const SPK::GL::GLLineRenderer*>(renderer))
        return 2;
    return 0;
}

void spawnNewGroup(SPK::System& system)
{
    SPK::Ref<SPK::Group> g = system.createGroup(500);
    if (!g)
        return;
    g->setLifeTime(1.0f, 4.0f);
    g->setRadius(0.1f);
    g->addEmitter(spawnEmitter(1));
}

static const char* kModifierSpawnLabels[] = {
    "Gravity",
    "Friction",
    "Obstacle",
    "LinearForce",
    "Collider",
    "PointMass",
    "RandomForce",
    "Vortex",
    "EmitterAttacher",
    "Destroyer",
    "Rotator"};
static const int kNbModifierSpawns = static_cast<int>(sizeof(kModifierSpawnLabels) / sizeof(kModifierSpawnLabels[0]));

static const char* kEmitterSpawnLabels[] = {
    "StaticEmitter",
    "RandomEmitter",
    "StraightEmitter",
    "SphericEmitter",
    "NormalEmitter"};
static const int kNbEmitterSpawns = static_cast<int>(sizeof(kEmitterSpawnLabels) / sizeof(kEmitterSpawnLabels[0]));

static const char* kRendererSpawnLabels[] = {
    "GLQuadRenderer",
    "GLPointRenderer",
    "GLLineRenderer"};
static const int kNbRendererSpawns = static_cast<int>(sizeof(kRendererSpawnLabels) / sizeof(kRendererSpawnLabels[0]));

static const char* kColorInterpSpawnLabels[] = {
    "ColorSimpleInterpolator",
    "ColorDefaultInitializer",
    "ColorRandomInitializer",
    "ColorRandomInterpolator",
    "ColorGraphInterpolator"};
static const int kNbColorInterpSpawns = static_cast<int>(sizeof(kColorInterpSpawnLabels) / sizeof(kColorInterpSpawnLabels[0]));

static const char* kFloatInterpSpawnLabels[] = {
    "FloatSimpleInterpolator",
    "FloatDefaultInitializer",
    "FloatRandomInitializer",
    "FloatRandomInterpolator",
    "FloatGraphInterpolator"};
static const int kNbFloatInterpSpawns = static_cast<int>(sizeof(kFloatInterpSpawnLabels) / sizeof(kFloatInterpSpawnLabels[0]));

static const char* kParamSlotLabels[] = {
    "scale",
    "mass",
    "angle",
    "textureIndex",
    "rotationSpeed"};
static const int kNbParamSlots = static_cast<int>(sizeof(kParamSlotLabels) / sizeof(kParamSlotLabels[0]));

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
    if (dynamic_cast<const SPK::FloatGraphInterpolator*>(interpolator))
        return "FloatGraphInterpolator";
    if (dynamic_cast<const SPK::FloatRandomInterpolator*>(interpolator))
        return "FloatRandomInterpolator";
    if (dynamic_cast<const SPK::FloatSimpleInterpolator*>(interpolator))
        return "FloatSimpleInterpolator";
    if (dynamic_cast<const SPK::FloatRandomInitializer*>(interpolator))
        return "FloatRandomInitializer";
    if (dynamic_cast<const SPK::FloatDefaultInitializer*>(interpolator))
        return "FloatDefaultInitializer";
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
            r.typeName = rendererTypeName(renderer);
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
            int sameTypeOrdinal = 1;
            for (size_t prev = 0; prev < data_.emitters.size(); ++prev)
            {
                if (data_.emitters[prev].groupIndex == static_cast<int>(gi) &&
                    data_.emitters[prev].typeName == e.typeName)
                {
                    ++sameTypeOrdinal;
                }
            }
            e.title = e.typeName + " " + std::to_string(sameTypeOrdinal);
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
            SPK::ColorInterpolator* colorInterp = colorRef.get();
            if (SPK::ColorGraphInterpolator* colorGraph = dynamic_cast<SPK::ColorGraphInterpolator*>(colorInterp))
            {
                i.typeName = "ColorGraphInterpolator";
                i.title = "ColorGraphInterpolator";
                extractColorGraphProperties(colorGraph, i);
            }
            else if (SPK::ColorRandomInterpolator* colorRnd = dynamic_cast<SPK::ColorRandomInterpolator*>(colorInterp))
            {
                i.typeName = "ColorRandomInterpolator";
                i.title = "ColorRandomInterpolator";
                i.properties.push_back(makeColor4("minBirth", "Min birth", colorRnd->getMinBirthValue()));
                i.properties.push_back(makeColor4("maxBirth", "Max birth", colorRnd->getMaxBirthValue()));
                i.properties.push_back(makeColor4("minDeath", "Min death", colorRnd->getMinDeathValue()));
                i.properties.push_back(makeColor4("maxDeath", "Max death", colorRnd->getMaxDeathValue()));
            }
            else if (SPK::ColorSimpleInterpolator* colorSimple = dynamic_cast<SPK::ColorSimpleInterpolator*>(colorInterp))
            {
                i.typeName = "ColorSimpleInterpolator";
                i.title = "ColorSimpleInterpolator";
                i.properties.push_back(makeColor4("birth", "Birth", colorSimple->getBirthValue()));
                i.properties.push_back(makeColor4("death", "Death", colorSimple->getDeathValue()));
            }
            else if (SPK::ColorRandomInitializer* colorRndInit = dynamic_cast<SPK::ColorRandomInitializer*>(colorInterp))
            {
                i.typeName = "ColorRandomInitializer";
                i.title = "ColorRandomInitializer";
                i.properties.push_back(makeColor4("min", "Min", colorRndInit->getMinValue()));
                i.properties.push_back(makeColor4("max", "Max", colorRndInit->getMaxValue()));
            }
            else if (SPK::ColorDefaultInitializer* colorDef = dynamic_cast<SPK::ColorDefaultInitializer*>(colorInterp))
            {
                i.typeName = "ColorDefaultInitializer";
                i.title = "ColorDefaultInitializer";
                i.properties.push_back(makeColor4("value", "Value", colorDef->getDefaultValue()));
            }
            else
            {
                i.typeName = "ColorInterpolator";
                i.title = "ColorInterpolator";
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
            SPK::FloatInterpolator* fi = interpRef.get();
            i.typeName = interpolatorTypeName(fi);
            i.title = std::string(kParams[pi].second) + "Interpolator";
            if (SPK::FloatGraphInterpolator* fg = dynamic_cast<SPK::FloatGraphInterpolator*>(fi))
                extractFloatGraphProperties(fg, i);
            else if (SPK::FloatRandomInterpolator* fri = dynamic_cast<SPK::FloatRandomInterpolator*>(fi))
            {
                i.properties.push_back(makeFloat("minBirth", "Min birth", fri->getMinBirthValue(), 0.01f, -1.0e6f, 1.0e6f));
                i.properties.push_back(makeFloat("maxBirth", "Max birth", fri->getMaxBirthValue(), 0.01f, -1.0e6f, 1.0e6f));
                i.properties.push_back(makeFloat("minDeath", "Min death", fri->getMinDeathValue(), 0.01f, -1.0e6f, 1.0e6f));
                i.properties.push_back(makeFloat("maxDeath", "Max death", fri->getMaxDeathValue(), 0.01f, -1.0e6f, 1.0e6f));
            }
            else if (SPK::FloatSimpleInterpolator* simp = dynamic_cast<SPK::FloatSimpleInterpolator*>(fi))
            {
                i.properties.push_back(makeFloat("birth", "Birth", simp->getBirthValue(), 0.01f, -1000.0f, 1000.0f));
                i.properties.push_back(makeFloat("death", "Death", simp->getDeathValue(), 0.01f, -1000.0f, 1000.0f));
            }
            else if (SPK::FloatRandomInitializer* rnd = dynamic_cast<SPK::FloatRandomInitializer*>(fi))
            {
                i.properties.push_back(makeFloat("min", "Min", rnd->getMinValue(), 0.01f, -1000.0f, 1000.0f));
                i.properties.push_back(makeFloat("max", "Max", rnd->getMaxValue(), 0.01f, -1000.0f, 1000.0f));
            }
            else if (SPK::FloatDefaultInitializer* def = dynamic_cast<SPK::FloatDefaultInitializer*>(fi))
                i.properties.push_back(makeFloat("value", "Value", def->getDefaultValue(), 0.01f, -1.0e6f, 1.0e6f));
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
        const int minCapacity = (std::max)(1, static_cast<int>(group->getNbParticles()));
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
            if (!colorRef)
                continue;
            SPK::ColorInterpolator* colorInterp = colorRef.get();
            if (SPK::ColorGraphInterpolator* colorGraph = dynamic_cast<SPK::ColorGraphInterpolator*>(colorInterp))
                applyColorGraphProperties(i.properties, colorGraph);
            else if (SPK::ColorRandomInterpolator* colorRnd = dynamic_cast<SPK::ColorRandomInterpolator*>(colorInterp))
            {
                colorRnd->setValues(
                    readColor4(i.properties, "minBirth", colorRnd->getMinBirthValue()),
                    readColor4(i.properties, "maxBirth", colorRnd->getMaxBirthValue()),
                    readColor4(i.properties, "minDeath", colorRnd->getMinDeathValue()),
                    readColor4(i.properties, "maxDeath", colorRnd->getMaxDeathValue()));
            }
            else if (SPK::ColorSimpleInterpolator* simpleColor = dynamic_cast<SPK::ColorSimpleInterpolator*>(colorInterp))
            {
                const SPK::Color birth = readColor4(i.properties, "birth", simpleColor->getBirthValue());
                const SPK::Color death = readColor4(i.properties, "death", simpleColor->getDeathValue());
                simpleColor->setValues(birth, death);
            }
            else if (SPK::ColorRandomInitializer* colorRndInit = dynamic_cast<SPK::ColorRandomInitializer*>(colorInterp))
            {
                colorRndInit->setValues(
                    readColor4(i.properties, "min", colorRndInit->getMinValue()),
                    readColor4(i.properties, "max", colorRndInit->getMaxValue()));
            }
            else if (SPK::ColorDefaultInitializer* colorDef = dynamic_cast<SPK::ColorDefaultInitializer*>(colorInterp))
                colorDef->setDefaultValue(readColor4(i.properties, "value", colorDef->getDefaultValue()));
            continue;
        }

        SPK::Ref<SPK::FloatInterpolator> interpRef = groupRef->getParamInterpolator(static_cast<SPK::Param>(i.param));
        if (!interpRef)
            continue;
        SPK::FloatInterpolator* fi = interpRef.get();
        if (SPK::FloatGraphInterpolator* fg = dynamic_cast<SPK::FloatGraphInterpolator*>(fi))
            applyFloatGraphProperties(i.properties, fg);
        else if (SPK::FloatRandomInterpolator* fri = dynamic_cast<SPK::FloatRandomInterpolator*>(fi))
        {
            fri->setValues(
                readFloat(i.properties, "minBirth", fri->getMinBirthValue()),
                readFloat(i.properties, "maxBirth", fri->getMaxBirthValue()),
                readFloat(i.properties, "minDeath", fri->getMinDeathValue()),
                readFloat(i.properties, "maxDeath", fri->getMaxDeathValue()));
        }
        else if (SPK::FloatSimpleInterpolator* simp = dynamic_cast<SPK::FloatSimpleInterpolator*>(fi))
        {
            simp->setValues(
                readFloat(i.properties, "birth", simp->getBirthValue()),
                readFloat(i.properties, "death", simp->getDeathValue()));
        }
        else if (SPK::FloatRandomInitializer* rnd = dynamic_cast<SPK::FloatRandomInitializer*>(fi))
        {
            float minV = readFloat(i.properties, "min", rnd->getMinValue());
            float maxV = readFloat(i.properties, "max", rnd->getMaxValue());
            if (minV > maxV)
                std::swap(minV, maxV);
            rnd->setValues(minV, maxV);
        }
        else if (SPK::FloatDefaultInitializer* def = dynamic_cast<SPK::FloatDefaultInitializer*>(fi))
            def->setDefaultValue(readFloat(i.properties, "value", def->getDefaultValue()));
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

    auto drawEmitterNode = [&changed, &system](EmitterNode& e) -> bool {
        const std::string emitterScope = "emitter." + std::to_string(e.groupIndex) + "." + std::to_string(e.emitterIndex);
        const bool opened = ImGui::TreeNode((e.title + "##" + emitterScope).c_str());
        ImGui::SameLine();
        if (ImGui::Button(("-##del." + emitterScope).c_str()))
        {
            if (e.groupIndex >= 0 && static_cast<size_t>(e.groupIndex) < system->getNbGroups())
            {
                SPK::Ref<SPK::Group> gr = system->getGroup(static_cast<size_t>(e.groupIndex));
                if (gr && e.emitterIndex >= 0 && static_cast<size_t>(e.emitterIndex) < gr->getNbEmitters())
                    gr->removeEmitter(gr->getEmitter(static_cast<size_t>(e.emitterIndex)));
            }
            if (opened)
                ImGui::TreePop();
            return true;
        }
        if (!opened)
            return false;

        Property* flowProp = findProperty(e.properties, "flow");
        Property* tankMinProp = findProperty(e.properties, "tankMin");
        Property* tankMaxProp = findProperty(e.properties, "tankMax");

        for (size_t p = 0; p < e.properties.size(); ++p)
        {
            Property& prop = e.properties[p];
            if (prop.key == "flow" || prop.key == "tankMin" || prop.key == "tankMax")
                continue;
            changed = drawProperty(prop, emitterScope) || changed;
        }

        if (flowProp && flowProp->type == PropertyType::Float)
        {
            float flowV = std::get<float>(flowProp->value);
            const std::string flowLabel = flowProp->label + "##" + emitterScope + "." + flowProp->key;
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
            const std::string minLabel = tankMinProp->label + "##" + emitterScope + "." + tankMinProp->key;
            const std::string maxLabel = tankMaxProp->label + "##" + emitterScope + "." + tankMaxProp->key;

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

                const std::string zoneScope = emitterScope + ".zone";
                for (size_t zi = 0; zi < e.zoneProperties.size(); ++zi)
                    changed = drawProperty(e.zoneProperties[zi], zoneScope) || changed;
            }
            ImGui::TreePop();
        }

        ImGui::TreePop();
        return false;
    };

    auto drawSimpleNode = [&changed](const std::string& treeId,
                                     const std::string& title,
                                     std::vector<Property>& properties,
                                     const std::string& propertyScope,
                                     const std::string& deleteButtonId,
                                     bool& deleteClicked) {
        const bool opened = ImGui::TreeNode((title + treeId).c_str());
        ImGui::SameLine();
        if (ImGui::Button(deleteButtonId.c_str()))
        {
            deleteClicked = true;
            if (opened)
                ImGui::TreePop();
            return;
        }
        if (!opened)
            return;
        if (properties.empty())
            ImGui::TextUnformatted("No exposed editable properties for this type yet.");
        for (size_t p = 0; p < properties.size(); ++p)
            changed = drawProperty(properties[p], propertyScope) || changed;
        ImGui::TreePop();
    };

    if (ImGui::CollapsingHeader("Particle System Tree", ImGuiTreeNodeFlags_DefaultOpen))
    {
        groupAddUi_.resize(data_.groups.size());
        bool pendingTreeResync = false;

        for (size_t i = 0; i < data_.groups.size(); ++i)
        {
            GroupNode& g = data_.groups[i];
            GroupAddUiState& addSt = groupAddUi_[i];
            addSt.modifierKind = std::clamp(addSt.modifierKind, 0, kNbModifierSpawns - 1);
            addSt.emitterKind = std::clamp(addSt.emitterKind, 0, kNbEmitterSpawns - 1);
            addSt.rendererKind = std::clamp(addSt.rendererKind, 0, kNbRendererSpawns - 1);
            addSt.colorInterpKind = std::clamp(addSt.colorInterpKind, 0, kNbColorInterpSpawns - 1);
            addSt.floatInterpParam = std::clamp(addSt.floatInterpParam, 0, kNbParamSlots - 1);
            addSt.floatInterpKind = std::clamp(addSt.floatInterpKind, 0, kNbFloatInterpSpawns - 1);

            SPK::Ref<SPK::Group> liveGroupRef;
            if (g.groupIndex >= 0 && static_cast<size_t>(g.groupIndex) < system->getNbGroups())
                liveGroupRef = system->getGroup(static_cast<size_t>(g.groupIndex));
            SPK::Group* liveGroupPtr = liveGroupRef.get();

            const bool groupOpened = ImGui::TreeNode((g.title + "##group").c_str());
            ImGui::SameLine();
            if (ImGui::Button(("-##del.group." + std::to_string(g.groupIndex)).c_str()) && liveGroupRef)
            {
                system->removeGroup(liveGroupRef);
                pendingTreeResync = true;
                if (groupOpened)
                    ImGui::TreePop();
                continue;
            }
            if (!groupOpened)
                continue;

            if (ImGui::TreeNode(("Group Properties##groupProps." + g.title).c_str()))
            {
                if (liveGroupPtr)
                    ImGui::Text("Particles: %zu", liveGroupPtr->getNbParticles());
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
                            const int minCap = liveGroupPtr ? (std::max)(1, static_cast<int>(liveGroupPtr->getNbParticles())) : 1;
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

            if (ImGui::TreeNode(("Renderer##rendererTree." + g.title).c_str()))
            {
                bool hasRenderer = false;
                for (size_t ri = 0; ri < data_.renderers.size(); ++ri)
                {
                    RendererNode& renderer = data_.renderers[ri];
                    if (renderer.groupIndex != g.groupIndex)
                        continue;
                    hasRenderer = true;
                    bool deleteRenderer = false;
                    drawSimpleNode("##renderer",
                                   renderer.title,
                                   renderer.properties,
                                   "renderer." + std::to_string(renderer.groupIndex),
                                   "-##del.renderer." + std::to_string(renderer.groupIndex),
                                   deleteRenderer);
                    if (deleteRenderer && liveGroupPtr)
                    {
                        liveGroupPtr->setRenderer(SPK_NULL_REF);
                        pendingTreeResync = true;
                        break;
                    }
                }

                if (!hasRenderer)
                {
                    ImGui::TextDisabled("(none)");
                    ImGui::PushID(static_cast<int>(static_cast<int>(i) * 10 + 1));
                    ImGui::TextUnformatted("New");
                    ImGui::SameLine();
                    {
                        const float addBtnW = ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                        float comboW = ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x;
                        if (comboW < 80.0f)
                            comboW = 80.0f;
                        ImGui::SetNextItemWidth(comboW);
                    }
                    const std::string rendItems = makeImGuiComboItems(kRendererSpawnLabels, kNbRendererSpawns);
                    ImGui::Combo("##rendererSpawn", &addSt.rendererKind, rendItems.c_str());
                    ImGui::SameLine();
                    if (ImGui::Button("+##addRenderer") && liveGroupPtr)
                    {
                        liveGroupPtr->setRenderer(spawnRenderer(addSt.rendererKind));
                        pendingTreeResync = true;
                    }
                    ImGui::PopID();
                }
                else if (liveGroupPtr)
                {
                    int currentKind = rendererSpawnKind(liveGroupPtr->getRenderer().get());
                    ImGui::TextUnformatted("Type");
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
                    const std::string rendItems = makeImGuiComboItems(kRendererSpawnLabels, kNbRendererSpawns);
                    if (ImGui::Combo(("##rendererTypeSwitch." + g.title).c_str(), &currentKind, rendItems.c_str()))
                    {
                        liveGroupPtr->setRenderer(spawnRenderer(currentKind));
                        pendingTreeResync = true;
                    }
                }
                ImGui::TreePop();
            }

            if (ImGui::TreeNode(("Emitters##emitters." + g.title).c_str()))
            {
                for (size_t ei = 0; ei < data_.emitters.size(); ++ei)
                {
                    EmitterNode& e = data_.emitters[ei];
                    if (e.groupIndex != g.groupIndex)
                        continue;
                    if (drawEmitterNode(e))
                    {
                        pendingTreeResync = true;
                        break;
                    }
                }
                bool anyEmitter = false;
                for (size_t ei = 0; ei < data_.emitters.size(); ++ei)
                {
                    if (data_.emitters[ei].groupIndex == g.groupIndex)
                    {
                        anyEmitter = true;
                        break;
                    }
                }
                if (!anyEmitter)
                    ImGui::TextDisabled("(none)");

                ImGui::PushID(static_cast<int>(static_cast<int>(i) * 10 + 2));
                ImGui::TextUnformatted("New");
                ImGui::SameLine();
                {
                    const float addBtnW = ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                    float comboW = ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x;
                    if (comboW < 80.0f)
                        comboW = 80.0f;
                    ImGui::SetNextItemWidth(comboW);
                }
                const std::string emItems = makeImGuiComboItems(kEmitterSpawnLabels, kNbEmitterSpawns);
                ImGui::Combo("##emitterSpawn", &addSt.emitterKind, emItems.c_str());
                ImGui::SameLine();
                if (ImGui::Button("+##addEmitter") && liveGroupPtr)
                {
                    liveGroupPtr->addEmitter(spawnEmitter(addSt.emitterKind));
                    pendingTreeResync = true;
                }
                ImGui::PopID();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode(("Modifiers##modifiers." + g.title).c_str()))
            {
                for (size_t mi = 0; mi < data_.modifiers.size(); ++mi)
                {
                    ModifierNode& m = data_.modifiers[mi];
                    if (m.groupIndex != g.groupIndex)
                        continue;
                    bool deleteModifier = false;
                    drawSimpleNode("##modifier",
                                   m.title,
                                   m.properties,
                                   "modifier." + std::to_string(m.groupIndex) + "." + std::to_string(m.modifierIndex),
                                   "-##del.modifier." + std::to_string(m.groupIndex) + "." + std::to_string(m.modifierIndex),
                                   deleteModifier);
                    if (deleteModifier && liveGroupPtr &&
                        m.modifierIndex >= 0 &&
                        static_cast<size_t>(m.modifierIndex) < liveGroupPtr->getNbModifiers())
                    {
                        liveGroupPtr->removeModifier(liveGroupPtr->getModifier(static_cast<size_t>(m.modifierIndex)));
                        pendingTreeResync = true;
                        break;
                    }
                }
                bool anyMod = false;
                for (size_t mi = 0; mi < data_.modifiers.size(); ++mi)
                {
                    if (data_.modifiers[mi].groupIndex == g.groupIndex)
                    {
                        anyMod = true;
                        break;
                    }
                }
                if (!anyMod)
                    ImGui::TextDisabled("(none)");

                ImGui::PushID(static_cast<int>(static_cast<int>(i) * 10 + 3));
                ImGui::TextUnformatted("New");
                ImGui::SameLine();
                {
                    const float addBtnW = ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                    float comboW = ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x;
                    if (comboW < 80.0f)
                        comboW = 80.0f;
                    ImGui::SetNextItemWidth(comboW);
                }
                const std::string modItems = makeImGuiComboItems(kModifierSpawnLabels, kNbModifierSpawns);
                ImGui::Combo("##modifierSpawn", &addSt.modifierKind, modItems.c_str());
                ImGui::SameLine();
                const char* selectedModifierType = kModifierSpawnLabels[addSt.modifierKind];
                bool modifierTypeExists = false;
                for (size_t mi = 0; mi < data_.modifiers.size(); ++mi)
                {
                    if (data_.modifiers[mi].groupIndex == g.groupIndex &&
                        data_.modifiers[mi].typeName == selectedModifierType)
                    {
                        modifierTypeExists = true;
                        break;
                    }
                }
                ImGui::BeginDisabled(!liveGroupPtr || modifierTypeExists);
                if (ImGui::Button("+##addModifier") && liveGroupPtr)
                {
                    liveGroupPtr->addModifier(spawnModifier(addSt.modifierKind));
                    pendingTreeResync = true;
                }
                ImGui::EndDisabled();
                if (modifierTypeExists)
                    ImGui::TextDisabled("  (exists)");
                ImGui::PopID();
                ImGui::TreePop();
            }

            if (ImGui::TreeNode(("Interpolators##interps." + g.title).c_str()))
            {
                for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
                {
                    InterpolatorNode& interp = data_.interpolators[ii];
                    if (interp.groupIndex != g.groupIndex)
                        continue;
                    bool deleteInterp = false;
                    drawSimpleNode("##interp",
                                   interp.title,
                                   interp.properties,
                                   "interp." + std::to_string(interp.groupIndex) + "." + std::to_string(interp.param),
                                   "-##del.interp." + std::to_string(interp.groupIndex) + "." + std::to_string(interp.param),
                                   deleteInterp);
                    if (deleteInterp && liveGroupPtr)
                    {
                        if (interp.param == -1)
                            liveGroupPtr->setColorInterpolator(SPK_NULL_REF);
                        else
                            liveGroupPtr->setParamInterpolator(static_cast<SPK::Param>(interp.param), SPK_NULL_REF);
                        pendingTreeResync = true;
                        break;
                    }
                }
                bool anyInterp = false;
                for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
                {
                    if (data_.interpolators[ii].groupIndex == g.groupIndex)
                    {
                        anyInterp = true;
                        break;
                    }
                }
                if (!anyInterp)
                    ImGui::TextDisabled("(none)");

                ImGui::PushID(static_cast<int>(static_cast<int>(i) * 10 + 4));
                ImGui::TextUnformatted("New color");
                ImGui::SameLine();
                {
                    const float addBtnW = ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                    float comboW = ImGui::GetContentRegionAvail().x - addBtnW - ImGui::GetStyle().ItemSpacing.x;
                    if (comboW < 80.0f)
                        comboW = 80.0f;
                    ImGui::SetNextItemWidth(comboW);
                }
                const std::string cItems = makeImGuiComboItems(kColorInterpSpawnLabels, kNbColorInterpSpawns);
                ImGui::Combo("##colorInterpSpawn", &addSt.colorInterpKind, cItems.c_str());
                ImGui::SameLine();
                const char* selectedColorInterpType = kColorInterpSpawnLabels[addSt.colorInterpKind];
                bool colorInterpTypeExists = false;
                for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
                {
                    if (data_.interpolators[ii].groupIndex == g.groupIndex &&
                        data_.interpolators[ii].typeName == selectedColorInterpType)
                    {
                        colorInterpTypeExists = true;
                        break;
                    }
                }
                ImGui::BeginDisabled(!liveGroupPtr || colorInterpTypeExists);
                if (ImGui::Button("+##addColorInterp") && liveGroupPtr)
                {
                    liveGroupPtr->setColorInterpolator(spawnColorInterpolator(addSt.colorInterpKind));
                    pendingTreeResync = true;
                }
                ImGui::EndDisabled();
                if (colorInterpTypeExists)
                    ImGui::TextDisabled("  (exists)");
                ImGui::PopID();

                ImGui::PushID(static_cast<int>(static_cast<int>(i) * 10 + 5));
                ImGui::TextUnformatted("New float param");
                ImGui::SameLine();
                {
                    const float addBtnW = ImGui::CalcTextSize("+").x + ImGui::GetStyle().FramePadding.x * 2.0f;
                    const float spacing = ImGui::GetStyle().ItemSpacing.x;
                    float remain = ImGui::GetContentRegionAvail().x - addBtnW - spacing * 2.0f;
                    if (remain < 180.0f)
                        remain = 180.0f;
                    float paramW = remain * 0.30f;
                    if (paramW < 90.0f)
                        paramW = 90.0f;
                    if (paramW > 180.0f)
                        paramW = 180.0f;
                    float interpW = remain - paramW;
                    if (interpW < 90.0f)
                    {
                        interpW = 90.0f;
                        paramW = remain - interpW;
                        if (paramW < 90.0f)
                            paramW = 90.0f;
                    }
                    ImGui::SetNextItemWidth(paramW);
                    const std::string pItems = makeImGuiComboItems(kParamSlotLabels, kNbParamSlots);
                    ImGui::Combo("##floatParamSlot", &addSt.floatInterpParam, pItems.c_str());
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(interpW);
                    const std::string fItems = makeImGuiComboItems(kFloatInterpSpawnLabels, kNbFloatInterpSpawns);
                    ImGui::Combo("##floatInterpSpawn", &addSt.floatInterpKind, fItems.c_str());
                }
                ImGui::SameLine();
                const char* selectedFloatInterpType = kFloatInterpSpawnLabels[addSt.floatInterpKind];
                bool floatInterpTypeExists = false;
                for (size_t ii = 0; ii < data_.interpolators.size(); ++ii)
                {
                    if (data_.interpolators[ii].groupIndex == g.groupIndex &&
                        data_.interpolators[ii].typeName == selectedFloatInterpType)
                    {
                        floatInterpTypeExists = true;
                        break;
                    }
                }
                ImGui::BeginDisabled(!liveGroupPtr || floatInterpTypeExists);
                if (ImGui::Button("+##addFloatInterp") && liveGroupPtr)
                {
                    const SPK::Param p = static_cast<SPK::Param>(addSt.floatInterpParam);
                    liveGroupPtr->setParamInterpolator(p, spawnFloatInterpolator(addSt.floatInterpKind));
                    pendingTreeResync = true;
                }
                ImGui::EndDisabled();
                if (floatInterpTypeExists)
                    ImGui::TextDisabled("  (exists)");
                ImGui::PopID();

                ImGui::TreePop();
            }

            ImGui::TreePop();
        }

        ImGui::TextUnformatted("New group");
        ImGui::SameLine();
        if (ImGui::Button("+##addSystemGroup"))
        {
            spawnNewGroup(*system);
            pendingTreeResync = true;
        }

        if (pendingTreeResync)
            extractFromSystem(*system);
    }

    if (changed)
        applyToSystem(*system);

    ImGui::Text("Source File : %s", data_.sourceFilePath.c_str());
}

} // namespace spark_editor
