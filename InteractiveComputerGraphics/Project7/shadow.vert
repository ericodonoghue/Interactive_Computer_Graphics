#version 330 core
layout(location=0) in vec3 pos;

uniform mat4 mlp;

void main()
{
	gl_Position = mlp * vec4(pos, 1);
}