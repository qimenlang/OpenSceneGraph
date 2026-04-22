#include "SparkEditorParticleIO.h"

#include <osg/Image>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>

#include <SPARK_GL.h>

#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace spark_editor
{
namespace
{

std::unordered_map<unsigned int, std::string> gTextureIdToBasename;

/** Resolve texture path from renderer name: absolute paths unchanged, else relative to particle file directory. */
std::string ResolveTextureImagePath(const std::string& particleFileDir, const std::string& nameFromRenderer)
{
    if (nameFromRenderer.empty())
        return std::string();
    if (osgDB::isAbsolutePath(nameFromRenderer))
        return nameFromRenderer;
    return osgDB::concatPaths(particleFileDir, nameFromRenderer);
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
                    qr->setName(it->second);
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
                    pr->setName(it->second);
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
    std::unordered_map<std::string, GLuint> resolvedPathToTex;

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
                continue;
            const std::string texPathInName = qr->getName();
            if (texPathInName.empty())
                continue;
            const std::string full = ResolveTextureImagePath(dir, texPathInName);

            GLuint tex = 0u;
            const auto cached = resolvedPathToTex.find(full);
            if (cached != resolvedPathToTex.end())
                tex = cached->second;
            else
            {
                osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile(full);
                if (!image.valid() || !image->data())
                {
                    std::cout << "SparkEditor: failed to load texture image for renderer: " << full << '\n';
                    continue;
                }
                tex = CreateTextureFromOsgImage(image.get());
                if (tex != 0u)
                {
                    resolvedPathToTex.emplace(full, tex);
                    RegisterDemoTextureFile(static_cast<unsigned int>(tex), osgDB::getSimpleFileName(full));
                }
            }
            if (tex != 0u)
                qr->setTexture(tex);

            ReseedStaticLoadedGroup(group, i, loadedParticlePath);
        }
        else if (SPK::GL::GLPointRenderer* pr = dynamic_cast<SPK::GL::GLPointRenderer*>(renderer))
        {
            if (pr->getType() != SPK::POINT_TYPE_SPRITE)
                continue;
            const std::string texPathInName = pr->getName();
            if (texPathInName.empty())
                continue;
            const std::string full = ResolveTextureImagePath(dir, texPathInName);

            GLuint tex = 0u;
            const auto cached = resolvedPathToTex.find(full);
            if (cached != resolvedPathToTex.end())
                tex = cached->second;
            else
            {
                osg::ref_ptr<osg::Image> image = osgDB::readRefImageFile(full);
                if (!image.valid() || !image->data())
                {
                    std::cout << "SparkEditor: failed to load texture image for point renderer: " << full << '\n';
                    continue;
                }
                tex = CreateTextureFromOsgImage(image.get());
                if (tex != 0u)
                {
                    resolvedPathToTex.emplace(full, tex);
                    RegisterDemoTextureFile(static_cast<unsigned int>(tex), osgDB::getSimpleFileName(full));
                }
            }
            if (tex != 0u)
                pr->setTexture(tex);

            ReseedStaticLoadedGroup(group, i, loadedParticlePath);
        }
    }
}

} // namespace spark_editor
