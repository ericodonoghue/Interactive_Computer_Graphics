#version 410 core

uniform int tess_level;

in vec3 v_pos[];
in vec2 v_tcx[];

layout(vertices=4) out;
out vec3 tc_pos[];
out vec2 tc_tcx[];

void main()
{
    if (gl_InvocationID == 0) {
        gl_TessLevelInner[0] = tess_level;
        gl_TessLevelInner[1] = tess_level;

        gl_TessLevelOuter[0] = tess_level;
        gl_TessLevelOuter[1] = tess_level;
        gl_TessLevelOuter[2] = tess_level;
        gl_TessLevelOuter[3] = tess_level; 
    }

    tc_pos[gl_InvocationID] = v_pos[gl_InvocationID];
    tc_tcx[gl_InvocationID] = v_tcx[gl_InvocationID];
}