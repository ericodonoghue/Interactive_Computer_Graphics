#version 330 core
layout(location=0) in vec3 pos;

uniform mat4 p_mvp;

out vec2 frag_tcx;

void main()
{
	frag_tcx = vec2(pos.xy + vec2(1,1))/2.0;

	gl_Position = p_mvp * vec4(pos, 1);
}
