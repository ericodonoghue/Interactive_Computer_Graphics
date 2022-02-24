#version 330

in vec3 direction;

uniform samplerCube cube;

out vec4 color;

void main(void) 
{
    color = texture(cube, direction);
}