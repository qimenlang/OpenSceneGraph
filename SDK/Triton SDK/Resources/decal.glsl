#ifdef OPENGL32
in vec4 position;
out float fogFactor;
#else
attribute vec4 position;
varying float fogFactor;
#endif


vec4 overridePosition(in vec4 position);

void main()
{
    vec4 clipPos = trit_mvProj * position;

    vec4 vEye = clipPos * trit_inverseProjection;
    float fogExponent = length(vEye.xyz) * trit_fogDensity;
    fogFactor = clamp(exp(-(fogExponent * fogExponent)), 0.0, 1.0);

    gl_Position = overridePosition(clipPos);
}
