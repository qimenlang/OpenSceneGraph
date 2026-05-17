#ifdef PS30
#define DETAIL
#endif
#define DETAIL_OCTAVE 2.0
#define DETAIL_BLEND 0.4
#define SPECULAR_BOOST 500.0
#define SPECULAR_DISTANCE_FACTOR 70.0
#define PROP_WASH_BOOST 1.0
#define BUBBLES 0.1

uniform float3 L;
uniform float3 lightColor;
uniform float3 ambientColor;
uniform float3 refractColor;
uniform float shininess;
uniform float4x4 modelview;
uniform float4x4 projection;
uniform float depthOffset;
uniform float4x4 invModelviewProj;
uniform float4 plane;
uniform float3x3 basis;
uniform float3x3 invBasis;
uniform float3 cameraPos;
uniform float4x4 gridScale;
uniform float gridSize;
uniform float2 textureSize;
uniform float foamScale;
uniform bool hasEnvMap;
uniform float fogDensity;
uniform float3 fogColor;
uniform float fogDensityBelow;
uniform float noiseAmplitude;
uniform float3x3 cubeMapMatrix;
uniform float invNoiseDistance;
uniform float invDampingDistance;
uniform bool doWakes;
uniform bool hasPlanarReflectionMap;
uniform float3x3 planarReflectionMapMatrix;
uniform float planarReflectionDisplacementScale;
uniform float3 floorPlanePoint;
uniform float3 floorPlaneNormal;
uniform float foamBlend;
uniform float washLength;
uniform float textureLODBias;
uniform float4x4 modelMatrix;
uniform float planarReflectionBlend;
uniform float2 zNearFar;

uniform bool hasHeightMap;
uniform float time;
uniform float seaLevel;
uniform float4x4 heightMapMatrix;

uniform bool hasDepthMap;

uniform bool depthOnly;

uniform bool underwater;

uniform int numKelvinWakes;
uniform int numPropWashes;
uniform int numCircularWaves;

uniform float3 doubleRefractionColor;
uniform float doubleRefractionIntensity;

uniform float oneOverGamma;
uniform float sunIntensity;

uniform float reflectivityScale;

uniform float transparency;

#ifdef BREAKING_WAVES
uniform float kexp;
uniform float breakerWavelength;
uniform float breakerWavelengthVariance;
uniform float4 breakerDirection;
uniform float breakerAmplitude;
uniform float breakerPhaseConstant;
uniform float surgeDepth;
uniform float steepnessVariance;
uniform float breakerDepthFalloff;
#endif

#define PI 3.14159265
#define TWOPI (2.0 * 3.14159265)

struct CircularWave {
    float amplitude;
    float radius;
    float k;
    float halfWavelength;
    float3 position;
};

struct KelvinWake {
    float amplitude;
    float3 position;
    float3 shipPosition;
    float foamAmount;
    float hullWakeWidth;
    float hullWakeLengthReciprocal;
};

#ifdef PROPELLER_WASH
struct PropWash {
    float3 deltaPos;
    float washWidth;
    float beamWidth;
    float3 propPosition;
    float distFromSource;
    float washLength;
    float alphaStart;
    float alphaEnd;
};

uniform PropWash washes[MAX_PROP_WASHES];
#endif

uniform CircularWave circularWaves[MAX_CIRCULAR_WAVES];

uniform KelvinWake wakes[MAX_KELVIN_WAKES];

#ifdef DX9
TEXTURE displacementTexture; // Kelvin wakes
TEXTURE displacementMap; // FFT waves
TEXTURE slopeFoamMap;
TEXTURE cubeMap;
TEXTURE foamTex;
TEXTURE noiseTex;
TEXTURE washTex;
TEXTURE planarReflectionMap;
TEXTURE heightMap;
TEXTURE depthMap;
TEXTURE breakerTex;
TEXTURE hullWakeTexture;

sampler2D gDisplacementSampler = sampler_state {
    Texture = (displacementMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D gDisplacementTextureSampler = sampler_state {
    Texture = (displacementTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D gHeightSampler = sampler_state {
    Texture = (heightMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler2D gDepthSampler = sampler_state {
    Texture = (depthMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

sampler2D gSlopeFoamSampler = sampler_state {
    Texture = (slopeFoamMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

samplerCUBE gCubeSampler = sampler_state {
    Texture = (cubeMap);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

sampler2D gFoamSampler = sampler_state {
    Texture = (foamTex);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D gWashSampler = sampler_state {
    Texture = (washTex);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = CLAMP;
    AddressV = WRAP;
};

sampler2D gBreakerSampler = sampler_state {
    Texture = (breakerTex);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = CLAMP;
};

sampler2D gNoiseSampler = sampler_state {
    Texture = (noiseTex);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

sampler2D gPlanarReflectionSampler = sampler_state {
    Texture = (planarReflectionMap);
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

/*sampler2D gHullWakeTextureSampler = sampler_state {
    Texture = (hullWakeTexture);
    MipFilter = LINEAR;
    MinFilter = LINEAR;
    MagFilter = LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};*/

#else

Texture2D displacementTexture;
Texture2D displacementMap;
Texture2D slopeFoamMap;
TextureCube cubeMap;
Texture2D foamTex;
Texture2D noiseTex;
Texture2D washTex;
Texture2D planarReflectionMap;
Texture2D heightMap;
Texture2D depthMap;
Texture2D breakerTex;
Texture2D hullWakeTexture;

SamplerState gBiLinearSamClamp {
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
};

SamplerState gTriLinearSamWrap {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

SamplerState gTriLinearSamWash {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = WRAP;
};

SamplerState gTriLinearSamBreaker {
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = CLAMP;
};

#endif

float getDepthFromHeightmap(in float3 worldPos)
{
    float depth = DEFAULT_DEPTH;
    float4 texCoords = mul(float4(worldPos, 1.0), heightMapMatrix);

    if (texCoords.x > 0 && texCoords.x < 1.0 && texCoords.y > 0 && texCoords.y < 1.0) {
        float4 tc4 = float4(texCoords.x, texCoords.y, 0.0, 0.0);

#ifdef DX9
        float height = tex2Dlod(gHeightSampler, tc4).x;
#else
        float height = heightMap.SampleLevel(gBiLinearSamClamp, texCoords.xy, 0).x;
#endif

        depth = -(height - seaLevel);
    }

    return depth;
}

float getDepthFromDepthmap(in float3 worldPos)
{
    float4 eyePos = mul(float4(worldPos, 1.0), modelview);
    float4 clipPos = mul(eyePos, projection);
    float3 ndcPos = clipPos.xyz / clipPos.w;

    float2 texPos = (ndcPos.xy + float2(1.0, 1.0)) * float2(0.5, 0.5);

#ifdef DX9
    float4 tc4 = float4(texPos.x, texPos.y, 0.0, 0.0);
    float terrainZ = tex2Dlod(gDepthSampler, tc4).x;
#else
    float terrainZ = depthMap.SampleLevel(gBiLinearSamClamp, texPos, 0).x;
#endif

    terrainZ = lerp(zNearFar.x, zNearFar.y, terrainZ);
    float4 terrainWorld = mul(float4(ndcPos.x, ndcPos.y, terrainZ, 1.0), invModelviewProj);

    terrainWorld /= terrainWorld.w;

    return length(terrainWorld.xyz - worldPos);
}

void computeTransparency(in float3 worldPos, inout float4 transparencyDepthBreakers)
{
    float depth = 1000.0;
    // Compute depth at this position
    if (hasHeightMap) {
        depth = getDepthFromHeightmap(worldPos);
    } else if (hasDepthMap) {
        depth = getDepthFromDepthmap(worldPos);
    } else {
        float3 up = mul(float3(0, 0, 1), invBasis);
        float3 l = -up;
        float3 l0 = worldPos;
        float3 n = floorPlaneNormal;
        float3 p0 = floorPlanePoint;
        float numerator = dot((p0 - l0), n);
        float denominator = dot(l, n);
        if (abs(denominator) > 0.0001) {
            depth = numerator / denominator;
        }
    }

    // Compute fog at this distance underwater
    float fogExponent = abs(depth) * fogDensityBelow;
    float transparency = clamp(exp(-abs(fogExponent)), 0.0, 1.0);

    transparencyDepthBreakers.xy = float2(transparency, depth);
}

float mod(float x, float y)
{
    return x - y * floor(x/y);
}

void applyBreakingWaves(inout float3 v, inout float4 transparencyDepthBreakers, out float2 breakerTexCoords)
{
    breakerTexCoords = float2(0.0, 0.0);
#ifdef BREAKING_WAVES
    float breaker = 0;
    float depth = transparencyDepthBreakers.y;

    if (hasHeightMap && breakerAmplitude > 0 && depth > 0 && depth < breakerWavelength * 0.5) {

        float halfWavelength = breakerWavelength * 0.5;
        float scaleFactor = ((depth - halfWavelength) / halfWavelength);
        float wavelength = breakerWavelength + scaleFactor * breakerWavelengthVariance;

        float halfKexp = kexp * 0.5;
        scaleFactor = (depth - halfKexp) / halfKexp;
        scaleFactor *= 1.0 + steepnessVariance;
        float k = kexp + scaleFactor;

        float3 localDir = mul(breakerDirection.xyz, basis);
        localDir.z = 0;
        localDir = normalize(localDir);
        float dotResult = dot(-localDir.xy, v.xy) * TWOPI / wavelength;

        float finalz = (dotResult + breakerPhaseConstant * time);

        float3 binormal = cross(float3(0.0, 0.0, 1.0), localDir);
        breakerTexCoords.x = dot(binormal.xy, v.xy);
        breakerTexCoords.x /= foamScale * 5.0;

#define OFFSET (PI * 0.3)
        float y = mod(finalz, TWOPI);
        float num = PI - y;
        float den = PI - OFFSET;
        breakerTexCoords.y = num / den;
        float sinz = sin(finalz);

        finalz = (sinz + 1.0) * 0.5;

        finalz = breakerAmplitude * pow(finalz, k);
        finalz *= 1.0 - min(depth * breakerDepthFalloff / halfWavelength, 1.0);

        finalz = max(0, finalz);

        breaker = clamp(sinz, 0.0, 1.0) * 2.0;

        breaker *= 1.0 - min((depth * 3.0 * breakerDepthFalloff) / halfWavelength, 1.0);

        // Hide the backs of waves if we're transparent
        float opacity = 1.0 - transparencyDepthBreakers.x;
        finalz = lerp(0.0, finalz, pow(opacity, 6.0));

        v.z += finalz;
    }
    transparencyDepthBreakers.z = breaker;
#endif
}
void applyCircularWaves(inout float3 v, in float fade, out float2 slope, out float foam)
{
    int i;

    slope = float2(0.0, 0.0);
    float disp = 0.0;

    for (i = 0; i < numCircularWaves; i++) {

        float2 D = (v - circularWaves[i].position).xy;
        float dist = length(D);

        float r = dist - circularWaves[i].radius;
        if (abs(r) < circularWaves[i].halfWavelength) {

            float amplitude = circularWaves[i].amplitude;

            float theta = circularWaves[i].k * r;
            disp += amplitude * cos(theta);
            float derivative = amplitude * -sin(theta);
            slope +=  D * (derivative / dist);
        }
    }

    v.z += disp * fade;

    foam = length(slope);
    slope *= fade;
}

void applyKelvinWakes(inout float3 v, in float fade, inout float2 slope, inout float foam)
{
    int i;
#ifdef KELVIN_WAKES
    float2 accumSlope = float2(0.0, 0.0);
    float disp = 0.0;

    float hullWakeFoam = 0;
    for (i = 0; i < numKelvinWakes; i++) {

        float3 X0 = wakes[i].position - wakes[i].shipPosition;
        float3 T = normalize(X0);
        float3 N = float3(0,0,1);
        float3 B = normalize(cross(N, T));

        float3 P = v - wakes[i].shipPosition;
        float3 X;
        X.x = dot(P.xy, T.xy);
        X.y = dot(P.xy, B.xy);

        float xLen = length(X0);
        float2 tc;
        tc.x = X.x / (1.54 * xLen);
        tc.y = (X.y) / (1.54 * xLen) + 0.5;

        if (tc.x >= 0.01 && tc.x <= 0.99 && tc.y >= 0.01 && tc.y <= 0.99) {
#ifdef DX9
            float4 tc4 = float4(tc.x, tc.y, 0.0f, 0.0f);
            float4 sample = tex2Dlod(gDisplacementTextureSampler, tc4);
#else
            float4 sample = displacementTexture.SampleLevel(gTriLinearSamWrap, tc, 0);
#endif
            float displacement = sample.w;

            displacement *= wakes[i].amplitude * fade;

            float3 thisnormal = normalize(sample.xyz * 2.0 - 1.0);
            float invmax = rsqrt( max( dot(T,T), dot(B,B) ) );
            float3x3 TBN = float3x3( T * invmax, B * invmax, N );

            thisnormal = normalize(mul(thisnormal, TBN));

            if (abs(thisnormal.x) < 0.05 && abs(thisnormal.y) < 0.05) {
                thisnormal = float3(0, 0, 1);
            }

            thisnormal.xy *= min(1.0, wakes[i].amplitude);
            thisnormal = normalize(thisnormal);

            accumSlope += float2(thisnormal.x / thisnormal.z, thisnormal.y / thisnormal.z);

            disp += displacement;

#ifdef DX9
#else
            if(wakes[i].hullWakeLengthReciprocal > 0.0) {
                tc.y = X.x * (wakes[i].hullWakeLengthReciprocal);
                tc.x = (X.y) / (wakes[i].hullWakeWidth) + 0.5;

                if (tc.x >= 0.01 && tc.x <= 0.99 && tc.y >= 0.01 && tc.y <= 0.99) {
                    float4 hullWakeSample = hullWakeTexture.SampleLevel(gTriLinearSamWrap, tc, 0);


                    if(hullWakeSample.z > 0.0) {
                        float ty = X.x * wakes[i].hullWakeLengthReciprocal;
                        float t = length(P) * wakes[i].hullWakeLengthReciprocal;

                        if(ty < 0.1) {
                            float tScale = 10.0*ty;
                            hullWakeFoam = hullWakeSample.z * wakes[i].foamAmount * tScale;
                        } else if(ty > 0.9) {
                            float tScale = 1.0-(10.0*(ty-0.9));
                            hullWakeFoam = hullWakeSample.z * wakes[i].foamAmount * tScale;
                        } else {
                            hullWakeFoam = hullWakeSample.z * wakes[i].foamAmount;
                        }
                    }
                }
            }
#endif
        }
    }

    v.z += disp * fade;

    //foam += min(1.0, length(accumSlope));
    foam = min(1.0, foam+hullWakeFoam+length(accumSlope));
    slope += accumSlope * fade;


#endif
}

void applyPropWash(in float3 v, out float3 washTexCoords)
{
    washTexCoords = float3(0.0, 0.0, 0.0);

#ifdef PROPELLER_WASH

    for (int i = 0; i < numPropWashes; i++) {

        if (washes[i].distFromSource == 0) continue;

        float3 C = washes[i].deltaPos;
        float3 A = v - washes[i].propPosition;
        float segmentLength = length(C);

        // Compute t
        float t0 = dot(C, A) / dot(C, C);

        // Compute enough overlap to account for curved paths.
        float overlap = (washes[i].washWidth / segmentLength) * 0.5;

        if (t0 >= -overlap && t0 <= 1.0 + overlap) {

            // Compute distance from source
            float distFromSource = washes[i].distFromSource - (1.0 - t0) * segmentLength;
            distFromSource = max(0.0, distFromSource);

            // Compute wash width
            float washWidth = (washes[i].washWidth * pow(distFromSource, 1.0 / 4.5)) * 0.5;

            // Compute distance to line
            float3 B = A - C;
            float3 aCrossB = cross(A, B);
            float d = length(aCrossB) / segmentLength;

            // The direction of A X B indicates if we're 'left' or 'right' of the path
            float nd = d / washWidth;

            if (nd >= 0.0 && nd <= 1.0) {
                float t = nd;

                float3 up = float3(0,0,1.0);
                if(dot(up, aCrossB) >= 0) {
                    t = (1.0-nd*0.5)-0.5;
                } else {
                    t = 0.5+(nd*0.5);
                }

                nd = t;
                washTexCoords.x = nd;
                // The t0 parameter from our initial distance test to the line segment makes
                // for a handy t texture coordinate
                //scale texture by 4 to reduce tiling
                washTexCoords.y =  (washes[i].washLength - distFromSource) / (4.0*washes[i].beamWidth);

                float blend = lerp(washes[i].alphaEnd, washes[i].alphaStart, t0);
                blend = clamp(blend, 0.0, 1.0);

                if(nd <= 0.05 || nd >= 0.95) {
                    blend = 0;
                } else if(nd <= 0.1) {
                    blend *= (nd-.05)*20.0;
                } else if (nd >= .9) {
                    blend *= (.95-nd)*20.0;
                }

                washTexCoords.z = blend;
            }
        }
    }
#endif
}

float3 applyPropWashFrag(in float3 v)
{
    float3 washTexCoords = float3(0.0, 0.0, 0.0);
    float3 Cw = float3(0.0, 0.0, 0.0);

#ifdef PROPELLER_WASH

    float numHits = 0.0;
    for (int i = 0; i < numPropWashes; i++) {

        if (washes[i].distFromSource == 0) continue;

        float3 C = washes[i].deltaPos;
        float3 A = v - washes[i].propPosition;
        float segmentLength = length(C);

        // Compute t
        float t0 = dot(C, A) / dot(C, C);

        // Compute enough overlap to account for curved paths.
        float overlap = (washes[i].washWidth / segmentLength) * 0.5;

        if (t0 >= -overlap && t0 <= 1.0 + overlap) {

            // Compute distance from source
            float distFromSource = washes[i].distFromSource - (1.0 - t0) * segmentLength;
            distFromSource = max(0.0, distFromSource);

            // Compute wash width
            float washWidth = (washes[i].washWidth * pow(distFromSource, 1.0 / 4.5)) * 0.5;

            // Compute distance to line
            float3 B = A - C;
            float3 aCrossB = cross(A, B);
            float d = length(aCrossB) / segmentLength;

            // The direction of A X B indicates if we're 'left' or 'right' of the path
            float nd = d / washWidth;

            if (nd >= 0.0 && nd <= 1.0) {

                float t = nd;

                float3 up = float3(0, 0, 1.0);
                if (dot(up, aCrossB) >= 0) {
                    t = (1.0 - nd * 0.5) - 0.5;
                } else {
                    t = 0.5 + (nd * 0.5);
                }

                nd = t;

                washTexCoords.x = nd;
                // The t0 parameter from our initial distance test to the line segment makes
                // for a handy t texture coordinate
                washTexCoords.y = (washes[i].washLength - distFromSource) / washes[i].beamWidth;

                float blend = lerp(washes[i].alphaEnd, washes[i].alphaStart, t0);
                //float distFromCenter = d / washWidth;
                blend *= max(0.0, 1.0 - nd * nd);
                //if (washes[i].number == 0) blend *= 1.0 - clamp(t0 * t0, 0.0, 1.0);
                //blend *= smoothstep(0, 0.1, nd);
                washTexCoords.z = clamp(blend, 0.0, 1.0);

#ifdef DX9
                Cw += tex2D(gWashSampler, washTexCoords.xy).xyz * ambientColor * washTexCoords.z;
#else
                Cw += washTex.Sample(gTriLinearSamWash, washTexCoords.xy).xyz * ambientColor * washTexCoords.z;
#endif
                numHits += 1.0;
            }
        }
    }
#endif

    return numHits > 0.0 ? Cw / numHits : Cw;
}


void displace(inout float3 v, inout float2 texCoords, inout float2 foamTexCoords, inout float2 noiseTexCoords,
              out float2 slope, out float foam, out float3 washTexCoords, inout float4 transparencyDepthBreakers,
              out float2 breakerTexCoords)
{
    // Transform so z is up
    float3 localV = mul(v, basis);

    texCoords = localV.xy / textureSize;
    float4 tc4 = float4(texCoords.x, texCoords.y, 0.0, 0.0);

#ifdef DX9
    float3 displacement = tex2Dlod(gDisplacementSampler, tc4).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += tex2Dlod(gDisplacementSampler, float4(texCoords * 2.0, 0.0, 0.0)).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#else
    float3 displacement = displacementMap.SampleLevel(gTriLinearSamWrap, texCoords, 0).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += displacementMap.SampleLevel(gTriLinearSamWrap, texCoords * 2.0, 0).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#endif

    float opacity = 1.0 - transparencyDepthBreakers.x;
    displacement.z = lerp(0.0, displacement.z, pow(opacity, 6.0));

    float fade = 1.0 - smoothstep(0.0, 1.0, length(v - cameraPos) * invDampingDistance);

#ifdef BREAKING_WAVES
    // Fade out waves in the surge zone
    float depthFade = 1.0;

    if (surgeDepth > 0) {
        depthFade = min(surgeDepth, transparencyDepthBreakers.y) / surgeDepth;
        depthFade = clamp(depthFade, 0.0, 1.0);
    }

    fade *= depthFade;
    transparencyDepthBreakers.w = depthFade;
#endif

    localV.xy += displacement.xy * fade;

    if (doWakes) {
        slope = float2(0,0);
        applyCircularWaves(localV, fade, slope, foam);
        applyKelvinWakes(localV, fade, slope, foam);
#ifndef PER_FRAGMENT_PROP_WASH
        applyPropWash(localV, washTexCoords);
#endif
    } else {
        washTexCoords = float3(0.0, 0.0, 0.0);
        slope = float2( 0.f, 0.f );
        foam = 0.f;
    }

#ifdef PER_FRAGMENT_PROP_WASH
    washTexCoords = localV;
#endif

    localV.z += displacement.z * fade;

    applyBreakingWaves(localV, transparencyDepthBreakers, breakerTexCoords);

    foamTexCoords = localV.xy / foamScale;
    noiseTexCoords = texCoords * 0.03;

    v = mul(localV, invBasis);
}


#ifdef DX9
void VS( float4 position : POSITION,

         out float4 oPosition : POSITION0,
         out float2 texCoords : TEXCOORD0,
         out float2 foamTexCoords : TEXCOORD1,
         out float2 noiseTexCoords : TEXCOORD2,
         out float4 V : TEXCOORD3,
         out float4  transparencyDepthBreakers : TEXCOORD4,
         out float4 wakeNormalsAndFoam : TEXCOORD5,
         out float  fogFactor : TEXCOORD6,
         out float3 washTexCoords : TEXCOORD7,
         out float2 breakerTexCoords : TEXCOORD8
       )
#else
void VS( float4 position : POSITION,

         out float4 oPosition : SV_POSITION,
         out float2 texCoords : TEXCOORD0,
         out float2 foamTexCoords : TEXCOORD1,
         out float2 noiseTexCoords : TEXCOORD2,
         out float4 V : TEXCOORD3,
         out float4  transparencyDepthBreakers : TEXCOORD4,
         out float4 wakeNormalsAndFoam : TEXCOORD5,
         out float  fogFactor : FOG,
         out float3 washTexCoords : TEXCOORD6,
         out float2 breakerTexCoords : TEXCOORD7
       )
#endif
{
    float4 worldPos = mul(position, modelMatrix);

    wakeNormalsAndFoam = float4( 0.0, 0.0, 1.0, 0.0);
    transparencyDepthBreakers = float4(0.0, 0.0, 0.0, 0.0);
    breakerTexCoords = float2(0.0, 0.0);

    // Displace
    float2 slope;
    float foam;

    computeTransparency(worldPos.xyz, transparencyDepthBreakers);

    displace(worldPos.xyz, texCoords, foamTexCoords, noiseTexCoords, slope, foam, washTexCoords, transparencyDepthBreakers, breakerTexCoords);

    float3 sx = float3(1.0, 0.0, slope.x);
    float3 sy = float3(0.0, 1.0, slope.y);
    wakeNormalsAndFoam.xyz = normalize(cross(sx, sy));
    wakeNormalsAndFoam.w = min(1.0, foam);

    float3 V3 = (worldPos.xyz - cameraPos);

    // Project it back again, apply depth offset.
    float4 v = mul(worldPos, modelview);
    v.w -= depthOffset;
    oPosition = mul(v, projection);

    V = float4(V3.x, V3.y, V3.z, oPosition.z / oPosition.w);

    float fogExponent = length(V.xyz) * fogDensity;
    fogFactor = saturate(exp(-(fogExponent * fogExponent)));
}

#ifdef DX9
float4 PS(float2 texCoords : TEXCOORD0,
          float2 foamTexCoords : TEXCOORD1,
          float2 noiseTexCoords : TEXCOORD2,
          float4 V : TEXCOORD3,
          float4 transparencyDepthBreakers : TEXCOORD4,
          float4 wakeNormalsAndFoam : TEXCOORD5,
          float  fogFactor : TEXCOORD6,
          float3 washTexCoords : TEXCOORD7,
          float2 breakerTexCoords : TEXCOORD8 ) : COLOR {
#else
float4 PS(float4 posH : SV_POSITION,
float2 texCoords : TEXCOORD0,
float2 foamTexCoords : TEXCOORD1,
float2 noiseTexCoords : TEXCOORD2,
float4 V : TEXCOORD3,
float4  transparencyDepthBreakers : TEXCOORD4,
float4 wakeNormalsAndFoam : TEXCOORD5,
float  fogFactor : FOG,
float3 washTexCoords : TEXCOORD6,
float2 breakerTexCoords : TEXCOORD7 ) : SV_TARGET {
#endif

    if (hasHeightMap && transparencyDepthBreakers.y < 0) {
        discard;
    }

#ifdef PS30
    if (depthOnly) {
        return float4(V.w, V.w, V.w, 1.0);
    }
#endif

    const float IOR = 1.33333;

    float tileFade = exp(-length(V.xyz) * invNoiseDistance);
    float horizDist = length((mul(V.xyz, basis)).xy);
#ifdef PS30
    float horizDistNorm = horizDist * invNoiseDistance;
    float tileFadeHoriz = exp(-horizDistNorm);
#endif

    float3 vNorm = normalize(V.xyz);

#ifdef DX9
    float4 tc = float4(texCoords.x, texCoords.y, 0.0, textureLODBias);
#ifdef HIGHALT
    float4 slopesAndFoamHigh = tex2Dbias(gSlopeFoamSampler, tc).xyzw;
    float4 slopesAndFoamMed = tex2Dbias(gSlopeFoamSampler, float4((tc.xy + 0.1) * 0.25, tc.z, tc.w)).xyzw;
    float4 slopesAndFoamLow = tex2Dbias(gSlopeFoamSampler, float4((tc.xy + 0.7) * 0.125, tc.z, tc.w)).xyzw;
    float altitude = abs(mul(V.xyz, basis).z);
    float invBlendDist = 10.0 * invNoiseDistance;
    float amp = 1.0 - min(1.0, altitude * invBlendDist);
    float4 slopesAndFoam = slopesAndFoamHigh * amp;
    float totalAmp = amp;
    amp = min(0.5, altitude * invBlendDist);
    slopesAndFoam += slopesAndFoamMed * amp;
    totalAmp += amp;
    amp = min(0.5, altitude * invBlendDist * 0.5);
    slopesAndFoam += slopesAndFoamLow * amp;
    totalAmp += amp;

    slopesAndFoam /= totalAmp;
#else
    float4 slopesAndFoam = tex2Dbias(gSlopeFoamSampler, tc).xyzw;
#endif

    float fresnelScale = length(slopesAndFoam.xyz);

#ifdef DETAIL
    for (int n = 1; n <= NUM_OCTAVES; n++) {
        float4 dtc = tc;
        dtc.xy = tc.xy * DETAIL_OCTAVE * n;
        slopesAndFoam.xyz += tex2Dbias(gSlopeFoamSampler, dtc).xyz * DETAIL_BLEND;
    }
#endif
    float3 unscaledNoise = (tex2D(gNoiseSampler, noiseTexCoords).xyz - float3(0.5f, 0.5f, 0.5f));
#else
#ifdef HIGHALT
    float4 slopesAndFoamHigh = slopeFoamMap.SampleBias(gTriLinearSamWrap, texCoords, textureLODBias).xyzw;
    float4 slopesAndFoamMed = slopeFoamMap.SampleBias(gTriLinearSamWrap, (texCoords + 0.1) * 0.25, textureLODBias).xyzw;
    float4 slopesAndFoamLow = slopeFoamMap.SampleBias(gTriLinearSamWrap, (texCoords + 0.7) * 0.125, textureLODBias).xyzw;
    float altitude = abs(mul(V.xyz, basis).z);
    float invBlendDist = 10.0 * invNoiseDistance;
    float amp = 1.0 - min(1.0, altitude * invBlendDist);
    float4 slopesAndFoam = slopesAndFoamHigh * amp;
    float totalAmp = amp;
    amp = min(0.5, altitude * invBlendDist);
    slopesAndFoam += slopesAndFoamMed * amp;
    totalAmp += amp;
    amp = min(0.5, altitude * invBlendDist * 0.5);
    slopesAndFoam += slopesAndFoamLow * amp;
    totalAmp += amp;

    slopesAndFoam /= totalAmp;
#else
    float4 slopesAndFoam = slopeFoamMap.SampleBias(gTriLinearSamWrap, texCoords, textureLODBias).xyzw;
#endif

    float fresnelScale = length(slopesAndFoam.xyz);

#ifdef DETAIL
    for (int n = 1; n <= NUM_OCTAVES; n++) {
        float2 dtc = texCoords * DETAIL_OCTAVE * n;
        slopesAndFoam.xyz += slopeFoamMap.SampleBias(gTriLinearSamWrap, dtc, textureLODBias).xyz * DETAIL_BLEND;
    }
#endif
    float3 unscaledNoise = (noiseTex.Sample(gTriLinearSamWrap, noiseTexCoords).xyz - float3(0.5, 0.5, 0.5));
#endif

    float3 normalNoise = unscaledNoise * noiseAmplitude;
    normalNoise.z = abs(normalNoise.z);

    slopesAndFoam.xyz = normalize(slopesAndFoam.xyz);
    float3 wakeNormal = wakeNormalsAndFoam.xyz;

#ifdef BREAKING_WAVES
    float breakerFade = transparencyDepthBreakers.w;
    float3 realNormal = lerp(float3(0.0, 0.0, 1.0), slopesAndFoam.xyz, breakerFade);
#else
    float3 realNormal = slopesAndFoam.xyz;
#endif
    realNormal = normalize(realNormal + wakeNormal);

#ifdef SPARKLE
    float bias = horizDistNorm * horizDistNorm * -64.0;
    bias = lerp(bias, -5.0, clamp(altitude * invBlendDist, 0.0, 1.0));
#ifdef DX9
    float4 stc = float4(texCoords.x, texCoords.y, 0.0, bias);
    float3 specularSlopes = tex2Dbias(gSlopeFoamSampler, tc).xyz;
#else
    float3 specularSlopes = slopeFoamMap.SampleBias(gTriLinearSamWrap, texCoords.xy, bias).xyz;
#endif
    float3 specNormal = mul(normalize(specularSlopes.xyz), invBasis);
#endif

#ifdef PS30
    float3 fadedNormal = lerp(float3(0, 0, 1), realNormal, tileFadeHoriz);
    float3 nNorm = mul(normalize(fadedNormal + (normalNoise * (1.0 - tileFade))), invBasis);
#else
    float3 nNorm = mul(normalize(realNormal + (normalNoise * (1.0 - tileFade))), invBasis);
#endif

    float3 P = reflect(vNorm, nNorm);

#ifndef FAST_FRESNEL
    // We don't need no stinkin Fresnel approximation, do it for real
    float3 S = refract(P, nNorm, 1.0 / IOR);

    float cos_theta1 = (dot(P, nNorm));
    float cos_theta2 = (dot(S, -nNorm));

    float Fp = (cos_theta1 - (IOR * cos_theta2)) /
    (cos_theta1 + (IOR * cos_theta2));
    float Fs = (cos_theta2 - (IOR * cos_theta1)) /
    (cos_theta2 + (IOR * cos_theta1));
    Fp = Fp * Fp;
    Fs = Fs * Fs;

    float reflectivity = clamp((Fs + Fp) * 0.5, 0.0, 1.0);
#else
    //float reflectivity = clamp( 0.02+0.97*pow((1.0-dot(reflection, nNorm)),5.0), 0.0, 1.0 );
    //float reflectivity = clamp(pow((1.0f-dot(P, nNorm)),5.0f), 0.0f, 1.0f );
    float r=(1.2-1.0)/(1.2+1.0);
    float reflectivity = max(0.0,min(1.0,r+(1.0-r)*pow((1.0-dot(nNorm, P)) * fresnelScale, 4.0)));
#endif

    // No reflections on foam.
    reflectivity = lerp(reflectivity, 0.0, clamp(wakeNormalsAndFoam.w, 0.0, 1.0));
    reflectivity *= reflectivityScale;

#ifdef PS30
    P = mul(P, basis);
    P.z = max(0.0, P.z);
    P = mul(normalize(P), invBasis);
#endif

#ifdef DX9
    float3 envColor = hasEnvMap ? texCUBElod(gCubeSampler, float4(mul(P, cubeMapMatrix), 0)) : ambientColor;
    float3 foamColor = tex2D(gFoamSampler, foamTexCoords).xyz;
#ifdef PS30
    foamColor += tex2D(gFoamSampler, foamTexCoords * 1.7).xyz;
    foamColor += tex2D(gFoamSampler, foamTexCoords * 0.3).xyz;
#endif
#else
    float3 envColor = hasEnvMap ? cubeMap.SampleLevel(gTriLinearSamWrap, mul(P, cubeMapMatrix), 0).xyz : ambientColor;
    float3 foamColor = foamTex.Sample(gTriLinearSamWrap, foamTexCoords).xyz;
    foamColor += foamTex.Sample(gTriLinearSamWrap, foamTexCoords * 1.7).xyz;
    foamColor += foamTex.Sample(gTriLinearSamWrap, foamTexCoords * 0.3).xyz;
#endif

#ifdef PS30
    if( hasPlanarReflectionMap ) {
        float3 up = mul( float3( 0., 0., 1. ), invBasis );
        // perturb view vector by normal xy coords multiplied by displacement scale
        // when we do it in world oriented space this perturbation is equal to:
        // ( nNorm - dot( nNorm, up ) * up ) == invBasis * vec3( ( basis * nNorm ).xy, 0 )
        float3 vNormPerturbed = vNorm + ( nNorm - dot( nNorm, up ) * up ) * planarReflectionDisplacementScale;
        float3 tc = mul( vNormPerturbed, planarReflectionMapMatrix );
#ifdef DX9
        float4 planarColor = tex2Dproj( gPlanarReflectionSampler, float4( tc.xy, 0., tc.z ) );
#else
        float4 planarColor = planarReflectionMap.Sample(gBiLinearSamClamp, tc.xy / tc.z );
#endif
        envColor = lerp( envColor.rgb, planarColor.rgb, planarColor.a * planarReflectionBlend);
    }
#endif

#ifndef HDR
    float3 Clight = min(ambientColor + lightColor * max(0, dot(L, nNorm)), float3(1.0, 1.0, 1.0));
#else
    float3 Clight = ambientColor + lightColor * max(0, dot(L, nNorm));
#endif

    float3 Cskylight = lerp(refractColor * Clight, envColor, reflectivity);
    float3 Cfoam = foamColor *ambientColor;

    float3 refractedL = L;

#ifdef PS30
    if (underwater) {
        refractedL = refract(normalize(L), invBasis[2], 1.0 / 1.333);
        nNorm = -nNorm;
    }
#endif

#ifdef SPARKLE
    float3 R = reflect(refractedL, specNormal);
#else
    float3 R = reflect(refractedL, nNorm);
#endif

    float spec = max(0.0f, dot(vNorm, R));
    float3 Csunlight = lightColor * min(1.0, pow(spec, shininess + horizDist * SPECULAR_DISTANCE_FACTOR) * sunIntensity * SPECULAR_BOOST * reflectivity);

    float3 Ci = Cskylight + Csunlight;

    // Fade out foam with distance to hide tiling
#ifdef BREAKING_WAVES
    float foamAmount = (clamp(slopesAndFoam.w, 0.0, 1.0) * breakerFade + wakeNormalsAndFoam.w) * tileFadeHoriz * foamBlend * breakerFade;
    foamAmount = foamAmount * foamAmount;
    Ci = Ci + (Cfoam * foamAmount);
#ifdef DX9
    float3 breakerCol = tex2D(gBreakerSampler, breakerTexCoords).xyz;
#else
    float3 breakerCol = breakerTex.Sample(gTriLinearSamBreaker, breakerTexCoords).xyz;
#endif

    float breakerNoise = max(0.0, (1.0 - abs(unscaledNoise.x * 10.0)));
    Ci += breakerCol * transparencyDepthBreakers.z * tileFadeHoriz * breakerNoise;
#else
    float foamAmount = (clamp(slopesAndFoam.w, 0.0, 1.0) + wakeNormalsAndFoam.w) * tileFadeHoriz * foamBlend;
    foamAmount = foamAmount * foamAmount;
    Ci = Ci + (Cfoam * foamAmount);
#endif

#ifdef PROPELLER_WASH
#ifdef PER_FRAGMENT_PROP_WASH
    float3 Cw = applyPropWashFrag(washTexCoords);
#else // PER_FRAGMENT_PROP_WASH
#ifdef DX9
#ifdef PS30
    float3 Cw = tex2D(gWashSampler, washTexCoords.xy).xyz * ambientColor * washTexCoords.z;
#endif // PS30
#else // DX9
    float3 Cw = washTex.Sample(gTriLinearSamWash, washTexCoords.xy).xyz * ambientColor * washTexCoords.z;
#endif // DX9
#endif // PER_FRAGMENT_PROP_WASH
    Ci = Ci + Cw;
#endif // PROPELLER_WASH

#ifdef PS30
    float doubleRefraction = max(0, dot(-vNorm, nNorm)) * (1.0 - dot(-vNorm, invBasis[2]));
    doubleRefraction += slopesAndFoam.w * BUBBLES;
    doubleRefraction *= tileFade;

    Ci += doubleRefractionColor * Clight * doubleRefraction * doubleRefractionIntensity;
#endif

#ifdef PS30
    float transparencyLocal = clamp(transparencyDepthBreakers.x, 0.0, 1.0);
    float alpha = hasHeightMap ? 1.0 - transparencyLocal : lerp(1.0 - transparencyLocal, 1.0, reflectivity);
    float4 waterColor = float4(Ci, alpha);
    float4 fogColor4 = float4(fogColor, hasHeightMap ? alpha : 1.0);

    float4 finalColor = lerp(fogColor4, waterColor, clamp(fogFactor, 0.0, 1.0));

    finalColor.xyz = pow(finalColor.xyz, float3(oneOverGamma, oneOverGamma, oneOverGamma));
#else
    float4 finalColor = float4(Ci, 1.0);
#endif

    finalColor.w *= transparency;

#ifndef HDR
    finalColor = clamp(finalColor, 0.0, 1.0);
#endif

    return finalColor;
}

#ifdef DX11
technique11 ColorTech {
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS() ) );
    }
}
#endif

#ifdef DX10
technique10 ColorTech {
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
#endif

#ifdef DX10LEVEL9
technique10 ColorTech {
    pass P0
    {
        SetVertexShader( CompileShader( vs_4_0_level_9_1, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0_level_9_1, PS() ) );
    }
}
#endif

#ifdef DX9
technique {
    pass P0
    {
        SetVertexShader( CompileShader( vs_3_0, VS() ) );
#ifdef PS30
        SetPixelShader( CompileShader( ps_3_0, PS() ) );
#else
        SetPixelShader( CompileShader( ps_2_0, PS() ) );
#endif
    }
}
#endif

