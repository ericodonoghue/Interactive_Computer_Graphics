#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec3 norm;
layout(location=2) in vec2 tcx;

uniform mat4 mvp;
uniform mat4 mvn;

out vec3 frag_norm;
out vec4 frag_pos;
out vec2 frag_tcx;

void main()
{
	frag_pos = mvn * vec4(pos, 1);
	frag_norm = (transpose(inverse(mvn)) * vec4(norm,0)).xyz;
	frag_tcx = vec2(tcx.x, 1.0 - tcx.y);

	gl_Position = mvp * vec4(pos, 1);
}