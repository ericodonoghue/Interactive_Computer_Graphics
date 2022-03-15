#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;

uniform mat4 mvp;
uniform mat4 mv;

out vec3 frag_norm;
out vec4 frag_pos;

void main()
{
	frag_pos = mv * vec4(pos, 1);
	frag_norm = (transpose(inverse(mv)) * vec4(norm,0)).xyz;

	gl_Position = mvp * vec4(pos, 1);
}