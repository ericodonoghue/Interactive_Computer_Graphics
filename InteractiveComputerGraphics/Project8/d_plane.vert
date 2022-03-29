#version 330 core

layout(location=0) in vec3 pos;
layout(location=2) in vec2 tcx;

out vec3 v_pos;
out vec2 v_tcx;

void main()
{
	v_pos = pos;
	v_tcx = vec2(tcx.x, 1.0 - tcx.y);
}