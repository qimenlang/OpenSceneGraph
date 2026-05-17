// user-vert-functions.glsl

float user_get_depth( in vec3 worldPos )
{
    return 1000.0;
}

#ifdef DOUBLE_PRECISION

dvec3 user_horizon(in dvec3 intersect)
{
    return intersect;
}

#else

vec3 user_horizon(in vec3 intersect)
{
    return intersect;
}

#endif

/* You may use this hook to set any varying parameters you need for the user-functions.glsl fragment program.
   provided are the ocean vertex in world, eye, and projected coordinates. */
void user_intercept(in vec3 worldPosition, in vec3 localPosition, in vec4 eyePosition, in vec4 projectedPosition)
{

}

// Provides a point to override the final value of gl_Position.
// Useful for implementing logarithmic depth buffers etc. If you are
// also using decal effects (such as rotor wash,) be sure to also
// implement the user_decal_depth function in user-functions.glsl
// if you are manipulating depth values.
vec4 overridePosition(in vec4 position)
{
    return position;
}
