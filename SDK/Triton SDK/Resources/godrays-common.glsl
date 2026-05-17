// godrays-common.glsl
#define GODRAYS_SHADER

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

// These are used in the vertex shader
    UNIFORM_DECL_PREFIX mat4 mvProj;
    UNIFORM_DECL_PREFIX vec3 origin;
    UNIFORM_DECL_PREFIX float spacing;
    UNIFORM_DECL_PREFIX vec3 extinction;
    UNIFORM_DECL_PREFIX vec3 viewer;
    UNIFORM_DECL_PREFIX mat3 basis;
    UNIFORM_DECL_PREFIX vec2 textureSize;
    UNIFORM_DECL_PREFIX vec3 viewDir;
    UNIFORM_DECL_PREFIX float fade;

// These are used in the pixel shader
    UNIFORM_DECL_PREFIX vec3 sunColor;
    UNIFORM_DECL_PREFIX float visibility;

// These are used in both
    UNIFORM_DECL_PREFIX vec3 sunDir;
    UNIFORM_DECL_PREFIX mat3 invBasis;

#if defined(USE_UBO)
};
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
layout(std140) uniform trit_TextureHandles_UBO {
#endif

    SAMPLER_DECL_PREFIX sampler2D displacementMap;
    SAMPLER_DECL_PREFIX sampler2D slopeFoamMap;

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
};
#endif


