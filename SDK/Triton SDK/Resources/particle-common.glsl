//particle-common.glsl
#define PARTICLE_SHADER

#ifdef USE_UBO
#define UNIFORM_DECL_PREFIX
#else
#define UNIFORM_DECL_PREFIX uniform
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
#define SAMPLER_DECL_PREFIX
#else
#define SAMPLER_DECL_PREFIX uniform
#endif

#if defined(USE_UBO)
layout(std140) uniform trit_ShaderParameters_UBO {
#endif

// These are used in vertex shader
    UNIFORM_DECL_PREFIX mat4 trit_mvProj;
    UNIFORM_DECL_PREFIX mat4 trit_modelview;
    UNIFORM_DECL_PREFIX vec3 trit_cameraPos;
    UNIFORM_DECL_PREFIX vec3 trit_refOffset;
    UNIFORM_DECL_PREFIX bool trit_hasHeightMap;
    UNIFORM_DECL_PREFIX bool trit_hasUserHeightMap;
    UNIFORM_DECL_PREFIX vec2 trit_heightMapRangeOffset;
    UNIFORM_DECL_PREFIX mat4 trit_heightMapMatrix;
    UNIFORM_DECL_PREFIX float trit_invSizeFactor;
    UNIFORM_DECL_PREFIX float trit_seaLevel;
    UNIFORM_DECL_PREFIX float trit_fogDensity;
#ifdef BINDLESS_TEXTURES
    SAMPLER_DECL_PREFIX sampler2D trit_heightMap;
#endif

// These are used in pixel shader
    UNIFORM_DECL_PREFIX vec4 trit_lightColor;
#ifndef USE_INSTANCING_FOR_TRANSPARENCY
    UNIFORM_DECL_PREFIX float trit_transparency;
#endif
    UNIFORM_DECL_PREFIX vec3 trit_fogColor;

// These are used in both
    UNIFORM_DECL_PREFIX float trit_time;
    UNIFORM_DECL_PREFIX vec3 trit_g;

#if defined(USE_UBO)
};
#endif

#ifndef BINDLESS_TEXTURES
SAMPLER_DECL_PREFIX sampler2D trit_heightMap;
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
layout(std140) uniform trit_TextureHandles_UBO {
#endif

    SAMPLER_DECL_PREFIX sampler2D trit_particleTexture;

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
};
#endif

