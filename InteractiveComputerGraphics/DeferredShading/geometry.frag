#version 330 core
in vec3 f_pos;
in vec3 f_norm;
in vec2 f_texpos;

uniform sampler2D diffuse;
uniform sampler2D specular;

layout (location = 0) out vec3 g_position;
layout (location = 1) out vec3 g_normal;
layout (location = 2) out vec4 g_diffuse_spec;

void main()
{
    g_position = f_pos;
    g_normal = normalize(f_norm);
    g_diffuse_spec.rgb = texture(diffuse, f_texpos).rgb;
    g_diffuse_spec.a = texture(specular, f_texpos).r; 
}