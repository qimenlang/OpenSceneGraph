#ifdef OPENGL32
#define IN in
#define OUT out
#else
#define IN attribute
#define OUT varying
#endif

IN vec3 initialPosition;
IN vec3 initialVelocity;
IN float size;
IN float startTime;
IN float offsetX;
IN float offsetY;
IN float texCoordX;
IN float texCoordY;
#ifdef USE_INSTANCING_FOR_TRANSPARENCY
IN float in_transparency;
#endif
OUT float elapsed;
OUT vec2 texCoord;
OUT float transparency;
OUT vec3 vEye;
OUT vec3 vWorld;
OUT float fogFactor;

vec4 overridePosition(in vec4 position);
float user_get_depth( in vec3 worldPos );

float getDepthFromHeightmap(in vec3 worldPos)
{
    float depth = 1000.0;

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

    return depth;
}

void main()
{
    elapsed = trit_time - startTime;

    // p(t) = p0 + v0t + 0.5gt^2
    vec3 relPos = 0.5 * trit_g * elapsed * elapsed + initialVelocity * elapsed + initialPosition;
    vec2 offset = vec2(offsetX, offsetY);

    vec3 worldPos = relPos + trit_cameraPos - trit_refOffset;

    float depthFade = 1.0;

    if ( trit_hasUserHeightMap || trit_hasHeightMap ) {
        float depth;
        if ( trit_hasUserHeightMap )
            depth = user_get_depth(worldPos);
        else
            depth = getDepthFromHeightmap(worldPos);

        if (depth < (size * trit_invSizeFactor)) {
#ifdef POINT_SPRITES
            gl_PointSize = 0.0;
#endif
            gl_Position = vec4(0.0,0.0,100.0,0.0);
            return;
        }
    }

    vec4 wPos = vec4(relPos - trit_refOffset, 1.0);

    vec4 eyeSpacePos = trit_modelview * wPos;

#ifdef POINT_SPRITES
    float dist = length(eyeSpacePos.xyz);
    gl_PointSize = max(1.0, size / dist);
#endif

    eyeSpacePos.xy += offset;

    wPos = eyeSpacePos * trit_modelview;

    texCoord = vec2(texCoordX, texCoordY);

#ifdef USE_INSTANCING_FOR_TRANSPARENCY
    transparency = in_transparency;
#endif

    gl_Position = overridePosition(trit_mvProj * wPos);

    vEye = eyeSpacePos.xyz / eyeSpacePos.w;

    float fogExponent = length(vEye.xyz) * trit_fogDensity;
    fogFactor = clamp(exp(-(fogExponent * fogExponent)), 0.0, 1.0);

    vWorld = worldPos;
}
