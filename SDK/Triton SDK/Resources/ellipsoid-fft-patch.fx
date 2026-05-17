#ifdef PS30
#define DETAIL
#endif
#define DETAIL_OCTAVE 2.0
#define DETAIL_BLEND 0.4
#define SPECULAR_BOOST 500.0
#define SPECULAR_DISTANCE_FACTOR 70.0
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
uniform float3 north;
uniform float3 east;
uniform float3 northPole;
uniform float3 radii;
uniform float3 oneOverRadii;
uniform bool hasEnvMap;
uniform float fogDensity;
uniform float fogDensityBelow;
uniform float3 fogColor;
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
uniform float planarHeight;
uniform float textureLODBias;
uniform float4x4 modelMatrix;
uniform float planarAdjust;
uniform float3 referenceLocation;
uniform float planarReflectionBlend;
uniform float2 zNearFar;

uniform bool hasHeightMap;
uniform float time;
uniform float seaLevel;
uniform float4x4 heightMapMatrix;

uniform bool hasDepthMap;

uniform int numKelvinWakes;
uniform int numPropWashes;
uniform int numCircularWaves;

uniform bool depthOnly;
uniform bool underwater;

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

#ifdef DX9
struct KelvinWake {
    float amplitude;
    float3 position;
    float3 shipPosition;
};
#else
struct KelvinWake {
    float amplitude;
    float3 position;
    float3 shipPosition;
    float foamAmount;
    float hullWakeWidth;
    float hullWakeLengthReciprocal;
};
#endif

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
TEXTURE displacementMap;
TEXTURE displacementTexture;
TEXTURE slopeFoamMap;
TEXTURE cubeMap;
TEXTURE foamTex;
TEXTURE noiseTex;
TEXTURE washTex;
TEXTURE planarReflectionMap;
TEXTURE heightMap;
TEXTURE depthMap;
TEXTURE breakerTex;

sampler2D gDisplacementSampler = sampler_state {
    Texture = (displacementMap);
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

#else

Texture2D displacementMap;
Texture2D displacementTexture;
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

SamplerState gBiLinearSamClamp {
    Filter = MIN_MAG_LINEAR_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
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
        float height = heightMap.SampleLevel(gBiLinearSamClamp, texCoords, 0).x;
#endif

        depth = -(height - seaLevel);
    }

    return depth;
}

float getDepthFromDepthmap(in float3 localPos)
{
    float4 eyePos = mul(float4(localPos, 1.0), modelview);
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

    return length(terrainWorld.xyz) - length(localPos);
}

void computeTransparency(in float3 worldPos, in float3 localPos, in float3 up, inout float4 transparencyDepthBreakers)
{
    float depth = 1000.0;
    // Compute depth at this position
    if (hasHeightMap) {
        depth = getDepthFromHeightmap(worldPos);
    } else if (hasDepthMap) {
        depth = getDepthFromDepthmap(localPos);
    } else {
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

void applyCircularWaves(inout float3 v, in float3 localPos, float fade, out float2 slope, out float foam)
{
    int i;

    float3 slope3 = float3(0.0, 0.0, 0.0);
    float disp = 0.0;

    for (i = 0; i < numCircularWaves; i++) {

        float3 D = (localPos - circularWaves[i].position);
        float dist = length(D);

        float r = dist - circularWaves[i].radius;
        if (abs(r) < circularWaves[i].halfWavelength) {

            float amplitude = circularWaves[i].amplitude;

            float theta = circularWaves[i].k * r;
            disp += amplitude * cos(theta);
            float derivative = amplitude * -sin(theta);
            slope3 +=  D * (derivative / dist);
        }
    }

    v.z += disp * fade;

    slope = mul(slope3, basis).xy;

    foam = length(slope.xy);
}

void applyKelvinWakes(inout float3 v, in float3 localPos, float fade, in float3 up, inout float2 slope, inout float foam)
{
    int i;
#ifdef KELVIN_WAKES
    float2 accumSlope = float2(0,0);
    float disp = 0.0;

    float hullWakeFoam = 0;
    for (i = 0; i < numKelvinWakes; i++) {

        float3 X0 = wakes[i].position - wakes[i].shipPosition;
        float3 T = normalize(X0);
        float3 N = up;
        float3 B = normalize(cross(N, T));

        float3 P = localPos - wakes[i].shipPosition;
        float3 X;
        X.x = dot(P, T);
        X.y = dot(P, B);
//      X.z = dot(P, N);

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

            // Convert to z-up
            thisnormal = mul(thisnormal, basis);

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
#ifdef DX9
                    float4 tc4 = float4(tc.x, tc.y, 0.0f, 0.0f);
                    float4 hullWakeSample = tex2Dlod(hullWakeTexture, tc4);
#else
                    float4 hullWakeSample = hullWakeTexture.SampleLevel(gTriLinearSamWrap, tc, 0);
#endif

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


void applyPropWash(in float3 v, in float3 localPos, in float3 up, out float3 washTexCoords)
{
    washTexCoords = float3(0.0, 0.0, 0.0);

#ifdef PROPELLER_WASH

    for (int i = 0; i < numPropWashes; i++) {

        if (washes[i].distFromSource == 0) continue;

        float3 C = washes[i].deltaPos;
        float3 A = localPos - washes[i].propPosition;
        float3 B = localPos - (washes[i].propPosition + C);
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
            float3 aCrossB = cross(A, B);
            float d = length(aCrossB) / length(C);

            // The direction of A X B indicates if we're 'left' or 'right' of the path
            float nd = d / washWidth;

            if (nd >= 0.0 && nd <= 1.0) {
                float t = nd;

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

float3 applyPropWashFrag(in float3 localPos, in float3 up)
{
    float3 washTexCoords = float3(0.0, 0.0, 0.0);
    float3 Cw = float3(0.0, 0.0, 0.0);

#ifdef PROPELLER_WASH

    float numHits = 0.0;
    for (int i = 0; i < numPropWashes; i++) {

        if (washes[i].distFromSource == 0) continue;

        float3 C = washes[i].deltaPos;
        float3 A = localPos - washes[i].propPosition;
        float3 B = localPos - (washes[i].propPosition + C);
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
            float3 aCrossB = cross(A, B);
            float d = length(aCrossB) / length(C);

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

                float distFromCenter = d / washWidth;
                blend *= max(0.0, 1.0 - distFromCenter * distFromCenter);
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

// Intersect a ray of origin P0 and direction v against a unit sphere centered at the origin
bool raySphereIntersect(in float3 p0, in float3 v, out float3 intersection)
{
    float twop0v = 2.0 * dot(p0, v);
    float p02 = dot(p0, p0);
    float v2 = dot(v, v);

    float disc = twop0v * twop0v - (4.0 * v2)*(p02 - 1);
    if (disc > 0) {
        float discSqrt = sqrt(disc);
        float den = 2.0 * v2;
        float t = (-twop0v - discSqrt) / den;
        if (t < 0) {
            t = (-twop0v + discSqrt) / den;
        }
        intersection = p0 + t * v;
        return true;
    } else {
        intersection = float3(0.0, 0.0, 0.0);
        return false;
    }
}

// Intersect a ray against an ellipsoid centered at the origin
bool rayEllipsoidIntersect(in float3 R0, in float3 Rd, out float3 intersection)
{
    // Distort the ray so it aims toward a unit sphere, do a sphere intersection
    // and scale it back to the ellpsoid's space.

    float3 scaledR0 = R0 * oneOverRadii;
    float3 scaledRd = Rd * oneOverRadii;

    float3 sphereIntersection;
    if (raySphereIntersect(scaledR0, scaledRd, sphereIntersection)) {
        intersection = sphereIntersection * radii;
        return true;
    } else {
        intersection = float3(0.0, 0.0, 0.0);
        return false;
    }
}

// Alternate, faster method - but it can't handle viewpoints inside the ellipsoid.
// If you don't need underwater views, this may be better for you.
bool rayEllipsoidIntersectFast(in float3 R0, in float3 Rd, out float3 intersection)
{
    float3 q = R0 * oneOverRadii;
    float3 bUnit = normalize(Rd * oneOverRadii);
    float wMagnitudeSquared = dot(q, q) - 1.0;

    float t = -dot(bUnit, q);
    float tSquared = t * t;

    if ((t >= 0.0) && (tSquared >= wMagnitudeSquared)) {
        float temp = t - sqrt(tSquared - wMagnitudeSquared);
        float3 r = (q + temp * bUnit);
        intersection = r * radii;

        return true;
    } else {
        intersection = float3(0.0, 0.0, 0.0);
        return false;
    }
}

bool rayPlaneIntersect(in float4 p0, in float4 p1, out float4 intersection)
{
    // Intersect with the sea level
    float4 dp = p1 - p0;
    float t = -dot(p0, plane) / dot( dp, plane);
    if (t > 0.0 && t < 1.0) {
        intersection = dp * t + p0;
        intersection /= intersection.w;
        return true;
    } else {
        intersection = float4(0.0, 0.0, 0.0, 0.0);
        return false;
    }
}

float3 computeArcLengths(in float3 localPos, in float3 northDir, in float3 eastDir)
{
    float3 pt = referenceLocation + localPos;
    return float3(dot(pt, eastDir), dot(pt, northDir), 0);
}

#ifdef DX9
void VS( float4 position : POSITION,

         out float4 oPosition : POSITION0,
         out float2 texCoords : TEXCOORD0,
         out float2 foamTexCoords : TEXCOORD1,
         out float2 noiseTexCoords : TEXCOORD2,
         out float4 V : TEXCOORD3,
         out float3 up : TEXCOORD4,
         out float4  transparencyDepthBreakers : TEXCOORD5,
         out float4 wakeNormalAndFoam : TEXCOORD6,
         out float4 fogFactor : COLOR0,
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
         out float3 up : TEXCOORD4,
         out float4  transparencyDepthBreakers : TEXCOORD5,
         out float4 wakeNormalAndFoam : TEXCOORD6,
         out float fogFactor : FOG,
         out float3 washTexCoords : TEXCOORD7,
         out float2 breakerTexCoords : TEXCOORD8
       )
#endif
{
    // To avoid precision issues, the translation component of the modelview matrix
    // is zeroed out, and the camera position passed in via cameraPos

    float4 localPos = mul(position, modelMatrix);
    float4 worldPos = localPos + float4(cameraPos.x, cameraPos.y, cameraPos.z, 0.0);

    bool above = true;

    wakeNormalAndFoam = float4( 0.0, 0.0, 1.0, 0.0 );
    transparencyDepthBreakers = float4(0.0, 0.0, 0.0, 0.0);
    breakerTexCoords = float2(0.0, 0.0);

    up = normalize(worldPos.xyz);

    computeTransparency(worldPos.xyz, localPos.xyz, up, transparencyDepthBreakers);

    // Transform position on the ellipsoid into a planar reference,
    // x east, y north, z up
    float3 planar = computeArcLengths(localPos.xyz, north, east);

    float fade = 1.0 - smoothstep(0.0, 1.0, length(localPos.xyz) * invDampingDistance);
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

    // Compute displacement
    texCoords = planar.xy / textureSize;
    float4 tc4 = float4(texCoords.x, texCoords.y, 0, 0);
#ifdef DX9
    float3 displacement = tex2Dlod(gDisplacementSampler, tc4).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += tex2Dlod(gDisplacementSampler, float4(texCoords * 2.0, 0.0, 0.0)).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#else
    float3 displacement = displacementMap.SampleLevel(gTriLinearSamWrap, tc4.xy, 0).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += displacementMap.SampleLevel(gTriLinearSamWrap, texCoords * 2.0, 0).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#endif
    // Hide the backs of waves if we're transparent
    float opacity = 1.0 - transparencyDepthBreakers.x;
    displacement.z = lerp(0.0, displacement.z, pow(opacity, 6.0));

    displacement *= fade;

    foamTexCoords = (planar.xy + displacement.xy) / foamScale;
    noiseTexCoords = texCoords * 0.03f;

    localPos.xyz += displacement.x * east + displacement.y * north;

    if (doWakes) {
        float3 wakeNormal;
        float wakeFoam;
        float2 slope;
        applyCircularWaves(planar, localPos.xyz, fade, slope, wakeFoam);
        applyKelvinWakes(planar, localPos.xyz, fade, up, slope, wakeFoam);

#ifndef PER_FRAGMENT_PROP_WASH
        applyPropWash(planar, localPos.xyz, up, washTexCoords);
#endif

        float3 sx = float3(1.0, 0.0, slope.x * fade);
        float3 sy = float3(0.0, 1.0, slope.y * fade);
        wakeNormal = normalize(cross(sx, sy));

        wakeNormalAndFoam.xyz = wakeNormal;
        wakeNormalAndFoam.w = min(1.0, wakeFoam);
    } else {
        washTexCoords = float3(0.0, 0.0, 0.0);
    }

#ifdef PER_FRAGMENT_PROP_WASH
    washTexCoords = localPos.xyz;
#endif

    localPos.xyz += displacement.z * up;
    applyBreakingWaves(planar, transparencyDepthBreakers, breakerTexCoords);

    // Transform back into geocentric coords

    // Make relative to the camera, add in displacement
    localPos.xyz += planar.z * up;

    // Project it back again, apply depth offset.
    float4 v = mul(localPos, modelview);
    v.w -= depthOffset;
    oPosition = mul(v, projection);

    V.xyz = localPos.xyz;
    V.w = oPosition.z / oPosition.w;


    float fogExponent = length(V.xyz) * fogDensity;
    fogFactor = saturate(exp(-(fogExponent * fogExponent)));
}

#ifdef DX9
float4 PS(float posH : POSITION0,
          float2 texCoords : TEXCOORD0,
          float2 foamTexCoords : TEXCOORD1,
          float2 noiseTexCoords : TEXCOORD2,
          float4 V : TEXCOORD3,
          float3 up : TEXCOORD4,
          float4 transparencyDepthBreakers : TEXCOORD5,
          float4 wakeNormalAndFoam : TEXCOORD6,
          float fogFactor : COLOR0,
          float3 washTexCoords : TEXCOORD7,
          float2 breakerTexCoords : TEXCOORD8 ) : COLOR {
#else
float4 PS(float4 posH : SV_POSITION,
float2 texCoords : TEXCOORD0,
float2 foamTexCoords : TEXCOORD1,
float2 noiseTexCoords : TEXCOORD2,
float4 V : TEXCOORD3,
float3 up : TEXCOORD4,
float4  transparencyDepthBreakers : TEXCOORD5,
float4 wakeNormalAndFoam : TEXCOORD6,
float fogFactor : FOG,
float3 washTexCoords : TEXCOORD7,
float2 breakerTexCoords : TEXCOORD8 ) : SV_TARGET {
#endif

#ifdef PS30
    if (hasHeightMap && transparencyDepthBreakers.y < 0) {
        discard;
    }

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

    float3 localEast = normalize(cross(northPole, up));
    float3 localNorth = cross(up, localEast);

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
    float3 unscaledNoise = (tex2D(gNoiseSampler, noiseTexCoords).xyz - float3(0.5, 0.5, 0.5));
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

#ifdef BREAKING_WAVES
    float breakerFade = transparencyDepthBreakers.w;
    float3 realNormal = lerp(float3(0.0, 0.0, 1.0), slopesAndFoam.xyz, breakerFade);
#else
    float3 realNormal = slopesAndFoam.xyz;
#endif

#ifdef SPARKLE
    float bias = horizDistNorm * horizDistNorm * -64.0;
    bias = lerp(bias, -5.0, clamp(altitude * invBlendDist, 0.0, 1.0));
#ifdef DX9
    float4 stc = float4(texCoords.x, texCoords.y, 0.0, bias);
    float3 specularSlopes = tex2Dbias(gSlopeFoamSampler, tc).xyz;
#else
    float3 specularSlopes = slopeFoamMap.SampleBias(gTriLinearSamWrap, texCoords, bias).xyz;
#endif
    float3 specNormal = normalize(specularSlopes);
    specNormal = normalize(specNormal.x * localEast + specNormal.y * localNorth + specNormal.z * up);
#endif

#ifdef PS30
    float3 fadedNormal = lerp(float3(0.0, 0.0, 1.0), realNormal, tileFadeHoriz);
    float3 N = fadedNormal + normalNoise + (wakeNormalAndFoam.xyz - float3(0.0, 0.0, 1.0));
#else
    float3 N = realNormal + normalNoise + (wakeNormalAndFoam.xyz - float3(0.0, 0.0, 1.0));
#endif

    float3 nNorm = normalize(N.x * localEast + N.y * localNorth + N.z * up);

    float3 reflection = reflect(vNorm, nNorm);

#ifndef FAST_FRESNEL
    // We don't need no stinkin Fresnel approximation, do it for real

    float3 refraction = refract(vNorm, nNorm, 1.0 / IOR);

    float cos_theta1 = (dot(vNorm, nNorm));
    float cos_theta2 = (dot(refraction, nNorm));

    float Fp = (cos_theta1 - (IOR * cos_theta2)) /
    (cos_theta1 + (IOR * cos_theta2));
    float Fs = (cos_theta2 - (IOR * cos_theta1)) /
    (cos_theta2 + (IOR * cos_theta1));
    Fp = Fp * Fp;
    Fs = Fs * Fs;

    float reflectivity = clamp((Fs + Fp) * 0.5f, 0.0f, 1.0f);
#else
    //float reflectivity = clamp(pow((1.0f-dot(reflection, nNorm)),5.0f), 0.0f, 1.0f );
    //float reflectivity = clamp( 0.02+0.97*pow((1.0-dot(reflection, nNorm)),5.0), 0.0, 1.0 );
    float r=(1.2-1.0)/(1.2+1.0);
    float reflectivity = max(0.0,min(1.0,r+(1.0-r)*pow((1.0-dot(nNorm,reflection)) * fresnelScale, 4.0)));
#endif

    // No reflections on foam.
    reflectivity = lerp(reflectivity, 0.0, clamp(wakeNormalAndFoam.w, 0.0, 1.0));

    reflectivity *= reflectivityScale;

#ifdef DX9
    float3 envColor = hasEnvMap ? texCUBE(gCubeSampler, mul(reflection, cubeMapMatrix)).xyz : ambientColor;
#ifdef PS30
    float3 foamColor = tex2D(gFoamSampler, foamTexCoords).xyz;
    foamColor += tex2D(gFoamSampler, foamTexCoords * 1.7).xyz;
    foamColor += tex2D(gFoamSampler, foamTexCoords * 0.3).xyz;
#endif
#else
    float3 envColor = hasEnvMap ? cubeMap.Sample(gTriLinearSamWrap, mul(reflection, cubeMapMatrix)).xyz : ambientColor;
    float3 foamColor = foamTex.Sample(gTriLinearSamWrap, foamTexCoords).xyz;
    foamColor += foamTex.Sample(gTriLinearSamWrap, foamTexCoords * 1.7).xyz;
    foamColor += foamTex.Sample(gTriLinearSamWrap, foamTexCoords * 0.3).xyz;
#endif

#ifdef PS30
    if( hasPlanarReflectionMap ) {
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

    float3 refractedL = L;

#ifdef PS30
    if (underwater) {
        refractedL = refract(normalize(L), up, 1.0 / 1.333);
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

#ifdef PROPELLER_WASH
#ifdef PER_FRAGMENT_PROP_WASH
    float3 Cw = applyPropWashFrag(washTexCoords, up);
#else // PER_FRAGMENT_PROP_WASH
#ifdef DX9
#ifdef PS30
    float3 Cw = tex2D(gWashSampler, washTexCoords.xy).xyz * ambientColor * washTexCoords.z;
#endif // PS30
#else // DX9
    float3 Cw = washTex.Sample(gTriLinearSamWash, washTexCoords.xy ).xyz * ambientColor * washTexCoords.z;
#endif // DX9
#endif // PER_FRAGMENT_PROP_WASH
    Ci = Ci + Cw;
#endif // PROPELLER_WASH

#ifdef PS30
    float doubleRefraction = max(0, dot(-vNorm, nNorm)) * (1.0 - dot(-vNorm, up));
    doubleRefraction += slopesAndFoam.w * BUBBLES;
    doubleRefraction *= tileFade;
    Ci += doubleRefractionColor * Clight * doubleRefraction * doubleRefractionIntensity;
#endif

#ifdef PS30
    // Fade out foam with distance to hide tiling, do alpha
    float3 Cfoam = foamColor * ambientColor;
#ifdef BREAKING_WAVES
    float foamAmount = (clamp(slopesAndFoam.w, 0.0, 1.0) * breakerFade + wakeNormalAndFoam.w) * tileFadeHoriz * foamBlend;
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
    float foamAmount = (clamp(slopesAndFoam.w, 0.0, 1.0) + wakeNormalAndFoam.w) * tileFadeHoriz * foamBlend;
    foamAmount = foamAmount * foamAmount;
    Ci = Ci + (Cfoam * foamAmount);
#endif

    float transparencyLocal = clamp(transparencyDepthBreakers.x, 0.0, 1.0);
    float alpha = hasHeightMap ? 1.0 - transparencyLocal : lerp(1.0 - transparencyLocal, 1.0, reflectivity);
    float4 waterColor = float4(Ci, alpha);
    float4 fogColor4 = float4(fogColor, hasHeightMap ? alpha : 1.0);
#else
    float4 waterColor = float4(Ci, 1.0);
    float4 fogColor4 = float4(fogColor.xyz, 1.0);
#endif

    float4 finalColor = lerp(fogColor4, waterColor, clamp(fogFactor, 0.0, 1.0));

    finalColor.xyz = pow(finalColor.xyz, float3(oneOverGamma, oneOverGamma, oneOverGamma));

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

