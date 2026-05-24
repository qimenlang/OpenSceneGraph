// ocean-frag-flat-fft.glsl
#define DETAIL
#define DETAIL_OCTAVE 2.0
#define DETAIL_BLEND 0.4
//#define VISUALIZE_HEIGHT
#define SPECULAR_BOOST 500.0
#define SPECULAR_DISTANCE_FACTOR 70.0
#define BUBBLES 0.1

#ifdef OPENGL32
in vec3 V;
in vec2 foamTexCoords;
in vec2 texCoords;
in vec2 noiseTexCoords;
in vec3 wakeSlopeAndFoam;
#ifdef PROPELLER_WASH
in vec3 washTexCoords;
in float activeWashWidth;
#endif
in float fogFactor;
in float transparency;
in float depth;
#ifdef BREAKING_WAVES
in float breaker;
in float breakerFade;
in vec2 breakerTexCoords;
#endif
#ifdef LEEWARD_DAMPENING
in float leewardDampening;
#endif
in vec4 vVertex_Eye_Space;
in vec4 vVertex_Projection_Space;
#ifdef PER_FRAGMENT_PROP_WASH
in vec3 propWashCoord;
#endif
#else
varying vec3 V;
varying vec2 foamTexCoords;
varying vec2 texCoords;
varying vec2 noiseTexCoords;
varying vec3 wakeSlopeAndFoam;
#ifdef PROPELLER_WASH
varying vec3 washTexCoords;
varying float activeWashWidth;
#endif
varying float fogFactor;
varying float transparency;
varying float depth;
#ifdef BREAKING_WAVES
varying float breaker;
varying float breakerFade;
varying vec2 breakerTexCoords;
#endif
#ifdef LEEWARD_DAMPENING
varying float leewardDampening;
#endif
varying vec4 vVertex_Eye_Space;
varying vec4 vVertex_Projection_Space;
#ifdef PER_FRAGMENT_PROP_WASH
varying vec3 propWashCoord;
#endif
#endif

uniform vec3 trit_userRotorCenter;
const float VORTEX_VISUAL_RADIUS = 30.0;

// From user-functions.glsl:
void user_lighting(in vec3 L
                   , in vec3 vVertex_World_Space, in vec3 vNormal_World_Space
                   , in vec4 vVertex_Projection_Space
                   , in float shininess
                   , inout vec3 ambient, inout vec3 diffuse, inout vec3 specular
                  );
void user_fog(in vec3 vNorm, inout vec4 waterColor, inout vec4 fogColor, inout float fogBlend);
void user_tonemap(in vec4 preToneMapColor, inout vec4 postToneMapColor);
void user_reflection_adjust(in vec4 envColor, in vec4 planarColor, in float planarReflectionBlend, inout vec4 reflectedColor);
float user_cloud_shadow_fragment();
void writeFragmentData(in vec4 finalColor, in vec4 Cdiffuse, in vec3 lightColor, in vec3 nNorm);
void user_diffuse_color( inout vec3 Cdiffuse, in vec3 CiNoLight, in vec3 Cwash, in vec4 reflectedColor,
                         in float reflectivity, in vec3 nNorm );

float finalWashWidth;

#ifdef PER_FRAGMENT_PROP_WASH
vec3 applyPropWash(in vec3 v, in vec3 finalAmbient)
{
    vec3 texCoords = vec3(0.0, 0.0, 0.0);
    finalWashWidth = -1.0;

    vec3 Cw = vec3(0.0, 0.0, 0.0);
#ifdef PROPELLER_WASH

    float numHits = 0.0;
    for (int i = 0; i < trit_numPropWashes; i++) {

        //if (washes[i].distFromSource == 0) continue;

        vec3 C = trit_washes[i].deltaPos;
        vec3 A = v - trit_washes[i].propPosition;
        float segmentLength = length(C);

        // Compute t
        float t0 = dot(C, A) / dot(C, C);

        // Compute enough overlap to account for curved paths.
        float overlap = (trit_washes[i].washWidth / segmentLength) * 0.5;

        if (t0 >= -overlap && t0 <= 1.0 + overlap) {

            // Compute distance from source
            float distFromSource = trit_washes[i].distFromSource - (1.0 - t0) * segmentLength;

            // Compute wash width
            float washWidth = (trit_washes[i].washWidth * pow(distFromSource, 1.0 / 4.5)) * 0.5;

            // Compute distance to line
            vec3 B = A - C;
            vec3 aCrossB = cross(A, B);
            float d = length(aCrossB) / segmentLength;

            // The direction of A X B indicates if we're 'left' or 'right' of the path
            float nd = d / washWidth;

            if (clamp(nd, 0.0, 1.0) == nd) {
                float t = t0;
                if(nd >= 0.0 && nd <= 1.0) {
                    vec3 up = vec3(0.0,0.0,1.0);
                    if(dot(up, aCrossB) >= 0.0) {
                        t = (1.0-nd*0.5)-0.5;
                    } else {
                        t = 0.5+(nd*0.5);
                    }
                }

                nd = t;

                texCoords.x = nd;
                // The t0 parameter from our initial distance test to the line segment makes
                // for a handy t texture coordinate

                //scale texture by 4 to reduce tiling
                texCoords.y =  (trit_washes[i].washLength - distFromSource) / (4.0*trit_washes[i].washWidth);

                // We stuff the blend factor into the r coordinate.

                //float blend = max(0.0, 1.0 - distFromSource / trit_washLength);
                float blend = mix(trit_washes[i].alphaEnd, trit_washes[i].alphaStart, t0);

                if(t < 0.1) {
                    blend *= max(0.0, 10.0*t);
                } else if(t > 0.9) {
                    blend *= max(0.0, (1.0-t)*10.0);
                }

                texCoords.z = clamp(blend, 0.0, 1.0);

                finalWashWidth = trit_washes[i].washWidth;

#ifdef OPENGL32
                Cw += texture(trit_washTex, texCoords.xy).xyz * finalAmbient * texCoords.z;
#else
                Cw += texture2D(trit_washTex, texCoords.xy).xyz * finalAmbient * texCoords.z;
#endif
                numHits += 1.0;
            }
        }
    }
#endif

    return numHits > 0.0 ? Cw / numHits : Cw;
}
#endif

//--------------------------------
// gerstner wave (fragment-local; trit_time passed from main)
//--------------------------------

#define ROTOR_RIPPLE_PI 3.14159265359

float gerstnerIrregular(vec2 uv, vec2 dir, float amp, float len, float speed, float declRatio, float time)
{
    float r = length(uv); 
    // 扭曲uv坐标，改变波形形状
    float twistRatio = 5.0;
    vec2 warp = vec2(sin(uv.x*twistRatio),cos(uv.y*twistRatio  + 10.0));
    float bTwist = step(0.01, r);
    // uv += warp * 0.2*bTwist;

    float k = 2.0 * ROTOR_RIPPLE_PI / len;
    float w = k * speed;

    vec2 delta = uv - vec2(0.0, 0.0);

    float angle = atan(delta.y, delta.x);
    // 扰动相位，增加八重对称的旋转效果
    // 八重对称方向因子：在八个方向上为1，其余方向小于1
    // cos(8*angle) 在 angle = 0°,45°,90°,135°,180°,225°,270°,315° 时等于1
    float phaseTwist = 0.5 + 0.5 * sin(7.0 * angle);

    float phase = k * dot(uv, dir) - w * time + phaseTwist * r * 20.0;

    // 两种衰减叠加
    float decl = exp(-r*declRatio);
    // 随距离衰减,0.25处最强；0.0-0.5范围内衰减
    float highpoint = 0.5;
    // decl = clamp(1.0-  pow(r - highpoint,2.0)/pow(highpoint,2.0), 0.0, 1.0);
    decl = clamp(1.0- 4.0*pow(r - highpoint,2.0), 0.0, 1.0);
    // 随距离衰减
    // decl *= exp(-r*declRatio);
    amp *= decl; 
    
    return amp*sin(phase);
}

float vortexRing(vec2 uv, float time)
{
    float r = length(uv);

    float ring = 0.0;
    // 1. 简单的同心圆波纹

    // ring = sinWave(uv);

    // 2. 叠加多个偏移的波纹，模拟旋翼下的复杂涡流
    // 振幅、波长、速度
    float amp = 0.05; // 波纹振幅 米
    float len = 0.04;
    float speed = 0.08;
    vec2 uv0 = uv-vec2(0.0,0.0);
    float declRatio = 10.0; // 衰减速率

    // 扭曲uv坐标，改变波形形状
    // vec2 warp = vec2(noise(uv*3.0 + iTime),noise(uv*3.0 + iTime + 10.0));
    // uv0 += warp * 0.03;
    // ring =  gerstner(uv0, normalize(uv0), amp, len, speed*0.8);
    ring = gerstnerIrregular(uv0, normalize(uv0), amp, len, speed, declRatio, time);

    return ring;
}


float oceanHeight(vec2 uv, float time)
{
    float height = 0.0;
    height += vortexRing(uv, time);
    // height += RotorRipple(uv);
    // height*=0.3;// 整体缩放系数，控制波浪高度
    return height;
}

// 中心差分（uv 按 VORTEX_VISUAL_RADIUS 归一化，需换算到世界尺度）
vec3 getNormalMid(vec2 p, float time)
{
    float e = 0.002;

    float hL = oceanHeight(p - vec2(e, 0.0), time);
    float hR = oceanHeight(p + vec2(e, 0.0), time);
    float hD = oceanHeight(p - vec2(0.0, e), time);
    float hU = oceanHeight(p + vec2(0.0, e), time);

    float scale = 1.0 / VORTEX_VISUAL_RADIUS;
    return normalize(vec3((hL - hR) * scale / (2.0 * e), (hD - hU) * scale / (2.0 * e), 1.0));
}

vec3 getNormal(vec2 p, float time)
{
    return getNormalMid(p, time);
}

void main()
{
#ifdef VISUALIZE_HEIGHT
    float rr = (-depth / 100.0) + 0.5;
    rr = clamp(rr, 0.0, 1.0);
    gl_FragColor = vec4(rr, rr, rr, 1.0);
    return;
#endif

    bool hasHeightMap = ( trit_hasHeightMap || trit_hasUserHeightMap );
    if (hasHeightMap && depth < 0.0) {
        discard;
        return;
    }

    if (trit_depthOnly) {
        writeFragmentData(vec4(0.0,0.0,0.0,1.0), vec4(0.0,0.0,0.0,0.0), trit_lightColor, vec3(0.0,0.0,0.0));
        return;
    }

    const float IOR = 1.33333;

    float tileFade = exp(-length(V) * trit_invNoiseDistance);
    float horizDist = length((trit_basis * V).xy);
    float horizDistNorm = horizDist * trit_invNoiseDistance;
    float tileFadeHoriz = exp(-horizDistNorm);

    vec3 vNorm = normalize(V);
#ifdef OPENGL32

#ifdef HIGHALT
    vec4 slopesAndFoamHigh = texture(trit_slopeFoamMap, texCoords, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamMed = texture(trit_slopeFoamMap, (texCoords + 0.1) * 0.25, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamLow = texture(trit_slopeFoamMap, (texCoords + 0.7) * 0.125, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamReallyLow = texture(trit_slopeFoamMap, (texCoords + 0.3) * 0.0625, trit_textureLODBias).xyzw;
    float altitude = abs((trit_basis * V).z) * trit_invZoom;
    float invBlendDist = 10.0 * trit_invNoiseDistance * trit_invZoom;
    float amp = 1.0 - min(1.0, altitude * invBlendDist);
    vec4 slopesAndFoam = slopesAndFoamHigh * amp;
    float totalAmp = amp;
    amp = min(0.5, altitude * invBlendDist);
    slopesAndFoam += slopesAndFoamMed * amp;
    totalAmp += amp;
    amp = min(0.5, altitude * invBlendDist * 0.5);
    slopesAndFoam += slopesAndFoamLow * amp;
    totalAmp += amp;
    amp = min(0.5, altitude *invBlendDist * 0.25);
    slopesAndFoam += slopesAndFoamReallyLow * amp;
    totalAmp += amp;

    slopesAndFoam /= totalAmp;
#else // HIGHALT
    vec4 slopesAndFoam = texture(trit_slopeFoamMap, texCoords, trit_textureLODBias).xyzw;
#endif // HIGHALT

    float fresnelScale = length(slopesAndFoam.xyz);

#ifdef DETAIL
    for (int n = 1; n <= NUM_OCTAVES; n++) {
        slopesAndFoam.xyz += texture(trit_slopeFoamMap, texCoords * DETAIL_OCTAVE * n, trit_textureLODBias).xyz * DETAIL_BLEND;
    }
#endif // DETAIL
    vec3 unscaledNoise = normalize( texture(trit_noiseTex, noiseTexCoords).xyz - vec3(0.5, 0.5, 0.5) );
#else // OPENGL32

#ifdef HIGHALT
    vec4 slopesAndFoamHigh = texture2D(trit_slopeFoamMap, texCoords, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamMed = texture2D(trit_slopeFoamMap, (texCoords + 0.1) * 0.25, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamLow = texture2D(trit_slopeFoamMap, (texCoords + 0.7) * 0.125, trit_textureLODBias).xyzw;
    vec4 slopesAndFoamReallyLow = texture2D(trit_slopeFoamMap, (texCoords + 0.3) * 0.0625, trit_textureLODBias).xyzw;
    float altitude = abs((trit_basis * V).z) * trit_invZoom;
    float invBlendDist = 10.0 * trit_invNoiseDistance * trit_invZoom;

    float amp = 1.0 - min(1.0, altitude * invBlendDist);
    vec4 slopesAndFoam = slopesAndFoamHigh * amp;
    float totalAmp = amp;
    amp = min(0.5, altitude * invBlendDist);
    slopesAndFoam += slopesAndFoamMed * amp;
    totalAmp += amp;
    amp = min(0.5, altitude * invBlendDist * 0.5);
    slopesAndFoam += slopesAndFoamLow * amp;
    totalAmp += amp;
    amp = min(0.5, altitude *invBlendDist * 0.25);
    slopesAndFoam += slopesAndFoamReallyLow * amp;
    totalAmp += amp;

    slopesAndFoam /= totalAmp;
#else // HIGHALT
    vec4 slopesAndFoam = texture2D(trit_slopeFoamMap, texCoords, trit_textureLODBias).xyzw;
#endif // HIGHALT
    // Uncomment to visualize Jacobian
    //gl_FragColor = vec4(slopesAndFoam.z, slopesAndFoam.z, slopesAndFoam.z, 1.0);
    //return;

    float fresnelScale = length(slopesAndFoam.xyz);

#ifdef DETAIL
    for (int n = 1; n <= NUM_OCTAVES; n++) {
        slopesAndFoam.xyz += texture2D(trit_slopeFoamMap, texCoords * DETAIL_OCTAVE * n, trit_textureLODBias).xyz * DETAIL_BLEND;
    }
#endif // DETAIL
    vec3 unscaledNoise = normalize(texture2D(trit_noiseTex, noiseTexCoords).xyz - vec3(0.5, 0.5, 0.5) );
#endif // else OPENGL32

    vec3 normalNoise = unscaledNoise * trit_noiseAmplitude;
    normalNoise.z = abs( normalNoise.z );

    slopesAndFoam.xyz = normalize(slopesAndFoam.xyz);
    vec4 finalSlopesAndFoam = slopesAndFoam;
#ifdef LEEWARD_DAMPENING
    finalSlopesAndFoam.xyz = mix(vec3(0.0,0.0,1.0), finalSlopesAndFoam.xyz, leewardDampening);
    finalSlopesAndFoam.w *= leewardDampening;
#endif

    vec3 wakeSlopeAndFoamClamped = clamp(wakeSlopeAndFoam.xyz, 0.0,1.0);
    vec3 wx = vec3(1.0, 0.0, wakeSlopeAndFoamClamped.x);
    vec3 wy = vec3(0.0, 1.0, wakeSlopeAndFoamClamped.y);
    vec3 wakeNormal = normalize(cross(wx, wy));

#ifdef BREAKING_WAVES
    float breakerFadeLocal = clamp(breakerFade, 0.0, 1.0);
    vec3 realNormal = mix(vec3(0.0,0.0,1.0), finalSlopesAndFoam.xyz, breakerFadeLocal);
#else
    vec3 realNormal = finalSlopesAndFoam.xyz;
#endif
    realNormal = normalize(realNormal + wakeNormal);

#ifdef SPARKLE
    float bias = horizDistNorm * horizDistNorm * -64.0;
    bias = mix(bias, -5.0, clamp(altitude * invBlendDist, 0.0, 1.0));
#ifdef OPENGL32
    vec3 specularSlopes = texture(trit_slopeFoamMap, texCoords, bias).xyz;
#else
    vec3 specularSlopes = texture2D(trit_slopeFoamMap, texCoords, bias).xyz;
#endif
    vec3 specNormal = trit_invBasis * normalize(specularSlopes);
#endif

    vec3 fadedNormal = mix(vec3(0.0,0.0,1.0), realNormal, tileFadeHoriz);

    vec3 nNorm = trit_invBasis * normalize(fadedNormal + (normalNoise * (1.0 - tileFade)));

    vec3 worldPos = V + trit_cameraPos;
    vec2 uv = (worldPos.xy - trit_userRotorCenter.xy) / VORTEX_VISUAL_RADIUS;
    float distUv = length(uv);
    float rippleMask = 1.0 - smoothstep(0.90, 1.0, distUv);
    if (rippleMask > 0.0) {
        // Additive slope perturbation on ocean normal (same pattern as wake/noise above).
        vec3 localDelta = getNormal(uv, trit_time) - vec3(0.0, 0.0, 1.0);
        vec3 worldDelta = trit_invBasis * localDelta;
        nNorm = normalize(nNorm + rippleMask * worldDelta);
#ifdef SPARKLE
        specNormal = normalize(specNormal + rippleMask * worldDelta);
#endif
    }

    vec3 P = reflect(vNorm, nNorm);

#if 0
    float reflectionHalfSphere = dot( P, trit_invBasis[2] /* == trit_invBasis * vec3(0,0,1) */ );

    if( reflectionHalfSphere < 0. ) { // make secondary reflection if reflected vector points down
        P = reflect(P, nNorm);
    }
#endif

#ifdef FAST_FRESNEL
    //float reflectivity = clamp( 0.02+0.97*pow((1.0-dot(P, nNorm)),5.0), 0.0, 1.0 );

    float r=(1.2-1.0)/(1.2+1.0);
    float reflectivity = max(0.0,min(1.0,r+(1.0-r)*pow((1.0-dot(nNorm,P)) * fresnelScale, 4.0)));

#else
    vec3 S = refract(P, nNorm, 1.0 / IOR);

    float cos_theta1 = (dot(P, nNorm));
    float cos_theta2 = (dot(S, -nNorm));

    float Fp = (cos_theta1 - (IOR * cos_theta2)) /
               ((IOR * cos_theta2) + cos_theta1);
    float Fs = (cos_theta2 - (IOR * cos_theta1)) /
               ((IOR * cos_theta1) + cos_theta2);
    Fp = Fp * Fp;
    Fs = Fs * Fs;

    float reflectivity = clamp((Fs + Fp) * 0.5, 0.0, 1.0);
#endif

    // No reflections on foam.
    reflectivity = mix(reflectivity, 0.0, clamp(wakeSlopeAndFoam.z, 0.0, 1.0));

    reflectivity *= trit_reflectivityScale;

    vec4 envColor;

    if (trit_hasEnvMap) {
        P = trit_basis * P;
        P.z = max(0, P.z);
        P = trit_invBasis * normalize(P);

#ifdef OPENGL32
        envColor = textureLod(trit_cubeMap, trit_cubeMapMatrix * P, 0);
#else
        envColor = textureCubeLod(trit_cubeMap, trit_cubeMapMatrix * P, 0);
#endif
    } else {
        envColor = vec4(trit_ambientColor, 1.0);
    }

    vec4 reflectedColor = envColor;

    if( trit_hasPlanarReflectionMap ) {
        vec3 up = trit_invBasis[2]; // == trit_invBasis * vec3( 0.0, 0.0, 1.0 )
        // perturb view vector by normal xy coords multiplied by displacement scale
        // when we do it in world oriented space this perturbation is equal to:
        // ( nNorm - dot( nNorm, up ) * up ) == trit_invBasis * vec3( ( trit_basis * nNorm ).xy, 0 )
        vec3 vNormPerturbed = vNorm + ( nNorm - dot( nNorm, up ) * up ) * trit_planarReflectionDisplacementScale;
        vec3 tc = trit_planarReflectionMapMatrix * vNormPerturbed;
#ifdef OPENGL32
        vec4 planarColor = textureProj( trit_planarReflectionMap, tc );
#else
        vec2 tcProj = vec2(tc.x / tc.z, tc.y / tc.z);
        vec4 planarColor = texture2D(trit_planarReflectionMap, tcProj);
#endif

        //planarColor.a = planarColor.a > 0.9 ? planarColor.a : 0.0;

        reflectedColor = mix( envColor, planarColor, planarColor.a * trit_planarReflectionBlend);

        user_reflection_adjust(envColor, planarColor, trit_planarReflectionBlend, reflectedColor);
    } else {
        user_reflection_adjust(envColor, vec4(0.0,0.0,0.0,0.0), 1.0, reflectedColor);
    }

    // This makes the water more visually interesting. For realism, comment out.
    //reflectivity = pow(reflectivity, 0.8);

    vec3 finalAmbient = trit_ambientColor;
    vec3 finalDiffuse = trit_lightColor * max(0,dot(trit_L, nNorm));

    vec3 refractedLight, LNorm;

    if (trit_underwater) {
        LNorm = normalize(trit_L);
        refractedLight = refract(LNorm, trit_invBasis[2], 1.0 / 1.333);
        nNorm = -nNorm;
    } else {
        refractedLight = LNorm = normalize(trit_L);
    }

#ifdef SPARKLE
    vec3 R = reflect(refractedLight, specNormal);
#else
    vec3 R = reflect(refractedLight, nNorm);
#endif

    float spec = max(0.0, dot(vNorm, R));

#ifndef HDR
    vec3 finalSpecular = trit_lightColor * min(1.0, pow(spec, trit_shininess + horizDist * SPECULAR_DISTANCE_FACTOR) * trit_sunIntensity * SPECULAR_BOOST * reflectivity);
#else
    vec3 finalSpecular = trit_lightColor * pow(spec, trit_shininess + horizDist * SPECULAR_DISTANCE_FACTOR) * trit_sunIntensity * SPECULAR_BOOST * reflectivity;
#endif

    // Allow lighting overrides in the user-functions.glsl
    user_lighting(trit_L, V, nNorm
                  , vVertex_Projection_Space, trit_shininess, finalAmbient, finalDiffuse, finalSpecular);

    float shadow = user_cloud_shadow_fragment();

    finalDiffuse *= vec3(shadow);

    finalSpecular *= vec3(shadow);

    vec3 Csunlight = finalSpecular;

#ifndef HDR
    vec3 Clight = min(finalAmbient + finalDiffuse, 1.0);
#else
    vec3 Clight = finalAmbient + finalDiffuse;
#endif

    vec3 Cskylight = mix(trit_refractColor * Clight, reflectedColor.rgb  * vec3(shadow), reflectivity);

#ifdef OPENGL32
    vec3 Clightfoam = texture(trit_lightFoamTex, foamTexCoords).xyz;
    Clightfoam += texture(trit_lightFoamTex, foamTexCoords * 1.7).xyz;
    Clightfoam += texture(trit_lightFoamTex, foamTexCoords * 0.3).xyz;
    Clightfoam *= finalAmbient;
#else
    vec3 Clightfoam = texture2D(trit_lightFoamTex, foamTexCoords).xyz;
    Clightfoam += texture2D(trit_lightFoamTex, foamTexCoords * 1.7).xyz;
    Clightfoam += texture2D(trit_lightFoamTex, foamTexCoords * 0.3).xyz;
    Clightfoam *= finalAmbient;
#endif

    vec3 CiNoLight = vec3(0.0,0.0,0.0);

    // Fade out foam with distance to hide tiling
#ifdef BREAKING_WAVES
    float foamAmount = clamp(finalSlopesAndFoam.w, 0.0, 1.0) * breakerFadeLocal;
    foamAmount = foamAmount * foamAmount;
    vec3 foamColor = Clightfoam * foamAmount;


#ifdef OPENGL32
    vec3 breakerTex = texture(trit_breakerTex, breakerTexCoords).xyz;
#else
    vec3 breakerTex = texture2D(trit_breakerTex, breakerTexCoords).xyz;
#endif

    float breakerNoise = max(0.0, (1.0 - abs(unscaledNoise.x * 6.0)));
    CiNoLight += breakerTex * breaker * tileFadeHoriz * breakerNoise;
#else
    float foamAmount = (clamp(finalSlopesAndFoam.w, 0.0, 1.0));
    foamAmount = foamAmount * foamAmount;
    vec3 foamColor = Clightfoam * foamAmount;
#endif

    CiNoLight += (foamColor * tileFadeHoriz * trit_foamBlend);
    CiNoLight += Clightfoam * wakeSlopeAndFoamClamped.z * trit_foamBlend;

    vec3 Cwash = vec3(0.0,0.0,0.0);

#ifdef PROPELLER_WASH

#ifdef PER_FRAGMENT_PROP_WASH
    Cwash = applyPropWash(propWashCoord, finalAmbient);
#else
    finalWashWidth = activeWashWidth;

    float washTexCoordZClamped = clamp(washTexCoords.z, 0.0, 1.0);

#ifdef OPENGL32
    Cwash = texture(trit_washTex, washTexCoords.xy).xyz * finalAmbient * washTexCoordZClamped;
#else
    Cwash = texture2D(trit_washTex, washTexCoords.xy).xyz * finalAmbient * washTexCoordZClamped;
#endif
#endif

    CiNoLight += Cwash * vec3(shadow);
#endif

    float doubleRefraction = max(0.0,dot(-vNorm,nNorm)) * (1.0 - dot(-vNorm, trit_invBasis[2]));
    doubleRefraction += finalSlopesAndFoam.w * BUBBLES;
    doubleRefraction *= tileFade;

    vec3 Ci = Cskylight + Csunlight + CiNoLight;
    Ci += trit_doubleRefractionColor * Clight * doubleRefraction * trit_doubleRefractionIntensity;

    float transparencyLocal = clamp(transparency, 0.0, 1.0);
    float alpha = hasHeightMap ? 1.0 - transparencyLocal : mix(1.0 - transparencyLocal, 1.0, reflectivity);
    vec4 waterColor = vec4(Ci, alpha);
    vec4 fogColor4 = vec4(trit_fogColor, hasHeightMap ? alpha : 1.0);

    float fogBlend = clamp(fogFactor, 0.0, 1.0);

    // Allow user override of fog in user-functions.glsl
    user_fog(V, waterColor, fogColor4, fogBlend);

    vec4 finalColor = mix(fogColor4, waterColor, fogBlend);

    finalColor.xyz = pow(finalColor.xyz, vec3(trit_oneOverGamma, trit_oneOverGamma, trit_oneOverGamma));

#ifndef HDR
    vec4 toneMappedColor = clamp(finalColor, 0.0, 1.0);
#else
    vec4 toneMappedColor = finalColor;
#endif

    user_tonemap(finalColor, toneMappedColor);

    toneMappedColor.w *= trit_transparency;

    vec3 Cdiffuse = vec3(0.0,0.0,0.0);
    user_diffuse_color( Cdiffuse, CiNoLight, Cwash, reflectedColor, reflectivity, nNorm );

    writeFragmentData(toneMappedColor, vec4( Cdiffuse, alpha ), trit_lightColor, nNorm);

}
