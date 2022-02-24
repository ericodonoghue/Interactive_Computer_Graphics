#version 330

layout(location = 0) in vec3 pos;

uniform mat4 mvp;

out vec3 direction;

void main() 
{
    direction = pos;

    gl_Position = mvp * vec4(pos, 1.0);
}