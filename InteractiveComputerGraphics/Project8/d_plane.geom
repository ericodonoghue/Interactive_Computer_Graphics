#version 330 core

in te_data {
    vec3 te_pos;
    vec2 te_tcx;
} v[];

out vec3 g_pos;
out vec2 g_tcx;

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

void main() 
{
    for(int i = 0; i < 3; i++) {
        g_pos = v[i].te_pos;
        g_tcx = v[i].te_tcx;    
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
  EndPrimitive();
}