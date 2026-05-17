//projgrid-ellipsoid-common.glsl
#define OCEAN_SHADER

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

#ifdef USE_UBO
layout(std140) uniform trit_ShaderParameters_UBO {
#endif
// These are used in vertex shader
    UNIFORM_DECL_PREFIX mat4 trit_modelMatrix;
    UNIFORM_DECL_PREFIX mat4 trit_modelview;
    UNIFORM_DECL_PREFIX mat4 trit_projection;
    UNIFORM_DECL_PREFIX mat4 trit_invModelviewProj;
    UNIFORM_DECL_PREFIX vec2 trit_zNearFar;
    UNIFORM_DECL_PREFIX float trit_depthOffset;
    UNIFORM_DECL_PREFIX vec3 trit_radii;
    UNIFORM_DECL_PREFIX vec3 trit_oneOverRadii;
    UNIFORM_DECL_PREFIX vec3 trit_north;
    UNIFORM_DECL_PREFIX vec3 trit_east;
#ifdef DOUBLE_PRECISION
    UNIFORM_DECL_PREFIX dvec3 trit_cameraPos;
#else
    UNIFORM_DECL_PREFIX vec3 trit_cameraPos;
#endif
    UNIFORM_DECL_PREFIX mat4 trit_gridScale;
    UNIFORM_DECL_PREFIX float trit_foamScale;
    UNIFORM_DECL_PREFIX vec2 trit_textureSize;
    UNIFORM_DECL_PREFIX float trit_fogDensity;
    UNIFORM_DECL_PREFIX float trit_fogDensityBelow;
    UNIFORM_DECL_PREFIX float trit_invDampingDistance;
    UNIFORM_DECL_PREFIX vec3 trit_floorPlanePoint;
    UNIFORM_DECL_PREFIX vec3 trit_floorPlaneNormal;
    UNIFORM_DECL_PREFIX vec4 trit_plane;
    UNIFORM_DECL_PREFIX float trit_planarHeight;
    UNIFORM_DECL_PREFIX float trit_planarAdjust;
    UNIFORM_DECL_PREFIX vec3 trit_referenceLocation;
    UNIFORM_DECL_PREFIX float trit_time;
    UNIFORM_DECL_PREFIX float trit_seaLevel;

    UNIFORM_DECL_PREFIX vec2 trit_heightMapRangeOffset;
    UNIFORM_DECL_PREFIX mat4 trit_heightMapMatrix;

    UNIFORM_DECL_PREFIX bool trit_hasDepthMap;
    UNIFORM_DECL_PREFIX bool trit_bypassOverridePosition;

#ifdef BREAKING_WAVES
    UNIFORM_DECL_PREFIX float trit_kexp;
    UNIFORM_DECL_PREFIX float trit_breakerWavelength;
    UNIFORM_DECL_PREFIX float trit_breakerWavelengthVariance;
    UNIFORM_DECL_PREFIX vec3 trit_breakerDirection;
    UNIFORM_DECL_PREFIX float trit_breakerAmplitude;
    UNIFORM_DECL_PREFIX float trit_breakerPhaseConstant;
    UNIFORM_DECL_PREFIX float trit_surgeDepth;
    UNIFORM_DECL_PREFIX float trit_steepnessVariance;
    UNIFORM_DECL_PREFIX float trit_breakerDepthFalloff;
#ifdef BREAKING_WAVES_MAP
    UNIFORM_DECL_PREFIX mat4 trit_breakingWaveMapMatrix;
    UNIFORM_DECL_PREFIX bool trit_hasBreakingWaveMap;
#endif
#endif

// These are used in the pixel shader
    UNIFORM_DECL_PREFIX vec3 trit_L;
    UNIFORM_DECL_PREFIX vec3 trit_lightColor;
    UNIFORM_DECL_PREFIX vec3 trit_ambientColor;
    UNIFORM_DECL_PREFIX vec3 trit_refractColor;
    UNIFORM_DECL_PREFIX float trit_shininess;
    UNIFORM_DECL_PREFIX bool trit_hasEnvMap;
    UNIFORM_DECL_PREFIX vec3 trit_northPole;
#ifdef BINDLESS_TEXTURES
    SAMPLER_DECL_PREFIX samplerCube trit_cubeMap;
#endif
    UNIFORM_DECL_PREFIX float trit_noiseAmplitude;
    UNIFORM_DECL_PREFIX mat3 trit_cubeMapMatrix;
    UNIFORM_DECL_PREFIX float trit_invNoiseDistance;
#ifdef BINDLESS_TEXTURES
    SAMPLER_DECL_PREFIX sampler2D trit_planarReflectionMap;
#endif
    UNIFORM_DECL_PREFIX bool trit_hasPlanarReflectionMap;
    UNIFORM_DECL_PREFIX mat3 trit_planarReflectionMapMatrix;
    UNIFORM_DECL_PREFIX float trit_planarReflectionDisplacementScale;
    UNIFORM_DECL_PREFIX float trit_foamBlend;
    UNIFORM_DECL_PREFIX vec3 trit_fogColor;
    UNIFORM_DECL_PREFIX float trit_textureLODBias;
    UNIFORM_DECL_PREFIX float trit_planarReflectionBlend;
    UNIFORM_DECL_PREFIX bool trit_depthOnly;
#ifdef LEEWARD_DAMPENING
    UNIFORM_DECL_PREFIX vec3 trit_windDir;
#endif
    UNIFORM_DECL_PREFIX bool trit_underwater;
    UNIFORM_DECL_PREFIX vec3 trit_doubleRefractionColor;
    UNIFORM_DECL_PREFIX float trit_doubleRefractionIntensity;
    UNIFORM_DECL_PREFIX float trit_oneOverGamma;
    UNIFORM_DECL_PREFIX float trit_sunIntensity;
    UNIFORM_DECL_PREFIX float trit_reflectivityScale;
    UNIFORM_DECL_PREFIX float trit_transparency;
#ifdef HIGHALT
    UNIFORM_DECL_PREFIX float trit_invZoom;
#endif

// These are used in both
    UNIFORM_DECL_PREFIX mat3 trit_basis;
    UNIFORM_DECL_PREFIX bool trit_hasHeightMap;
    UNIFORM_DECL_PREFIX bool trit_hasUserHeightMap;
#ifdef USE_UBO
};
#endif

#ifndef BINDLESS_TEXTURES
SAMPLER_DECL_PREFIX samplerCube trit_cubeMap;
SAMPLER_DECL_PREFIX sampler2D trit_planarReflectionMap;
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
layout(std140) uniform trit_TextureHandles_UBO {
#endif

    SAMPLER_DECL_PREFIX sampler2D trit_heightMap;
    SAMPLER_DECL_PREFIX sampler2D trit_depthMap;
    SAMPLER_DECL_PREFIX sampler2D trit_displacementMap;
    SAMPLER_DECL_PREFIX sampler2D trit_slopeFoamMap;
    SAMPLER_DECL_PREFIX sampler2D trit_displacementTexture;
    SAMPLER_DECL_PREFIX sampler2D trit_hullWakeTexture;
#ifdef BREAKING_WAVES
#ifdef BREAKING_WAVES_MAP
    SAMPLER_DECL_PREFIX sampler2D trit_breakingWaveMap;
#endif
#endif

    SAMPLER_DECL_PREFIX sampler2D trit_lightFoamTex;
    SAMPLER_DECL_PREFIX sampler2D trit_noiseTex;
    SAMPLER_DECL_PREFIX sampler2D trit_washTex;
#ifdef BREAKING_WAVES
    SAMPLER_DECL_PREFIX sampler2D trit_breakerTex;
#endif

#if defined(USE_UBO) && defined(BINDLESS_TEXTURES)
};
#endif

#define PI 3.14159265
#define TWOPI (2.0 * 3.14159265)

struct trit_CircularWave {
    float amplitude;
    float radius;
    float k;
    float halfWavelength;
    vec3 position;
};

struct trit_KelvinWake {
    float amplitude;
    vec3 position;
    vec3 shipPosition;
    float foamAmount;
    float hullWakeWidth;
    float hullWakeLengthReciprocal;
};

struct trit_PropWash {
    vec3 deltaPos;
    float washWidth;
    vec3 propPosition;
    float distFromSource;
    float washLength;
    float alphaStart;
    float alphaEnd;
};

struct trit_LeewardDampener {
    vec3 bowPos;
    vec3 sternPos;
    float velocityDampening;
};

#ifdef USE_UBO
layout(std140) uniform trit_WakeParameters {
#ifdef PROPELLER_WASH
    trit_PropWash trit_washes[MAX_PROP_WASHES];
#endif

    trit_CircularWave trit_circularWaves[MAX_CIRCULAR_WAVES];

    trit_KelvinWake trit_wakes[MAX_KELVIN_WAKES];

#ifdef LEEWARD_DAMPENING
    trit_LeewardDampener trit_leewardDampeners[MAX_LEEWARD_DAMPENERS];
    int trit_numLeewardDampeners;
    float trit_leewardDampeningStrength;
#endif

    int trit_numPropWashes;
    int trit_numKelvinWakes;
    int trit_numCircularWaves;

    bool trit_doWakes;
    float trit_washLength;
};
#else
#ifdef PROPELLER_WASH
uniform trit_PropWash trit_washes[MAX_PROP_WASHES];
#endif

uniform trit_CircularWave trit_circularWaves[MAX_CIRCULAR_WAVES];

uniform trit_KelvinWake trit_wakes[MAX_KELVIN_WAKES];

#ifdef LEEWARD_DAMPENING
uniform trit_LeewardDampener trit_leewardDampeners[MAX_LEEWARD_DAMPENERS];
uniform int trit_numLeewardDampeners;
uniform float trit_leewardDampeningStrength;
#endif

uniform int trit_numPropWashes;
uniform int trit_numKelvinWakes;
uniform int trit_numCircularWaves;
uniform bool trit_doWakes;
uniform float trit_washLength;
#endif

