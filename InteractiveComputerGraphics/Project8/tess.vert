#version 330 core

uniform mat4 mvp;
uniform int displacement;

layout(location=0) in vec3 pos;
layout(location=2) in vec2 tcx;

out vec3 v_pos;
out vec2 v_tcx;

void main()
{
	if (displacement == 1) { 
		v_pos = pos;
		v_tcx = vec2(tcx.x, 1.0 - tcx.y);
	}
	else {
		gl_Position = mvp * vec4(pos, 1); 
	}
}