#include "SparkEditorParticleIO.h"

#include <osg/Image>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <SPARK_GL.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace spark_editor
{
namespace
{

std::unordered_map<unsigned int, std::string> gTextureIdToBasename;

struct EncodedRendererName
{
    std::string basename;
    std::unordered_map<std::string, std::string> kv;
};

std::vector<std::string> Split(const std::string& text, char delim)
{
    std::vector<std::string> parts;
    std::string current;
    for (char ch : text)
    {
        if (ch == delim)
        {
            parts.push_back(current);
            current.clear();
        }
        else
            current.push_back(ch);
    }
    parts.push_back(current);
    return parts;
}

std::string ToStringFloat(float value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

float ToFloatOr(const std::unordered_map<std::string, std::string>& kv, const char* key, float fallback)
{
    const auto it = kv.find(key);
    if (it == kv.end())
        return fallback;
    try { return std::stof(it->second); } catch (...) { return fallback; }
}

int ToIntOr(const std::unordered_map<std::string, std::string>& kv, const char* key, int fallback)
{
    const auto it = kv.find(key);
    if (it == kv.end())
        return fallback;
    try { return std::stoi(it->second); } catch (...) { return fallback; }
}

bool ToBoolOr(const std::unordered_map<std::string, std::string>& kv, const char* key, bool fallback)
{
    const auto it = kv.find(key);
    if (it == kv.end())
        return fallback;
    if (it->second == "1" || it->second == "true")
        return true;
    if (it->second == "0" || it->second == "false")
        return false;
    return fallback;
}

EncodedRendererName DecodeRendererName(const std::string& name)
{
    EncodedRendererName out;
    const std::string marker = "|se:";
    const size_t markerPos = name.find(marker);
    if (markerPos == std::string::npos)
    {
        out.basename = name;
        return out;
    }

    out.basename = name.substr(0, markerPos);
    const std::string payload = name.substr(markerPos + marker.size());
    for (const std::string& item : Split(payload, ','))
    {
        if (item.empty())
            continue;
        const size_t eq = item.find('=');
        if (eq == std::string::npos || eq == 0 || eq == item.size() - 1)
            continue;
        out.kv[item.substr(0, eq)] = item.substr(eq + 1);
    }
    return out;
}

void AppendEncodedKV(std::ostringstream& os, bool& first, const std::string& key, const std::string& value)
{
    if (!first)
        os << ",";
    first = false;
    os << key << "=" << value;
}

std::string InferBlendCode(const SPK::GL::GLRenderer* gl)
{
    if (!gl)
        return "n";
    if (!gl->isBlendingEnabled())
        return "n";
    if (gl->getSrcBlendingFunction() == GL_SRC_ALPHA && gl->getDestBlendingFunction() == GL_ONE)
        return "a";
    if (gl->getSrcBlendingFunction() == GL_SRC_ALPHA && gl->getDestBlendingFunction() == GL_ONE_MINUS_SRC_ALPHA)
        return "l";
    return "c";
}

void ApplyBlendCode(SPK::GL::GLRenderer* gl, const std::unordered_map<std::string, std::string>& kv)
{
    if (!gl)
        return;
    const auto it = kv.find("bm");
    if (it == kv.end())
        return;
    if (it->second == "n")
        gl->setBlendMode(SPK::BLEND_MODE_NONE);
    else if (it->second == "a")
        gl->setBlendMode(SPK::BLEND_MODE_ADD);
    else if (it->second == "l")
        gl->setBlendMode(SPK::BLEND_MODE_ALPHA);
}

std::string EncodeQuadRendererName(const std::string& base, SPK::GL::GLQuadRenderer* qr)
{
    std::ostringstream os;
    os << base << "|se:";
    bool first = true;
    AppendEncodedKV(os, first, "tm", std::to_string(static_cast<int>(qr->getTexturingMode())));
    AppendEncodedKV(os, first, "ax", std::to_string(static_cast<int>(qr->getAtlasDimensionX())));
    AppendEncodedKV(os, first, "ay", std::to_string(static_cast<int>(qr->getAtlasDimensionY())));
    AppendEncodedKV(os, first, "sx", ToStringFloat(qr->getScaleX()));
    AppendEncodedKV(os, first, "sy", ToStringFloat(qr->getScaleY()));
    AppendEncodedKV(os, first, "lo", std::to_string(static_cast<int>(qr->getLookOrientation())));
    AppendEncodedKV(os, first, "uo", std::to_string(static_cast<int>(qr->getUpOrientation())));
    AppendEncodedKV(os, first, "la", std::to_string(static_cast<int>(qr->getLockedAxis())));
    AppendEncodedKV(os, first, "lvx", ToStringFloat(qr->lookVector.x));
    AppendEncodedKV(os, first, "lvy", ToStringFloat(qr->lookVector.y));
    AppendEncodedKV(os, first, "lvz", ToStringFloat(qr->lookVector.z));
    AppendEncodedKV(os, first, "uvx", ToStringFloat(qr->upVector.x));
    AppendEncodedKV(os, first, "uvy", ToStringFloat(qr->upVector.y));
    AppendEncodedKV(os, first, "uvz", ToStringFloat(qr->upVector.z));
    AppendEncodedKV(os, first, "bm", InferBlendCode(dynamic_cast<SPK::GL::GLRenderer*>(qr)));
    return os.str();
}

std::string EncodePointRendererName(const std::string& base, SPK::GL::GLPointRenderer* pr)
{
    std::ostringstream os;
    os << base << "|se:";
    bool first = true;
    AppendEncodedKV(os, first, "pt", std::to_string(static_cast<int>(pr->getType())));
    AppendEncodedKV(os, first, "ws", pr->isWorldSizeEnabled() ? "1" : "0");
    AppendEncodedKV(os, first, "ss", ToStringFloat(pr->getScreenSize()));
    AppendEncodedKV(os, first, "sc", ToStringFloat(pr->getWorldScale()));
    AppendEncodedKV(os, first, "bm", InferBlendCode(dynamic_cast<SPK::GL::GLRenderer*>(pr)));
    return os.str();
}

void ApplyQuadRendererMeta(SPK::GL::GLQuadRenderer* qr, const std::unordered_map<std::string, std::string>& kv)
{
    qr->setTexturingMode(static_cast<SPK::TextureMode>(ToIntOr(kv, "tm", static_cast<int>(qr->getTexturingMode()))));
    qr->setAtlasDimensions(static_cast<size_t>(ToIntOr(kv, "ax", static_cast<int>(qr->getAtlasDimensionX()))),
                           static_cast<size_t>(ToIntOr(kv, "ay", static_cast<int>(qr->getAtlasDimensionY()))));
    qr->setScale(ToFloatOr(kv, "sx", qr->getScaleX()), ToFloatOr(kv, "sy", qr->getScaleY()));
    qr->setOrientation(static_cast<SPK::LookOrientation>(ToIntOr(kv, "lo", static_cast<int>(qr->getLookOrientation()))),
                       static_cast<SPK::UpOrientation>(ToIntOr(kv, "uo", static_cast<int>(qr->getUpOrientation()))),
                       static_cast<SPK::LockedAxis>(ToIntOr(kv, "la", static_cast<int>(qr->getLockedAxis()))));
    qr->lookVector.set(ToFloatOr(kv, "lvx", qr->lookVector.x), ToFloatOr(kv, "lvy", qr->lookVector.y), ToFloatOr(kv, "lvz", qr->lookVector.z));
    qr->upVector.set(ToFloatOr(kv, "uvx", qr->upVector.x), ToFloatOr(kv, "uvy", qr->upVector.y), ToFloatOr(kv, "uvz", qr->upVector.z));
    ApplyBlendCode(dynamic_cast<SPK::GL::GLRenderer*>(qr), kv);
}

void ApplyPointRendererMeta(SPK::GL::GLPointRenderer* pr, const std::unordered_map<std::string, std::string>& kv)
{
    pr->setType(static_cast<SPK::PointType>(ToIntOr(kv, "pt", static_cast<int>(pr->getType()))));
    pr->enableWorldSize(ToBoolOr(kv, "ws", pr->isWorldSizeEnabled()));
    if (pr->isWorldSizeEnabled())
        pr->setWorldScale(ToFloatOr(kv, "sc", pr->getWorldScale()));
    else
        pr->setScreenSize(ToFloatOr(kv, "ss", pr->getScreenSize()));
    ApplyBlendCode(dynamic_cast<SPK::GL::GLRenderer*>(pr), kv);
}

SPK::Ref<SPK::Zone> FindSeedZoneFromGroup(const SPK::Ref<SPK::Group>& group)
{
    if (!group)
        return SPK::Ref<SPK::Zone>();
    const size_t modifierCount = group->getNbModifiers();
    for (size_t idx = 0; idx < modifierCount; ++idx)
    {
        const SPK::Ref<SPK::Modifier>& modifier = group->getModifier(idx);
        if (!modifier)
            continue;
        if (SPK::ZonedModifier* zoned = dynamic_cast<SPK::ZonedModifier*>(modifier.get()))
        {
            const SPK::Ref<SPK::Zone>& zone = zoned->getZone();
            if (zone)
                return zone;
        }
    }
    return SPK::Ref<SPK::Zone>();
}

void ReseedStaticLoadedGroup(const SPK::Ref<SPK::Group>& group, size_t groupIndex, const std::string& loadedParticlePath)
{
    (void)groupIndex;
    (void)loadedParticlePath;
    if (!group)
        return;
    const bool shouldReseed = group->isImmortal() && group->getNbEmitters() == 0 && group->getNbParticles() == 0 && group->getCapacity() > 0;
    if (!shouldReseed)
        return;

    const SPK::Ref<SPK::Zone> seedZone = FindSeedZoneFromGroup(group);
    if (seedZone)
        group->addParticles(group->getCapacity(), seedZone, SPK::Vector3D());
    else
        group->addParticles(group->getCapacity(), SPK::Point::create(), SPK::Vector3D());
    group->flushBufferedParticles();
}

GLuint CreateTextureFromOsgImage(osg::Image* image)
{
    if (!image || !image->data())
        return 0;

    GLuint textureId = 0;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D,
        0,
        image->getPixelFormat(),
        image->s(),
        image->t(),
        0,
        image->getPixelFormat(),
        image->getDataType(),
        image->data());
    glBindTexture(GL_TEXTURE_2D, 0);

    return textureId;
}

} // namespace

void RegisterDemoTextureFile(unsigned int textureId, const std::string& sparkResFileName)
{
    if (textureId == 0u)
        return;
    const std::string base = osgDB::getSimpleFileName(sparkResFileName);
    if (!base.empty())
        gTextureIdToBasename[textureId] = base;
}

bool PrepareParticleSaveWithDemoTextures(const SPK::Ref<SPK::System>& system, const std::string& savePath)
{
    if (!system)
        return false;

    const std::string outDir = osgDB::getFilePath(savePath);
    std::unordered_set<std::string> basenamesToCopy;

    const size_t nb = system->getNbGroups();
    for (size_t i = 0; i < nb; ++i)
    {
        const SPK::Ref<SPK::Group>& group = system->getGroup(i);
        SPK::Renderer* renderer = group->getRenderer().get();
        if (!renderer)
            continue;

        if (SPK::GL::GLQuadRenderer* qr = dynamic_cast<SPK::GL::GLQuadRenderer*>(renderer))
        {
            if (qr->getTexturingMode() == SPK::TEXTURE_MODE_2D && qr->getTexture() != 0u)
            {
                const auto it = gTextureIdToBasename.find(static_cast<unsigned int>(qr->getTexture()));
                if (it != gTextureIdToBasename.end())
                {
                    qr->setName(EncodeQuadRendererName(it->second, qr));
                    basenamesToCopy.insert(it->second);
                }
                else
                {
                    std::cout << "SparkEditor: GLQuadRenderer uses texture id " << qr->getTexture()
                              << " with no SparkRes registration; name left unset for save.\n";
                    qr->setName("");
                }
            }
            else
                qr->setName("");

        }
        else if (SPK::GL::GLPointRenderer* pr = dynamic_cast<SPK::GL::GLPointRenderer*>(renderer))
        {
            if (pr->getType() == SPK::POINT_TYPE_SPRITE && pr->getTexture() != 0u)
            {
                const auto it = gTextureIdToBasename.find(static_cast<unsigned int>(pr->getTexture()));
                if (it != gTextureIdToBasename.end())
                {
                    pr->setName(EncodePointRendererName(it->second, pr));
                    basenamesToCopy.insert(it->second);
                }
                else
                {
                    std::cout << "SparkEditor: GLPointRenderer uses texture id " << pr->getTexture()
                              << " with no SparkRes registration; name left unset for save.\n";
                    pr->setName("");
                }
            }
            else
                pr->setName("");

        }
    }

    for (const std::string& base : basenamesToCopy)
    {
        const std::string src = osgDB::findDataFile("SparkRes/" + base);
        if (src.empty())
        {
            std::cout << "SparkEditor: could not find SparkRes/" << base << " to copy beside save file.\n";
            continue;
        }
        const std::string dst = osgDB::concatPaths(outDir, base);
        const osgDB::CopyFileResult cr = osgDB::copyFile(src, dst);
        if (cr != osgDB::COPY_FILE_OK && cr != osgDB::COPY_FILE_SOURCE_EQUALS_DESTINATION)
            std::cout << "SparkEditor: copyFile SparkRes asset failed (" << base << ") code " << static_cast<int>(cr) << '\n';
    }

    return true;
}

void BindLoadedParticleTextures(const SPK::Ref<SPK::System>& system, const std::string& loadedParticlePath)
{
    if (!system)
        return;

    const std::string dir = osgDB::getFilePath(loadedParticlePath);
    std::unordered_map<std::string, GLuint> basenameToTex;

    const size_t nb = system->getNbGroups();
    for (size_t i = 0; i < nb; ++i)
    {
        const SPK::Ref<SPK::Group>& group = system->getGroup(i);
        SPK::Renderer* renderer = group->getRenderer().get();
        if (!renderer)
            continue;

        if (SPK::GL::GLQuadRenderer* qr = dynamic_cast<SPK::GL::GLQuadRenderer*>(renderer))
        {
            if (qr->getTexturingMode() != SPK::TEXTURE_MODE_2D)
                ;
            const EncodedRendererName encoded = DecodeRendererName(qr->getName());
            if (!encoded.kv.empty())
            {
                ApplyQuadRendererMeta(qr, encoded.kv);
            }
            if (qr->getTexturingMode() != SPK::TEXTURE_MODE_2D)
                continue;
            if (encoded.basename.empty())
                continue;

            GLuint tex = 0u;
            const auto cached = basenameToTex.find(encoded.basename);
            if (cached != basenameToTex.end())
                tex = cached->second;
            else
            {
                const std::string full = osgDB::concatPaths(dir, encoded.basename);
                osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile(full);
                if (!image.valid() || !image->data())
                {
                    std::cout << "SparkEditor: failed to load texture image for renderer: " << full << '\n';
                    continue;
                }
                tex = CreateTextureFromOsgImage(image.get());
                if (tex != 0u)
                {
                    basenameToTex.emplace(encoded.basename, tex);
                    RegisterDemoTextureFile(static_cast<unsigned int>(tex), encoded.basename);
                }
            }
            if (tex != 0u){
                qr->setTexture(tex);
            }

            ReseedStaticLoadedGroup(group, i, loadedParticlePath);
        }
        else if (SPK::GL::GLPointRenderer* pr = dynamic_cast<SPK::GL::GLPointRenderer*>(renderer))
        {
            if (pr->getType() != SPK::POINT_TYPE_SPRITE)
                ;
            const EncodedRendererName encoded = DecodeRendererName(pr->getName());
            if (!encoded.kv.empty())
            {
                ApplyPointRendererMeta(pr, encoded.kv);
            }
            if (pr->getType() != SPK::POINT_TYPE_SPRITE)
                continue;
            if (encoded.basename.empty())
                continue;

            GLuint tex = 0u;
            const auto cached = basenameToTex.find(encoded.basename);
            if (cached != basenameToTex.end())
                tex = cached->second;
            else
            {
                const std::string full = osgDB::concatPaths(dir, encoded.basename);
                osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile(full);
                if (!image.valid() || !image->data())
                {
                    std::cout << "SparkEditor: failed to load texture image for point renderer: " << full << '\n';
                    continue;
                }
                tex = CreateTextureFromOsgImage(image.get());
                if (tex != 0u)
                {
                    basenameToTex.emplace(encoded.basename, tex);
                    RegisterDemoTextureFile(static_cast<unsigned int>(tex), encoded.basename);
                }
            }
            if (tex != 0u)
                pr->setTexture(tex);

            ReseedStaticLoadedGroup(group, i, loadedParticlePath);
        }
    }
}

} // namespace spark_editor
