#version 330 core
in vec3 pos;
in vec2 texpos;

out vec2 f_texpos;

void main()
{
    f_texpos = texpos;
    gl_Position = vec4(pos, 1.0);
}
