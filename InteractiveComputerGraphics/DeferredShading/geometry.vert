#version 330 core
in vec3 pos;
in vec3 norm;
in vec3 texpos;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec3 f_pos;
out vec3 f_norm;
out vec2 f_texpos;

void main()
{
	vec4 world_pos = (model * vec4(pos, 1.0));
    f_pos = world_pos.xyz; 
    f_texpos = vec2(texpos.x, texpos.y);
    f_norm = (transpose(inverse(model)) * vec4(norm,0)).xyz;

    gl_Position = projection * view * world_pos;
}