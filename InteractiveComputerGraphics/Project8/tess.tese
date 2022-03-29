#version 410 core

uniform sampler2D tex;
uniform mat4 mvp;

layout(quads, equal_spacing, ccw) in;
in vec3 tc_pos[];
in vec2 tc_tcx[];

vec3 interpolate(vec3 v0, vec3 v1, vec3 v2, vec3 v3) 
{
    vec3 a = mix(v0, v1, gl_TessCoord.x);
    vec3 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

vec2 interpolate(vec2 v0, vec2 v1, vec2 v2, vec2 v3) 
{
    vec2 a = mix(v0, v1, gl_TessCoord.x);
    vec2 b = mix(v3, v2, gl_TessCoord.x);
    return mix(a, b, gl_TessCoord.y);
}

void main()
{
    vec3 pos = interpolate(tc_pos[0], tc_pos[1], tc_pos[2], tc_pos[3]);
    vec2 tcx = interpolate(tc_tcx[0], tc_tcx[1], tc_tcx[2], tc_tcx[3]);
    float displacment = texture(tex, tcx).x * 6;

    gl_Position = mvp * vec4(pos.x,pos.y, pos.z + displacment, 1.0);
}