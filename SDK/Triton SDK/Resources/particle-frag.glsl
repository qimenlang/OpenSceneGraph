

#ifdef OPENGL32
#define IN in
#else
#define IN varying
#endif

IN float elapsed;
IN vec2 texCoord;
#ifdef USE_INSTANCING_FOR_TRANSPARENCY
IN float transparency;
#endif
IN vec3 vEye;
IN vec3 vWorld;
IN float fogFactor;


void user_particle_color(in vec3 vEye, in vec3 vWorld, in vec4 texColor,
                         in vec4 lightColor, in float transparency, in float decay,
                         inout vec4 additiveColor);

void writeFragmentData(in vec4 finalColor, in vec4 Cdiffuse, in vec3 lightColor, in vec3 nNorm);

void main()
{
    float decay = clamp(exp(-2.0 * elapsed) * 2.0 * sin(3.0 * elapsed), 0.0, 1.0);

#ifdef POINT_SPRITES
    vec2 tex = gl_PointCoord;
#else
    vec2 tex = texCoord;
#endif

#ifdef OPENGL32
    vec4 texColor = texture(trit_particleTexture, tex);
#else
    vec4 texColor = texture2D(trit_particleTexture, tex);
#endif

#ifndef USE_INSTANCING_FOR_TRANSPARENCY
    float transparency = trit_transparency;
#endif
    vec4 color = texColor * mix(vec4(trit_fogColor, 1.0), trit_lightColor, fogFactor) * decay * transparency;

    user_particle_color(vEye, vWorld, texColor, trit_lightColor, transparency, decay, color);

    if (length(color.xyz) < 0.05) {
        discard;
    } else {
        writeFragmentData(color, vec4(texColor.xyz, decay * transparency), trit_lightColor.xyz, -trit_g);
    }
}
