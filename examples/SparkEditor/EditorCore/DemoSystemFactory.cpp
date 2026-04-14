#include "DemoSystemFactory.h"

namespace spark_editor
{
namespace
{

SPK::Ref<SPK::GL::GLQuadRenderer> makeQuadRenderer(GLuint textureId, SPK::BlendMode blendMode)
{
    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = SPK::GL::GLQuadRenderer::create();
    renderer->setBlendMode(blendMode);
    renderer->enableRenderingOption(SPK::RENDERING_OPTION_DEPTH_WRITE, false);
    renderer->setTexture(textureId);
    renderer->setTexturingMode(SPK::TEXTURE_MODE_2D);
    renderer->setAtlasDimensions(2, 2);
    return renderer;
}

SPK::Ref<SPK::System> createSPKTest(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::SPKTest");

    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = makeQuadRenderer(textures.explosion, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::SphericEmitter> emitter = SPK::SphericEmitter::create(
        SPK::Vector3D(0.0f, 0.0f, -1.0f), 0.0f, 3.14159f / 4.0f,
        SPK::Point::create(), true, -1, 100.0f, 0.2f, 0.5f);

    SPK::Ref<SPK::Group> phantomGroup = system->createGroup(40);
    SPK::Ref<SPK::Group> trailGroup = system->createGroup(1000);
    SPK::Ref<SPK::Plane> ground = SPK::Plane::create(SPK::Vector3D(0.0f, -1.0f, 0.0f));

    phantomGroup->setLifeTime(5.0f, 5.0f);
    phantomGroup->setRadius(0.06f);
    phantomGroup->addEmitter(SPK::SphericEmitter::create(
        SPK::Vector3D(0.0f, 1.0f, 0.0f), 0.0f, 3.14159f / 4.0f,
        SPK::Point::create(SPK::Vector3D(0.0f, -1.0f, 0.0f)), true, -1, 2.0f, 1.2f, 2.0f));
    phantomGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -1.0f, 0.0f)));
    phantomGroup->addModifier(SPK::Obstacle::create(ground, 0.8f));
    phantomGroup->addModifier(SPK::EmitterAttacher::create(trailGroup, emitter, true));

    trailGroup->setLifeTime(0.5f, 1.0f);
    trailGroup->setRadius(0.06f);
    trailGroup->setRenderer(renderer);
    trailGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0xFF802080, 0xFF000000));
    trailGroup->setParamInterpolator(SPK::PARAM_TEXTURE_INDEX, SPK::FloatRandomInitializer::create(0.0f, 4.0f));
    trailGroup->setParamInterpolator(SPK::PARAM_ROTATION_SPEED, SPK::FloatRandomInitializer::create(-0.1f, 1.0f));
    trailGroup->setParamInterpolator(SPK::PARAM_ANGLE, SPK::FloatRandomInitializer::create(0.0f, 2.0f * 3.14159f));
    trailGroup->addModifier(SPK::Rotator::create());
    trailGroup->addModifier(SPK::Destroyer::create(ground));

    return system;
}

SPK::Ref<SPK::System> createSPKCollision(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::SPKCollision");

    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = makeQuadRenderer(textures.ball ? textures.ball : textures.point, SPK::BLEND_MODE_NONE);
    renderer->enableRenderingOption(SPK::RENDERING_OPTION_ALPHA_TEST, true);
    renderer->setAlphaTestThreshold(0.8f);

    SPK::Ref<SPK::Box> cube = SPK::Box::create(SPK::Vector3D(), SPK::Vector3D(1.4f, 1.4f, 1.4f));
    SPK::Ref<SPK::Group> group = system->createGroup(750);
    group->setImmortal(true);
    group->setRadius(0.06f);
    group->setRenderer(renderer);
    group->addModifier(SPK::Gravity::create());
    group->addModifier(SPK::Obstacle::create(cube, 0.8f, 0.9f, SPK::ZONE_TEST_INTERSECT));
    group->addModifier(SPK::Collider::create(0.8f));
    group->addModifier(SPK::Friction::create(0.2f));
    group->addParticles(750, cube, SPK::Vector3D());
    group->flushBufferedParticles();
    return system;
}

SPK::Ref<SPK::System> createSPKFlakes(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::SPKFlakes");
    SPK::Ref<SPK::GL::GLPointRenderer> renderer = SPK::GL::GLPointRenderer::create(1.0f);
    renderer->setBlendMode(SPK::BLEND_MODE_ADD);
    if (textures.point)
    {
        renderer->setType(SPK::POINT_TYPE_SPRITE);
        renderer->setTexture(textures.point);
        renderer->enableWorldSize(true);
    }

    SPK::Ref<SPK::Sphere> sphere = SPK::Sphere::create(SPK::Vector3D(), 1.0f);
    SPK::Ref<SPK::Group> group = system->createGroup(500000);
    group->setRadius(0.0f);
    group->setRenderer(renderer);
    group->setColorInterpolator(SPK::ColorDefaultInitializer::create(0xFFCC4C66));
    group->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -0.5f, 0.0f)));
    group->addModifier(SPK::Friction::create(0.2f));
    group->addModifier(SPK::Obstacle::create(sphere, 0.9f, 0.9f));
    group->setImmortal(true);
    group->addParticles(50000, sphere, SPK::Vector3D());
    group->flushBufferedParticles();
    return system;
}

SPK::Ref<SPK::System> createSPKExplosion(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::SPKExplosion");

    SPK::Ref<SPK::GL::GLQuadRenderer> smokeRenderer = makeQuadRenderer(textures.explosion, SPK::BLEND_MODE_ALPHA);
    SPK::Ref<SPK::GL::GLQuadRenderer> flameRenderer = makeQuadRenderer(textures.explosion, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::GL::GLQuadRenderer> flashRenderer = makeQuadRenderer(textures.flash ? textures.flash : textures.explosion, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::GL::GLQuadRenderer> sparkRenderer = makeQuadRenderer(textures.spark1 ? textures.spark1 : textures.spark2, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::GL::GLQuadRenderer> waveRenderer = makeQuadRenderer(textures.wave ? textures.wave : textures.explosion, SPK::BLEND_MODE_ALPHA);
    sparkRenderer->setOrientation(SPK::DIRECTION_ALIGNED);
    sparkRenderer->setScale(0.05f, 1.0f);
    waveRenderer->setOrientation(SPK::FIXED_ORIENTATION);
    waveRenderer->lookVector.set(0.0f, 1.0f, 0.0f);
    waveRenderer->upVector.set(1.0f, 0.0f, 0.0f);

    SPK::Ref<SPK::Sphere> explosionSphere = SPK::Sphere::create(SPK::Vector3D(), 0.4f);
    SPK::Ref<SPK::RandomEmitter> smokeEmitter = SPK::RandomEmitter::create();
    smokeEmitter->setZone(SPK::Sphere::create(SPK::Vector3D(), 0.6f), false);
    smokeEmitter->setTank(15);
    smokeEmitter->setFlow(-1);
    smokeEmitter->setForce(0.02f, 0.04f);

    SPK::Ref<SPK::NormalEmitter> flameEmitter = SPK::NormalEmitter::create();
    flameEmitter->setZone(explosionSphere);
    flameEmitter->setTank(15);
    flameEmitter->setFlow(-1);
    flameEmitter->setForce(0.06f, 0.1f);

    SPK::Ref<SPK::Group> smokeGroup = system->createGroup(15);
    smokeGroup->setLifeTime(2.5f, 3.0f);
    smokeGroup->setRenderer(smokeRenderer);
    smokeGroup->addEmitter(smokeEmitter);
    smokeGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0x33333399, 0x33333300));
    smokeGroup->setParamInterpolator(SPK::PARAM_SCALE, SPK::FloatRandomInterpolator::create(0.3f, 0.4f, 0.5f, 0.7f));
    smokeGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, 0.05f, 0.0f)));

    SPK::Ref<SPK::Group> flameGroup = system->createGroup(15);
    flameGroup->setLifeTime(1.5f, 2.0f);
    flameGroup->setRenderer(flameRenderer);
    flameGroup->addEmitter(flameEmitter);
    flameGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0xFF8033FF, 0x33333300));

    SPK::Ref<SPK::Group> flashGroup = system->createGroup(3);
    flashGroup->setLifeTime(0.2f, 0.2f);
    flashGroup->setRenderer(flashRenderer);
    flashGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0xFFFFFFFF, 0xFFFFFF00));
    flashGroup->addEmitter(SPK::StaticEmitter::create(SPK::Sphere::create(SPK::Vector3D(), 0.1f), true, 3, -1.0f));

    SPK::Ref<SPK::Group> sparkGroup = system->createGroup(400);
    sparkGroup->setLifeTime(1.0f, 3.0f);
    sparkGroup->setRenderer(sparkRenderer);
    sparkGroup->setParamInterpolator(SPK::PARAM_MASS, SPK::FloatRandomInitializer::create(0.5f, 2.5f));
    sparkGroup->setColorInterpolator(SPK::ColorRandomInterpolator::create(0xFFFFB2FF, 0xFFFFB2FF, 0xFF4C4C00, 0xFFFF4C00));
    sparkGroup->addEmitter(SPK::NormalEmitter::create(explosionSphere, true, 400, -1.0f, 0.4f, 1.0f, explosionSphere, true));
    sparkGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -0.1f, 0.0f)));
    sparkGroup->addModifier(SPK::Friction::create(0.4f));

    SPK::Ref<SPK::Group> waveGroup = system->createGroup(1);
    waveGroup->setLifeTime(0.8f, 0.8f);
    waveGroup->setRenderer(waveRenderer);
    waveGroup->setColorInterpolator(SPK::ColorSimpleInterpolator::create(0xFFFFFF20, 0xFFFFFF00));
    waveGroup->setParamInterpolator(SPK::PARAM_SCALE, SPK::FloatRandomInterpolator::create(0.0f, 0.0f, 3.0f, 3.0f));
    waveGroup->addEmitter(SPK::StaticEmitter::create(SPK::Point::create(), true, 1, -1.0f));

    return system;
}

SPK::Ref<SPK::System> createSPKIrrlichtTest(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::SPKTestIrrlicht");
    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = makeQuadRenderer(textures.flare ? textures.flare : textures.point, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::RandomEmitter> emitter1 = SPK::RandomEmitter::create(SPK::Point::create());
    emitter1->setForce(0.4f, 0.6f);
    emitter1->setFlow(200);
    SPK::Ref<SPK::Emitter> emitter2 = SPK::SPKObject::copy(emitter1);

    SPK::Ref<SPK::ColorGraphInterpolator> colorGraph = SPK::ColorGraphInterpolator::create();
    colorGraph->addEntry(0.0f, 0xFF000088);
    colorGraph->addEntry(0.5f, 0x00FF0088);
    colorGraph->addEntry(1.0f, 0x0000FF88);

    SPK::Ref<SPK::Group> group = system->createGroup(400);
    group->setRadius(0.15f);
    group->setLifeTime(0.5f, 1.0f);
    group->setColorInterpolator(colorGraph);
    group->setParamInterpolator(SPK::PARAM_SCALE, SPK::FloatRandomInterpolator::create(0.8f, 1.2f, 0.0f, 0.0f));
    group->setParamInterpolator(SPK::PARAM_ANGLE, SPK::FloatRandomInitializer::create(0.0f, 2 * 3.14159f));
    group->addEmitter(emitter1);
    group->addEmitter(emitter2);
    group->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -0.5f, 0.0f)));
    group->addModifier(SPK::Friction::create(0.2f));
    group->setRenderer(renderer);
    return system;
}

SPK::Ref<SPK::System> createSPKDX9Basic(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::DX9Basic");
    SPK::Ref<SPK::GL::GLQuadRenderer> renderer = makeQuadRenderer(textures.point, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::Group> g1 = system->createGroup(100);
    g1->setRadius(0.05f);
    g1->setLifeTime(1.0f, 2.0f);
    SPK::Ref<SPK::RandomEmitter> emitter = SPK::RandomEmitter::create();
    emitter->setZone(SPK::Point::create());
    emitter->setForce(0.4f, 0.6f);
    emitter->setFlow(50);
    g1->addEmitter(emitter);
    g1->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -0.5f, 0.0f)));
    g1->addModifier(SPK::Friction::create(0.2f));
    g1->addModifier(SPK::Obstacle::create(SPK::Plane::create(SPK::Vector3D(0.0f, -0.8f, 0.0f), SPK::Vector3D(0.0f, 1.0f, 0.0f))));
    g1->addModifier(SPK::Rotator::create());
    g1->setRenderer(renderer);
    return system;
}

SPK::Ref<SPK::System> createSPKDX9Gravitation(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::DX9Gravitation");
    SPK::Ref<SPK::GL::GLQuadRenderer> trail = makeQuadRenderer(textures.point, SPK::BLEND_MODE_ADD);
    SPK::Ref<SPK::Group> particleGroup = system->createGroup(4100);
    particleGroup->setLifeTime(5.0f, 8.0f);
    SPK::Ref<SPK::RandomEmitter> e = SPK::RandomEmitter::create();
    e->setFlow(400);
    e->setForce(0.0f, 0.1f);
    particleGroup->addEmitter(e);
    particleGroup->setRenderer(trail);
    particleGroup->addModifier(SPK::PointMass::create(SPK::Vector3D(1.5f, 0.0f, 0.0f), 3.0f, 0.05f));
    particleGroup->addModifier(SPK::PointMass::create(SPK::Vector3D(-1.5f, 0.0f, 0.0f), 3.0f, 0.05f));
    return system;
}

SPK::Ref<SPK::System> createSPKDX9Rain(const DemoTextureSet& textures)
{
    SPK::Ref<SPK::System> system = SPK::System::create(true);
    system->setName("Demo::DX9Rain");

    SPK::Ref<SPK::GL::GLPointRenderer> dropRenderer = SPK::GL::GLPointRenderer::create(2.0f);
    dropRenderer->setBlendMode(SPK::BLEND_MODE_ADD);
    if (textures.point)
    {
        dropRenderer->setType(SPK::POINT_TYPE_SPRITE);
        dropRenderer->setTexture(textures.point);
        dropRenderer->enableWorldSize(true);
    }

    SPK::Ref<SPK::GL::GLLineRenderer> rainRenderer = SPK::GL::GLLineRenderer::create();
    rainRenderer->setBlendMode(SPK::BLEND_MODE_ADD);
    rainRenderer->setLength(-0.1f);

    SPK::Ref<SPK::GL::GLQuadRenderer> splashRenderer = makeQuadRenderer(
        textures.waterdrops ? textures.waterdrops : textures.point,
        SPK::BLEND_MODE_ADD);

    SPK::Ref<SPK::Box> rainZone = SPK::Box::create(SPK::Vector3D(0.0f, 5.0f, 0.0f), SPK::Vector3D(10.0f, 0.0f, 10.0f));
    SPK::Ref<SPK::StraightEmitter> rainEmitter = SPK::StraightEmitter::create(SPK::Vector3D(0.0f, -1.0f, 0.0f));
    rainEmitter->setZone(rainZone);
    rainEmitter->setFlow(3000.0f);
    rainEmitter->setForce(5.0f, 10.0f);
    SPK::Ref<SPK::Group> rainGroup = system->createGroup(10000);
    rainGroup->setRenderer(rainRenderer);
    rainGroup->addEmitter(rainEmitter);
    rainGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -2.0f, 0.0f)));
    rainGroup->addModifier(SPK::Friction::create(0.7f));

    SPK::Ref<SPK::Group> dropGroup = system->createGroup(22000);
    dropGroup->setLifeTime(0.05f, 0.5f);
    dropGroup->setRenderer(dropRenderer);
    dropGroup->addModifier(SPK::Gravity::create(SPK::Vector3D(0.0f, -2.0f, 0.0f)));
    dropGroup->addModifier(SPK::Friction::create(0.7f));
    dropGroup->addEmitter(SPK::SphericEmitter::create(
        SPK::Vector3D(0.0f, 1.0f, 0.0f),
        0.0f,
        0.2f * 3.14159f,
        SPK::Point::create(SPK::Vector3D(0.0f, 0.01f, 0.0f)),
        true,
        -1,
        0.1f,
        1.0f,
        2.0f));

    SPK::Ref<SPK::Group> splashGroup = system->createGroup(3000);
    splashGroup->setLifeTime(0.2f, 0.5f);
    splashGroup->setRenderer(splashRenderer);
    splashGroup->setParamInterpolator(SPK::PARAM_SCALE, SPK::FloatRandomInterpolator::create(0.2f, 0.4f, 1.0f, 2.0f));
    splashGroup->setParamInterpolator(SPK::PARAM_ANGLE, SPK::FloatRandomInitializer::create(0.0f, 2.0f * 3.14159f));
    splashGroup->addEmitter(SPK::StaticEmitter::create(SPK::Point::create(), true, 120, 30.0f));

    return system;
}

} // namespace

const std::vector<std::string>& GetDemoSystemNames()
{
    static const std::vector<std::string> kNames = {
        "SPKTest",
        "SPKCollision",
        "SPKFlakes",
        "SPKExplosion",
        "SPKTestIrrlicht",
        "SPKTestIrrlicht_Controllers",
        "DX9.BasicDemo",
        "DX9.GravitationDemo",
        "DX9.RainDemo"
    };
    return kNames;
}

SPK::Ref<SPK::System> CreateDemoSystem(size_t index, const DemoTextureSet& textures)
{
    switch (index)
    {
    case 0: return createSPKTest(textures);
    case 1: return createSPKCollision(textures);
    case 2: return createSPKFlakes(textures);
    case 3: return createSPKExplosion(textures);
    case 4: return createSPKIrrlichtTest(textures);
    case 5: return createSPKIrrlichtTest(textures); // same base, controller behavior omitted for editor runtime
    case 6: return createSPKDX9Basic(textures);
    case 7: return createSPKDX9Gravitation(textures);
    case 8: return createSPKDX9Rain(textures);
    default: return createSPKTest(textures);
    }
}

} // namespace spark_editor
