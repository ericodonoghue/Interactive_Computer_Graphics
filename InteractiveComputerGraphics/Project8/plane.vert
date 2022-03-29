#version 330 core

layout(location=0) in vec3 pos;
layout(location=1) in vec2 tcx;

uniform mat4 mvp;
uniform mat4 mv;

out vec4 v_pos;
out vec2 v_tcx;

void main()
{
	v_pos = mv * vec4(pos, 1);
	v_tcx = vec2(tcx.x, 1.0 - tcx.y);

	gl_Position = mvp * vec4(pos, 1);
}