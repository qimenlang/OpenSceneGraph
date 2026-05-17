// projgrid-ellipsoid-fft.glsl
#define PRECISION_GUARD 2.0
#define EPSILON 0.0000001

#ifdef OPENGL32
in vec2 vertex;
out vec3 V;
out vec2 foamTexCoords;
out vec2 texCoords;
out vec2 noiseTexCoords;
out vec3 up;
out vec4 wakeNormalsAndFoam;
out float fogFactor;
out float transparency;
#ifdef PROPELLER_WASH
out vec3 washTexCoords;
out float activeWashWidth;
#endif
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
out vec3 propWashCoords;
out vec3 localPropWashCoords;
#endif
#else
varying vec3 V;
varying vec2 foamTexCoords;
varying vec2 texCoords;
varying vec2 noiseTexCoords;
varying vec3 up;
varying vec4 wakeNormalsAndFoam;
varying float fogFactor;
varying float transparency;
#ifdef PROPELLER_WASH
varying vec3 washTexCoords;
varying float activeWashWidth;
#endif
varying float depth;
#ifdef BREAKING_WAVES
varying float breaker;
varying float breakerFade;
varying vec2 breakerTexCoords;
#endif
#ifdef LEEWARD_DAMPENING
varying float leewardDampening;
#endif
varying vec4 vVertex_Projection_Space;
varying vec4 vVertex_Eye_Space;
#ifdef PER_FRAGMENT_PROP_WASH
varying vec3 propWashCoords;
varying vec3 localPropWashCoords;
#endif
#endif

void user_intercept(in vec3 worldPosition, in vec3 localPosition, in vec4 eyePosition, in vec4 projectedPosition);
vec4 overridePosition(in vec4 position);
float user_get_depth( in vec3 worldPos );

#ifdef DOUBLE_PRECISION
dvec3 user_horizon(in dvec3 intersect);
#else
vec3 user_horizon(in vec3 intersect);
#endif

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

void getDepthFromDepthmap(in vec3 localPos)
{
    vec4 clipPos = trit_projection * trit_modelview * vec4(localPos, 1.0);
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

    depth = length(terrainWorld.xyz) - length(localPos);
}

void computeTransparency(in vec3 worldPos, in vec3 localPos)
{
    depth = DEFAULT_DEPTH;
    // Compute depth at this position
    if ( trit_hasUserHeightMap ) {
        depth = user_get_depth(worldPos);
    } else if (trit_hasHeightMap) {
        getDepthFromHeightmap(worldPos);
    } else if (trit_hasDepthMap) {
        getDepthFromDepthmap(localPos);
    } else {
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

void applyBreakingWaves(inout vec3 v, in float fade, in vec3 worldPos)
{
#ifdef BREAKING_WAVES
    breaker = 0.0;
    breakerTexCoords = vec2(0.0, 1.0);

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

                if (dot(d,d) > 0) {
                    direction = normalize(d);
                    gotDirection = true;
                }
            }
        }
        if (!gotDirection) return;
#endif

        float halfWavelength = trit_breakerWavelength * 0.5;
        float scaleFactor = ((depth - halfWavelength) / halfWavelength);
        float wavelength = trit_breakerWavelength + scaleFactor * trit_breakerWavelengthVariance;

        float halfKexp = trit_kexp * 0.5;
        scaleFactor = (depth - halfKexp) / halfKexp;
        scaleFactor *= 1.0 + trit_steepnessVariance;
        float k = (trit_kexp + scaleFactor) * (1.0 - fade);

        vec3 horizDir = trit_basis * -direction;
        horizDir.z = 0.0;
        horizDir = normalize(horizDir);
        float dotResult = dot(horizDir.xy, v.xy) * TWOPI / wavelength;

        float finalz = (dotResult + trit_breakerPhaseConstant * trit_time);

        vec3 binormal = cross(vec3(0.0, 0.0, 1.0), -horizDir);
        breakerTexCoords.x = dot(binormal.xy, v.xy);
        breakerTexCoords.x /= trit_foamScale * 8.0;

#define OFFSET (PI * 0.3)
        float y = mod(finalz, TWOPI);
        if (y < OFFSET) return;

        float num = PI - y;
        float den = PI - OFFSET;
        breakerTexCoords.y = num / den;
        float sinz = sin(finalz);

        finalz = (sinz + 1.0) * 0.5;

        finalz = trit_breakerAmplitude * pow(finalz, k);

        finalz *= 1.0 - min(depth * trit_breakerDepthFalloff / halfWavelength, 1.0);
        finalz *= alpha;
        finalz = max(0, finalz);

        breaker = clamp(sinz, 0.0, 1.0);

        breaker *= 1.0 - min((depth * 3.0 * trit_breakerDepthFalloff) / halfWavelength, 1.0);
        breaker *= alpha;

        // Hide the backs of waves if we're transparent
        float opacity = 1.0 - transparency;
        finalz = mix(0.0, finalz, pow(opacity, 6.0));

        v.z += finalz;
    }
#endif
}

void applyCircularWaves(inout vec3 v, in vec3 localPos, float fade, out vec2 slope, out float foam)
{
    int i;

    vec3 slope3 = vec3(0.0, 0.0, 0.0);
    float disp = 0.0;

    for (i = 0; i < trit_numCircularWaves; i++) {

        vec3 D = (localPos - trit_circularWaves[i].position);
        float dist = length(D);

        float r = dist - trit_circularWaves[i].radius;
        if (abs(r) < trit_circularWaves[i].halfWavelength) {

            float amplitude = trit_circularWaves[i].amplitude;

            float theta = trit_circularWaves[i].k * r;
            disp += amplitude * cos(theta);
            float derivative = amplitude * -sin(theta);
            slope3 +=  D * (derivative / dist);
        }
    }

    v.z += disp * fade;

    slope = (trit_basis * slope3).xy * fade;

    foam = max(0.0, disp * fade);
}

void applyLeewardDampening(in vec3 v)
{
#ifdef LEEWARD_DAMPENING
    int i;

    float maxDampening = 0;

    for (i = 0; i < trit_numLeewardDampeners; i++) {
        vec3 bowPos = trit_leewardDampeners[i].bowPos;
        vec3 sternPos = trit_leewardDampeners[i].sternPos;
        vec3 center = (bowPos + sternPos) * 0.5;
        vec3 P = v - center;
        vec3 axis = normalize(bowPos - sternPos);
        float radius = length(bowPos - sternPos) * 0.5;

        float blockage = 1.0 - abs(dot(trit_basis * axis, trit_windDir));
        float directional = max(dot(trit_basis * P, trit_windDir), 0);
        float distance = max(1.0 - length(P) / radius, 0);

        float dampening = blockage * directional * distance * trit_leewardDampeningStrength;
        dampening *= trit_leewardDampeners[i].velocityDampening;
        maxDampening = max(dampening, maxDampening);
    }

    leewardDampening = 1.0 - maxDampening;
    leewardDampening = clamp(leewardDampening, 0.0, 1.0);
#endif
}

void applyKelvinWakes(inout vec3 v, in vec3 localPos, float fade, inout vec2 slope, inout float foam)
{
#ifdef KELVIN_WAKES
    vec2 accumSlope = vec2(0,0);
    float disp = 0.0;
    float accumFoam = 0;
    float hullWakeFoam = 0;

    int i;
    for (i = 0; i < trit_numKelvinWakes; i++) {
        vec3 X0 = trit_wakes[i].position - trit_wakes[i].shipPosition;
        vec3 T = normalize(X0);
        vec3 N = up;
        vec3 B = normalize(cross(N, T));

        vec3 P = localPos - trit_wakes[i].shipPosition;
        vec3 X;
        X.x = dot(P, T);
        X.y = dot(P, B);
        //      X.z = dot(P, N);

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

            displacement *= trit_wakes[i].amplitude;

            accumFoam += displacement * displacement * trit_wakes[i].foamAmount;

            vec3 normal = normalize(displacementSample.xyz * 2.0 - 1.0);
            float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) );
            mat3 TBN = mat3( T * invmax, B * invmax, N );

            normal = TBN * normal;

            // Convert to z-up
            normal = trit_basis * normal;
            if (clamp(normal.xy, vec2(-0.05, -0.05), vec2(0.05, 0.05)) == normal.xy) {
                normal = vec3(0.0,0.0,1.0);
            }
            normal.xy *= min(1.0, trit_wakes[i].amplitude);
            normal = normalize(normal);

            disp += displacement;

            vec2 thisSlope =  vec2(normal.x / normal.z, normal.y / normal.z);
            accumSlope += thisSlope;
            //accumFoam += length(thisSlope) * trit_wakes[i].foamAmount;
            //accumFoam += (1.0 - exp(-displacement)) * trit_wakes[i].foamAmount;

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

    v.z += disp * fade;

    accumFoam += min(1.0, hullWakeFoam);
    foam += min(1.0, accumFoam);
    slope += accumSlope * fade;

#endif
}

void applyPropWash(in vec3 v, in vec3 localPos)
{
#ifdef PROPELLER_WASH

    washTexCoords = vec3(0.0, 0.0, 0.0);
    activeWashWidth = -1.0;

    for (int i = 0; i < trit_numPropWashes; i++) {

        //if (trit_washes[i].distFromSource == 0) continue;

        vec3 C = trit_washes[i].deltaPos;
        vec3 A = localPos - trit_washes[i].propPosition;
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
                if(nd >= 0 && nd <= 1.0) {
                    if(dot(up, aCrossB) >= 0) {
                        t = (1.0-nd*0.5)-0.5;
                    } else {
                        t = 0.5+(nd*0.5);
                    }
                }

                nd = t;

                washTexCoords.x = nd;
                // The t0 parameter from our initial distance test to the line segment makes
                // for a handy t texture coordinate

                //scale texture by 4 to reduce tiling
                washTexCoords.y =  (trit_washes[i].washLength - distFromSource) / (4.0*trit_washes[i].washWidth);

                // We stuff the blend factor into the r coordinate.

                //float blend = max(0.0, 1.0 - distFromSource / (trit_washLength));
                float blend = mix(trit_washes[i].alphaEnd, trit_washes[i].alphaStart, t0);

                if(t < 0.1) {
                    blend *= max(0, 10.0*t);
                } else if(t > 0.9) {
                    blend *= max(0, (1.0-t)*10.0);
                }

                washTexCoords.z = clamp(blend, 0.0, 1.0);
                activeWashWidth = trit_washes[i].washWidth;
            }
        }
    }
#endif
}

#ifdef DOUBLE_PRECISION
// Intersect a ray of origin P0 and direction v against a unit sphere centered at the origin
bool raySphereIntersect(in dvec3 p0, in dvec3 v, out dvec3 intersection)
{
    double twop0v = 2.0 * dot(p0, v);
    double p02 = dot(p0, p0);
    double v2 = dot(v, v);

    double disc = twop0v * twop0v - (4.0 * v2)*(p02 - 1.0);
    if (disc > 0.0) {
        double discSqrt = sqrt(disc);
        double den = 2.0 * v2;
        double t = (-twop0v - discSqrt) / den;
        if (t < 0.0) {
            t = (-twop0v + discSqrt) / den;
        }
        intersection = p0 + t * v;
        return true;
    } else {
        intersection = dvec3(0.0, 0.0, 0.0);
        return false;
    }
}

// Intersect a ray against an ellipsoid centered at the origin
bool rayEllipsoidIntersect(in dvec3 R0, in dvec3 Rd, out dvec3 intersection)
{
    // Distort the ray so it aims toward a unit sphere, do a sphere intersection
    // and scale it back to the ellpsoid's space.

    dvec3 scaledR0 = R0 * dvec3(trit_oneOverRadii);
    dvec3 scaledRd = Rd * dvec3(trit_oneOverRadii);

    dvec3 sphereIntersection;
    if (raySphereIntersect(scaledR0, scaledRd, sphereIntersection)) {
        intersection = dvec3(sphereIntersection) * dvec3(trit_radii);
        return true;
    } else {
#ifdef SMOOTH_HORIZON
        // Project Vector A onto B
        // Vector A - Vector from eye to center of the earth.
        // Vector B - Direction vector from gridpoint near to gridpoint far frustum.
        // Vector C - Is the result of Projecting vector A onto B.
        vec3 a = vec3(-trit_cameraPos);
        vec3 b = normalize(vec3(R0+Rd) - vec3(trit_cameraPos));
        vec3 c = dot(a,b) * b;

        // Get the direction vector from the center of the earth to the closest point to
        // the horizon for vector b.
        // Scale by the earth radius to get the point on the edge of the horizon.
        intersection = trit_radii * normalize(c + vec3(trit_cameraPos));

        // Optimize by hiding vertices that have a vector b further then 6 degrees from the
        // horizon edge. Below calculation is to optimize atan(length(d)) > 6 degrees.
        const float angle  = pow(tan(6.0 * (PI / 180.0)), 2.0);
        vec3  d      = vec3(intersection) - (c + vec3(trit_cameraPos));
        float d_dist = dot(d,d) / dot(c,c);

        // Cull the vertex if angle to projected vertex is greater than 90 degrees
        const float max_proj_angle = 0.0;
        intersection = (dot(normalize(a),b) < max_proj_angle || d_dist > angle) ?
                       trit_cameraPos : intersection;
        return true;
#else
        intersection = dvec3(0.0, 0.0, 0.0);
        return false;
#endif
    }
}
#else
// Intersect a ray of origin P0 and direction v against a unit sphere centered at the origin
bool raySphereIntersect(in vec3 p0, in vec3 v, out vec3 intersection)
{
    float twop0v = 2.0 * dot(p0, v);
    float p02 = dot(p0, p0);
    float v2 = dot(v, v);

    float disc = twop0v * twop0v - (4.0 * v2)*(p02 - 1.0);
    if (disc > 0.0) {
        float discSqrt = sqrt(disc);
        float den = 2.0 * v2;
        float t = (-twop0v - discSqrt) / den;
        if (t < 0.0) {
            t = (-twop0v + discSqrt) / den;
        }
        intersection = p0 + t * v;
        return true;
    } else {
        intersection = vec3(0.0, 0.0, 0.0);
        return false;
    }
}

// Intersect a ray against an ellipsoid centered at the origin
bool rayEllipsoidIntersect(in vec3 R0, in vec3 Rd, out vec3 intersection)
{
    // Distort the ray so it aims toward a unit sphere, do a sphere intersection
    // and scale it back to the ellpsoid's space.

    vec3 scaledR0 = R0 * trit_oneOverRadii;
    vec3 scaledRd = Rd * trit_oneOverRadii;

    vec3 sphereIntersection;
    if (raySphereIntersect(scaledR0, scaledRd, sphereIntersection)) {
        intersection = sphereIntersection * trit_radii;
        return true;
    } else {
#ifdef SMOOTH_HORIZON
        // Project Vector A onto B
        // Vector A - Vector from eye to center of the earth.
        // Vector B - Direction vector from gridpoint near to gridpoint far frustum.
        // Vector C - Is the result of Projecting vector A onto B.
        vec3 a = -trit_cameraPos;
        vec3 b = normalize((R0+Rd) - trit_cameraPos);
        vec3 c = dot(a,b) * b;

        // Get the direction vector from the center of the earth to the closest point to
        // the horizon for vector b.
        // Scale by the earth radius to get the point on the edge of the horizon.
        intersection = trit_radii * normalize(c + trit_cameraPos);

        // Optimize by hiding vertices that have a vector b further then 6 degrees from the
        // horizon edge.
        const float angle = pow(tan(6.0f * (PI / 180.0f)), 2.0);

        vec3  d      = intersection - (c + trit_cameraPos);
        float d_dist = dot(d,d) / dot(c,c);

        // Cull the vertex if angle to projected vertex is greater than 90 degrees
        const float max_proj_angle = 0.0;
        intersection = (dot(normalize(a),b) < max_proj_angle || d_dist > angle) ?
                       trit_cameraPos : intersection;
        return true;
#else
        intersection = vec3(0.0);
        return false;
#endif

    }
}
#endif

// Alternate, faster method - but it can't handle viewpoints inside the ellipsoid.
// If you don't need underwater views, this may be better for you.
bool rayEllipsoidIntersectFast(in vec3 R0, in vec3 Rd, out vec3 intersection)
{
    vec3 q = R0 * trit_oneOverRadii;
    vec3 bUnit = normalize(Rd * trit_oneOverRadii);
    float wMagnitudeSquared = dot(q, q) - 1.0;

    float t = -dot(bUnit, q);
    float tSquared = t * t;

    if ((t >= 0.0) && (tSquared >= wMagnitudeSquared)) {
        float temp = t - sqrt(tSquared - wMagnitudeSquared);
        vec3 r = (q + temp * bUnit);
        intersection = r * trit_radii;

        return true;
    } else {
        intersection = vec3(0.0, 0.0, 0.0);
        return false;
    }
}

#ifdef DOUBLE_PRECISION
bool rayPlaneIntersect(in dvec4 p0, in dvec4 p1, out dvec4 intersection)
{
    double offset = 0;
    dvec4 p = dvec4(trit_plane);
    if (trit_plane.w < PRECISION_GUARD && trit_plane.w >= 0) {
        p.w = PRECISION_GUARD;
        offset = PRECISION_GUARD - trit_plane.w;
    } else if (trit_plane.w < 0 && trit_plane.w >= -PRECISION_GUARD) {
        p.w = -PRECISION_GUARD;
        offset = -PRECISION_GUARD - trit_plane.w;
    }

    // Intersect with the sea level
    dvec4 dp = p1 - p0;
    double t = -dot(p0, p) / dot( dp, p);
    if (t > 0.0 && t < 1.0) {
        intersection = dp * t + p0;
        intersection /= intersection.w;
        dvec3 up = dvec3(normalize(trit_cameraPos));
        intersection.xyz += up * offset;
        return true;
    } else {
        intersection = dvec4(0.0, 0.0, 0.0, 0.0);
        return false;
    }
}
#else
bool rayPlaneIntersect(in vec4 p0, in vec4 p1, out vec4 intersection)
{
    float offset = 0;
    vec4 p = trit_plane;
    if (trit_plane.w < PRECISION_GUARD && trit_plane.w >= 0.0) {
        p.w = PRECISION_GUARD;
        offset = PRECISION_GUARD - trit_plane.w;
    } else if (trit_plane.w < 0 && trit_plane.w >= -PRECISION_GUARD) {
        p.w = -PRECISION_GUARD;
        offset = -PRECISION_GUARD - trit_plane.w;
    }

    // Intersect with the sea level
    vec4 dp = p1 - p0;
    float t = -dot(p0, p) / dot( dp, p);
    if (t > 0.0 && t < 1.0) {
        intersection = dp * t + p0;
        intersection /= intersection.w;
        vec3 up = vec3(normalize(trit_cameraPos));
        intersection.xyz += up * offset;
        return true;
    } else {
        intersection = vec4(0.0, 0.0, 0.0, 0.0);
        return false;
    }
}
#endif

bool projectToSea(in vec4 v, out vec4 worldPos, out vec4 localPos)
{
    // Get the line this screen position projects to
    vec4 p0 = v;
    p0.z = trit_zNearFar.x;
    vec4 p1 = v;
    p1.z = trit_zNearFar.y;

    // Transform into world coords
    p0 = trit_invModelviewProj * p0;
    p1 = trit_invModelviewProj * p1;

    if (trit_plane.w < trit_planarHeight) {
        // Intersect with the sea level
#ifdef DOUBLE_PRECISION
        dvec4 intersect = dvec4(localPos);
        if (rayPlaneIntersect(dvec4(p0), dvec4(p1), intersect)) {
            intersect.xyz = user_horizon(intersect.xyz + dvec3(trit_cameraPos));

            localPos = vec4(intersect.xyz - dvec3(trit_cameraPos), 1.0);
            worldPos = vec4(intersect.xyz, 1.0);

            // Check if intersection point should be culled out.
            if (dot(intersect.xyz, intersect.xyz) < EPSILON)
                return true;

            // Account for error from plane approximation
            double dist = length(dvec3(localPos.xyz) + (dvec3(trit_plane.xyz) * double(trit_plane.w)));
            dvec2 v = dvec2(double(trit_radii.x), dist);
            double error = double(trit_planarAdjust) + (length(v) - double(trit_radii.x));
            dvec3 errorv = dvec3(trit_plane.xyz) * error;

            intersect.xyz -= dvec3(errorv);

            worldPos.xyz = vec3(intersect.xyz);
            localPos.xyz = vec3(intersect.xyz - dvec3(trit_cameraPos));
            return true;
        } else {
            localPos = vec4(0.0);
            worldPos = vec4(0.0);
            return false;
        }
#else //not double precision
        // Intersect with the sea level
        vec4  intersect = vec4(localPos);
        if (rayPlaneIntersect(p0, p1, intersect)) {
            intersect.xyz = user_horizon(intersect.xyz + trit_cameraPos);

            localPos = vec4(intersect.xyz - trit_cameraPos, 1.0);
            worldPos = vec4(intersect.xyz, 1.0);

            // Check if intersection point should be culled out.
            if (dot(intersect.xyz, intersect.xyz) < EPSILON)
                return true;

            // Account for error from plane approximation
            float dist = length(localPos.xyz + (trit_plane.xyz * trit_plane.w));
            vec2 v = vec2(trit_radii.x, dist);
            float error = trit_planarAdjust + (length(v) - trit_radii.x);
            vec3 errorv = trit_plane.xyz * error;

            intersect.xyz -= errorv;

            worldPos.xyz = vec3(intersect.xyz);
            localPos.xyz = worldPos.xyz - vec3(trit_cameraPos);
            return true;
        } else {
            localPos = vec4(0.0);
            worldPos = vec4(0.0);
            return false;
        }
#endif // double precision
    } else {
#ifdef DOUBLE_PRECISION
        dvec3 intersect = dvec3(0.0);
        dvec3 p03 = p0.xyz / p0.w;
        dvec3 p13 = p1.xyz / p1.w;
        if (rayEllipsoidIntersect(p03 + dvec3(trit_cameraPos), p13 - p03, intersect)) {
            intersect = user_horizon(intersect);
            localPos = vec4(intersect - dvec3(trit_cameraPos), 1.0);
            worldPos = vec4(intersect, 1.0);
#ifdef SMOOTH_HORIZON
            return (dot(localPos.xyz, localPos.xyz) > EPSILON);
#else
            return true;
#endif
        } else {
            localPos = vec4(0.0);
            worldPos = vec4(0.0);
            return false;
        }
#else
        vec3 intersect = vec3(0.0);
        vec3 p03 = p0.xyz / p0.w;
        vec3 p13 = p1.xyz / p1.w;
        if (rayEllipsoidIntersect(p03 + vec3(trit_cameraPos), p13 - p03, intersect)) {
            intersect = user_horizon(intersect);
            localPos = vec4(intersect - trit_cameraPos, 1.0);
            worldPos = vec4(intersect, 1.0);
#ifdef SMOOTH_HORIZON
            return (dot(localPos.xyz, localPos.xyz) > EPSILON);
#else
            return true;
#endif

        } else {
            localPos = vec4(0.0);
            worldPos = vec4(0.0);
            return false;
        }
#endif
    }

}

vec3 computeArcLengths(in vec3 localPos, in vec3 northDir, in vec3 eastDir)
{
    vec3 pt = trit_referenceLocation + localPos;
    return vec3(dot(pt, eastDir), dot(pt, northDir), 0.0);
}

void main()
{
    wakeNormalsAndFoam = vec4( 0.0, 0.0, 1.0, 0.0 );
    transparency = 0.0;

#ifdef LEEWARD_DAMPENING
    leewardDampening = 1.0;
#endif

#ifdef PROPELLER_WASH
    washTexCoords = vec3(0.0, 0.0, 0.0);
#endif
    // To avoid precision issues, the translation component of the modelview matrix
    // is zeroed out, and the camera position passed in via cameraPos
    vec4 worldPos = vec4(0.0);

#ifdef OPENGL32
    vec4 gridPos = trit_gridScale * vec4(vertex.x, vertex.y, 0.0, 1.0);
#else
    vec4 gridPos = trit_gridScale * vec4(gl_Vertex.x, gl_Vertex.y, 0.0, 1.0);

#endif

    vec4 localPos = vec4(0.0);
    if (projectToSea(gridPos, worldPos, localPos)) {
        // Here, worldPos is relative to the center of the Earth, since
        // projectToSea added the camera position back in after transforming

        float fogExponent = length(localPos.xyz) * trit_fogDensity;
        fogFactor = clamp(exp(-(fogExponent * fogExponent)), 0.0, 1.0);

        up = normalize(worldPos.xyz);

        // Transform position on the ellipsoid into a planar reference,
        // x east, y north, z up
        vec3 planar = computeArcLengths(localPos.xyz, trit_north, trit_east);

        // Compute water depth and transparency
        computeTransparency(worldPos.xyz, localPos.xyz);

        float fade = 1.0 - smoothstep(0.0, 1.0, length(localPos.xyz) * trit_invDampingDistance);

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

        // Compute displacement
        texCoords = planar.xy / trit_textureSize;
#ifdef OPENGL32
        vec3 disp = texture(trit_displacementMap, texCoords).xyz;
#else
        vec3 disp = texture2D(trit_displacementMap, texCoords).xyz;
#endif
        // Hide the backs of waves if we're transparent
        float opacity = 1.0 - transparency;
        disp.z = mix(0.0, disp.z, pow(opacity, 6.0));

        disp = disp * fade;

        localPos.xyz += disp.x * trit_east + disp.y * trit_north;

        foamTexCoords = (planar.xy + disp.xy) / trit_foamScale;
        noiseTexCoords = texCoords * 0.03;

        if (trit_doWakes) {
            vec2 slope;
            float foam;

            applyCircularWaves(planar, localPos.xyz, fade, slope, foam);
            applyKelvinWakes(planar, localPos.xyz, fade, slope, foam);
#ifdef PER_FRAGMENT_PROP_WASH
            propWashCoords = planar;
            localPropWashCoords = localPos.xyz;
#else
            applyPropWash(planar, localPos.xyz);
#endif
            applyLeewardDampening(localPos.xyz);

            vec3 sx = vec3(1.0, 0.0, slope.x);
            vec3 sy = vec3(0.0, 1.0, slope.y);
            wakeNormalsAndFoam.xyz = normalize(cross(sx, sy));
            wakeNormalsAndFoam.w = min(1.0, foam);
        } else {
            fade = 0.0;
        }

#ifdef LEEWARD_DAMPENING
        disp.z *= leewardDampening;
#endif

        applyBreakingWaves(planar, fade, worldPos.xyz);

        // Transform back into geocentric coords
        localPos.xyz += (disp.z + planar.z) * up;

        V = localPos.xyz;

        // Project it back again, apply depth offset.
        vec4 v = trit_modelview * localPos;
        vVertex_Eye_Space = v;
        v.w -= trit_depthOffset;
        vec4 vVertInProjSpc = trit_projection * v;
        vVertex_Projection_Space = vVertInProjSpc;

        if (trit_bypassOverridePosition && trit_depthOnly) {
            gl_Position = vVertInProjSpc;
        } else {
            gl_Position = overridePosition(vVertInProjSpc);
        }

        user_intercept(vec3(trit_cameraPos) + localPos.xyz, localPos.xyz, vVertex_Eye_Space, vVertex_Projection_Space);
    } else {
        // No intersection, move the vert out of clip space
        vVertex_Eye_Space = vec4(0,0,0,1);
        vec4 vVertInProjSpc = vec4(gridPos.x, gridPos.y, 1000.0, 1.0);
        vVertex_Projection_Space = vVertInProjSpc;
        gl_Position = vVertInProjSpc;
        V = vec3(0.0);
        foamTexCoords = vec2(0.0);
        texCoords = vec2(0.0);
        noiseTexCoords = vec2(0.0);
        up = vec3(0.0);
        fogFactor = 1.0;
#ifdef PROPELLER_WASH
        washTexCoords = vec3(0.0);
#endif
    }
}