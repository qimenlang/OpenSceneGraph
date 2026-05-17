// projgrid-flat-fft-patch.glsl
#define PI 3.14159265
#define TWOPI (2.0 * 3.14159265)

#ifdef OPENGL32
in vec3 vertex;
out vec3 V;
out vec2 foamTexCoords;
out vec2 texCoords;
out vec2 noiseTexCoords;
out vec3 wakeSlopeAndFoam;
#ifdef PROPELLER_WASH
out vec3 washTexCoords;
out float activeWashWidth;
#endif
out float fogFactor;
out float transparency;
out float depth;
#ifdef BREAKING_WAVES
out float breaker;
out float breakerFade;
out vec2 breakerTexCoords;
#endif
#ifdef LEEWARD_DAMPENING
out float leewardDampening;
#endif
out vec4 vVertex_Eye_Space;
out vec4 vVertex_Projection_Space;
#ifdef PER_FRAGMENT_PROP_WASH
out vec3 propWashCoord;
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

void user_intercept(in vec3 worldPosition, in vec3 localPosition, in vec4 eyePosition, in vec4 projectedPosition);
vec4 overridePosition(in vec4 position);
float user_get_depth( in vec3 worldPos );

void getDepthFromHeightmap(in vec3 worldPos)
{
    vec2 texCoord = (trit_heightMapMatrix * vec4(worldPos, 1.0)).xy;
    if (clamp(texCoord, vec2(0.0, 0.0), vec2(1.0, 1.0)) == texCoord) {
#ifdef OPENGL32
        float height = texture(trit_heightMap, texCoord).x;
#else
        float height = texture2D(trit_heightMap, texCoord).x;
#endif
        height = height*trit_heightMapRangeOffset.x + trit_heightMapRangeOffset.y;
        depth = -(height - trit_seaLevel);
    }
}

void getDepthFromDepthmap(in vec3 worldPos)
{
    vec4 clipPos = trit_projection * trit_modelview * vec4(worldPos, 1.0);
    vec3 ndcPos = clipPos.xyz / clipPos.w;
    vec2 texCoord = ndcPos.xy * 0.5 + 0.5;
#ifdef OPENGL32
    float terrainZ = texture(trit_depthMap, texCoord).x;
#else
    float terrainZ = texture2D(trit_depthMap, texCoord).x;
#endif

    ndcPos.z = mix(trit_zNearFar.x, trit_zNearFar.y, terrainZ);

    clipPos.w = trit_projection[3][2] / (ndcPos.z - (trit_projection[2][2] / trit_projection[2][3]));
    clipPos.xyz = ndcPos * clipPos.w;

    vec4 terrainWorld = trit_invModelviewProj * clipPos;

    depth = length(terrainWorld.xyz - worldPos);
}

void computeTransparency(in vec3 worldPos)
{
    depth = DEFAULT_DEPTH;
    // Compute depth at this position
    if ( trit_hasUserHeightMap ) {
        depth = user_get_depth(worldPos);
    } else if (trit_hasHeightMap) {
        getDepthFromHeightmap(worldPos);
    } else if (trit_hasDepthMap) {
        getDepthFromDepthmap(worldPos);
    } else {
        vec3 up = trit_invBasis * vec3(0.0, 0.0, 1.0);
        vec3 l = -up;
        vec3 l0 = worldPos;
        vec3 n = trit_floorPlaneNormal;
        vec3 p0 = trit_floorPlanePoint;
        float numerator = dot((p0 - l0), n);
        float denominator = dot(l, n);
        if (abs(denominator) > 0.0001) {
            depth = numerator / denominator;
        }
    }

    // Compute fog at this distance underwater
    float fogExponent = abs(depth) * trit_fogDensityBelow;
    transparency = clamp(exp(-abs(fogExponent)), 0.0, 1.0);
}

float applyBreakingWaves(in vec3 v, in float fade, in vec3 worldPos)
{
    float finalz = 0.0;

#ifdef BREAKING_WAVES
    breaker = 0.0;
    breakerTexCoords = vec2(0.0,1.0);

    bool hasHeightMap = (trit_hasHeightMap || trit_hasUserHeightMap);
    if (hasHeightMap && trit_breakerAmplitude > 0 && depth > 1.0 && depth < trit_breakerWavelength * 0.5) {

        vec3 direction = trit_breakerDirection;
        float alpha = 1.0;

#ifdef BREAKING_WAVES_MAP
        bool gotDirection = false;
        if (trit_hasBreakingWaveMap) {
            vec2 texCoord = (trit_breakingWaveMapMatrix * vec4(worldPos, 1.0)).xy;
            if (clamp(texCoord, vec2(0.0, 0.0), vec2(1.0, 1.0)) == texCoord) {
#ifdef OPENGL32
                vec4 t = texture(trit_breakingWaveMap, texCoord);
#else
                vec4 t = texture2D(trit_breakingWaveMap, texCoord);
#endif

                vec3 d = t.xyz;
                alpha = t.w;

                if (dot(d,d) == 0.0) {
                    direction = normalize(d);
                    gotDirection = true;
                }
            }
        }
        if (!gotDirection) return 0.0;
#endif

        float halfWavelength = trit_breakerWavelength * 0.5;
        float scaleFactor = ((depth - halfWavelength) / halfWavelength);
        float wavelength = trit_breakerWavelength + scaleFactor * trit_breakerWavelengthVariance;

        float halfKexp = trit_kexp * 0.5;
        scaleFactor = (depth - halfKexp) / halfKexp;
        scaleFactor *= 1.0 + trit_steepnessVariance;
        float k = (trit_kexp + scaleFactor) * (1.0 - fade);

        vec3 localDir = trit_basis * direction;
        localDir.z = 0.0;
        localDir = normalize(localDir);
        float dotResult = dot(-localDir.xy, v.xy) * TWOPI / wavelength;

        finalz = (dotResult + trit_breakerPhaseConstant * trit_time);

        vec3 binormal = cross(vec3(0.0, 0.0, 1.0), localDir);
        breakerTexCoords.x = dot(binormal.xy, v.xy);
        breakerTexCoords.x /= trit_foamScale * 8.0;

#define OFFSET (PI * 0.3)
        float y = mod(finalz, TWOPI);
        if (y < OFFSET) return 0.0;
        float num = PI - y;
        float den = PI - OFFSET;
        breakerTexCoords.y = num / den;
        float sinz = sin(finalz);

        finalz = (sinz + 1.0) * 0.5;

        finalz = trit_breakerAmplitude * pow(finalz, max(1.0, k));

        finalz *= 1.0 - min(depth * trit_breakerDepthFalloff / halfWavelength, 1.0);
        finalz *= alpha;
        finalz = max(0.0, finalz);

        breaker = clamp(sinz, 0.0, 1.0);

        breaker *= 1.0 - min((depth * 3.0 * trit_breakerDepthFalloff) / halfWavelength, 1.0);
        breaker *= alpha;

        // Hide the backs of waves if we're transparent
        float opacity = 1.0 - transparency;
        finalz = mix(0.0, finalz, pow(opacity, 6.0));
    }
#endif

    return finalz;
}

void applyCircularWaves(inout vec3 v, float fade)
{
    vec2 slope = vec2(0.0, 0.0);
    float disp = 0.0;

    int i;
    for (i = 0; i < trit_numCircularWaves; i++) {

        vec2 D = (v - trit_circularWaves[i].position).xy;
        float dist = length(D);

        float r = dist - trit_circularWaves[i].radius;
        if (abs(r) < trit_circularWaves[i].halfWavelength) {

            float amplitude = trit_circularWaves[i].amplitude;

            float theta = trit_circularWaves[i].k * r;
            disp += amplitude * cos(theta);
            float derivative = amplitude * -sin(theta);
            slope +=  D * (derivative / dist);
        }
    }

    v.z += disp * fade;

    wakeSlopeAndFoam.z += max(0.0, disp * fade);
    wakeSlopeAndFoam.xy += slope * fade;
}

void applyLeewardDampening(in vec3 v)
{
#ifdef LEEWARD_DAMPENING
    int i;

    float maxDampening = 0.0;

    for (i = 0; i < trit_numLeewardDampeners; i++) {
        vec3 bowPos = trit_leewardDampeners[i].bowPos;
        vec3 sternPos = trit_leewardDampeners[i].sternPos;
        vec3 center = (bowPos + sternPos) * 0.5;
        vec3 P = v - center;
        vec3 axis = normalize(bowPos - sternPos);
        float radius = length(bowPos - sternPos) * 0.5;

        float blockage = 1.0 - abs(dot(axis, trit_windDir));
        float directional = max(dot(P, trit_windDir), 0);
        float distance = max(1.0 - length(P) / radius, 0);

        float dampening = blockage * directional * distance * trit_leewardDampeningStrength;
        dampening *= trit_leewardDampeners[i].velocityDampening;
        maxDampening = max(dampening, maxDampening);
    }

    leewardDampening = 1.0 - maxDampening;
    leewardDampening = clamp(leewardDampening, 0.0, 1.0);
#endif
}

void applyKelvinWakes(inout vec3 v, float fade)
{

#ifdef KELVIN_WAKES
    vec2 slope = vec2(0.0, 0.0);
    float foam = 0.0;
    float hullWakeFoam = 0.0;

    int i;
    for (i = 0; i < trit_numKelvinWakes; i++) {
        vec3 X0 = trit_wakes[i].position - trit_wakes[i].shipPosition;
        vec3 T = normalize(X0);
        vec3 N = vec3(0,0,1);
        vec3 B = normalize(cross(N, T));

        vec3 P = v - trit_wakes[i].shipPosition;
        vec3 X;
        X.x = dot(P.xy, T.xy);
        X.y = dot(P.xy, B.xy);

        float xLen = length(X0);
        vec2 tc;
        tc.x = X.x / (1.54 * xLen);
        tc.y = (X.y) / (1.54 * xLen) + 0.5;

        if (clamp(tc, 0.01, 0.99) == tc) {
#ifdef OPENGL32
            vec4 displacementSample = texture(trit_displacementTexture, tc);
#else
            vec4 displacementSample = texture2D(trit_displacementTexture, tc);
#endif
            float displacement = displacementSample.w;

            displacement *= trit_wakes[i].amplitude;// * fade;

            foam += displacement * displacement * trit_wakes[i].foamAmount;

            vec3 normal = normalize(displacementSample.xyz * 2.0 - 1.0);
            float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
            mat3 TBN = mat3( T * invmax, B * invmax, N );

            normal = TBN * normal;

            if (clamp(normal.xy, vec2(-0.05, -0.05), vec2(0.05, 0.05)) == normal.xy) {
                normal = vec3(0.0,0.0,1.0);
            }

            normal.xy *= min(1.0, trit_wakes[i].amplitude);
            normal = normalize(normal);

            v.z = max(v.z,displacement);
            vec2 s = vec2(normal.x / normal.z, normal.y / normal.z);
            slope = max(slope,s);
            //foam += length(s) * trit_wakes[i].foamAmount;
        }

        if(trit_wakes[i].hullWakeLengthReciprocal > 0.0) {
            tc.y = X.x * (trit_wakes[i].hullWakeLengthReciprocal);
            tc.x = (X.y) / (trit_wakes[i].hullWakeWidth) + 0.5;

            if (clamp(tc, 0.01, 0.99) == tc) {
#ifdef OPENGL32
                vec4 hullWakeSample = texture(trit_hullWakeTexture, tc);
#else
                vec4 hullWakeSample = texture2D(trit_hullWakeTexture, tc);
#endif

                if(hullWakeSample.z > 0.0) {
                    float ty = X.x * trit_wakes[i].hullWakeLengthReciprocal;
                    float t = length(P) * trit_wakes[i].hullWakeLengthReciprocal;


                    if(ty < 0.1) {
                        float tScale = 10.0*ty;
                        hullWakeFoam = hullWakeSample.z * trit_wakes[i].foamAmount * tScale;
                    } else if(ty > 0.9) {
                        float tScale = 1.0-(10.0*(ty-0.9));
                        hullWakeFoam = hullWakeSample.z * trit_wakes[i].foamAmount * tScale;
                    } else {
                        hullWakeFoam = hullWakeSample.z * trit_wakes[i].foamAmount;
                    }
                }
            }
        }
    }

    foam = min(1.0, foam+hullWakeFoam);
    wakeSlopeAndFoam.z += foam;
    wakeSlopeAndFoam.xy += slope * fade;

#endif
}

void applyPropWash(in vec3 v)
{
#ifdef PROPELLER_WASH

    washTexCoords = vec3(0.0, 0.0, 0.0);
    activeWashWidth = -1.0;

    for (int i = 0; i < trit_numPropWashes; i++) {

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
                float t = nd;

                vec3 up = vec3(0,0,1.0);
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
                washTexCoords.y =  (trit_washes[i].washLength - distFromSource) / (4.0*trit_washes[i].washWidth);



                float blend = mix(trit_washes[i].alphaEnd, trit_washes[i].alphaStart, t0);
                blend = clamp(blend, 0.0, 1.0);

                if(nd <= 0.1 || nd >= 0.9) {
                    blend = 0;
                } else if(nd <= 0.2) {
                    blend *= (nd-.1)*10.0;
                } else if (nd >= .8) {
                    blend *= (.9-nd)*10.0;
                }

                washTexCoords.z = blend;


                activeWashWidth = trit_washes[i].washWidth;
            }
        }
    }
#endif
}

void displace(inout vec3 v)
{
    float fade = 1.0 - smoothstep(0.0, 1.0, length(v - trit_cameraPos) * trit_invDampingDistance);

#ifdef BREAKING_WAVES
    // Fade out waves in the surge zone
    float depthFade = 1.0;

    if (trit_surgeDepth > 0) {
        depthFade = min(trit_surgeDepth, depth) / trit_surgeDepth;
        depthFade = clamp(depthFade, 0.0, 1.0);
    }

    fade *= depthFade;
    breakerFade = depthFade;
#endif

    // Transform so z is up
    vec3 localV = trit_basis * v;

    texCoords = localV.xy / trit_textureSize;

#ifdef OPENGL32
    vec3 displacement = texture(trit_displacementMap, texCoords).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += texture(trit_displacementMap, texCoords * 2.0).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#else
    vec3 displacement = texture2D(trit_displacementMap, texCoords).xyz;
#ifdef DISPLACEMENT_DETAIL
    displacement += texture2D(trit_displacementMap, texCoords * 2.0).xyz * DISPLACEMENT_DETAIL_FACTOR;
#endif
#endif

    // Hide the backs of waves if we're transparent
    float opacity = 1.0 - transparency;
    displacement.z = mix(0.0, displacement.z, pow(opacity, 6.0));

    localV.xy += displacement.xy * fade;

#if (defined(KELVIN_WAKES) || defined(PROPELLER_WASH))
    if (trit_doWakes) {
        wakeSlopeAndFoam.xyz = vec3(0.0, 0.0, 0.0);
        applyKelvinWakes(localV, fade);
        applyCircularWaves(localV, fade);
#ifdef PER_FRAGMENT_PROP_WASH
        propWashCoord = localV;
#else
        applyPropWash(localV);
#endif
        applyLeewardDampening(localV);
    } else {
#ifdef PROPELLER_WASH
        washTexCoords = vec3(0.0, 0.0, 0.0);
#endif
    }
#endif

#ifdef LEEWARD_DAMPENING
    localV.z += displacement.z * fade * leewardDampening;
#else
    localV.z += displacement.z * fade;
#endif

    localV.z += applyBreakingWaves(localV, fade, v);

    foamTexCoords = localV.xy / trit_foamScale;
    noiseTexCoords = texCoords * 0.03;

    v = trit_invBasis * localV;
}

void main()
{
    wakeSlopeAndFoam = vec3( 0. );
    transparency = 0.0;

#ifdef LEEWARD_DAMPENING
    leewardDampening = 1.0;
#endif

#ifdef OPENGL32
    vec4 worldPos = trit_modelMatrix * vec4(vertex.x, vertex.y, vertex.z, 1.0);
#else
    vec4 worldPos = trit_modelMatrix * gl_Vertex;
#endif

    computeTransparency(worldPos.xyz);

    // Displace
    displace(worldPos.xyz);

    V = (worldPos.xyz - trit_cameraPos);

    // Project it back again, apply depth offset.
    vec4 v = trit_modelview * worldPos;

    vVertex_Eye_Space = v;

    v.w -= trit_depthOffset;

    vec4 vVertInProjSpc = trit_projection * v;
    vVertex_Projection_Space = vVertInProjSpc;
    if (trit_bypassOverridePosition && trit_depthOnly) {
        gl_Position = vVertInProjSpc;
    } else {
        gl_Position = overridePosition(vVertInProjSpc);
    }

    float fogExponent = length(V.xyz) * trit_fogDensity;
    fogFactor = clamp(exp(-(fogExponent * fogExponent)), 0.0, 1.0);

    wakeSlopeAndFoam.z = min(1.0, wakeSlopeAndFoam.z);

    user_intercept(worldPos.xyz, worldPos.xyz, vVertex_Eye_Space, vVertex_Projection_Space);
}