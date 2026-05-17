//decal-common.glsl
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

    UNIFORM_DECL_PREFIX mat4 trit_modelview;
    UNIFORM_DECL_PREFIX mat4 trit_inverseView;
    UNIFORM_DECL_PREFIX vec4 trit_viewport;
    UNIFORM_DECL_PREFIX vec2 trit_inverseViewport;
    UNIFORM_DECL_PREFIX mat4 trit_projMatrix;
    UNIFORM_DECL_PREFIX mat4 trit_inverseProjection;
    UNIFORM_DECL_PREFIX vec3 trit_up;
    UNIFORM_DECL_PREFIX vec2 trit_depthRange;
    UNIFORM_DECL_PREFIX float trit_depthOffset;
    UNIFORM_DECL_PREFIX float trit_fogDensity;
    UNIFORM_DECL_PREFIX vec3 trit_fogColor;

    // can change per decal
    UNIFORM_DECL_PREFIX mat4 trit_mvProj;
    UNIFORM_DECL_PREFIX mat4 trit_decalMatrix;
    UNIFORM_DECL_PREFIX float trit_alpha;
    UNIFORM_DECL_PREFIX vec4 trit_lightColor;
    UNIFORM_DECL_PREFIX vec2 trit_colorUVOffset;


#if defined(USE_UBO)
};
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
layout(std140) uniform trit_TextureHandles_UBO {
#endif

    SAMPLER_DECL_PREFIX sampler2D trit_depthTexture;
    SAMPLER_DECL_PREFIX sampler2D trit_decalTexture;

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
};
#endif
