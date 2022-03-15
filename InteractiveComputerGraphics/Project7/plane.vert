#version 330 core

layout(location=0) in vec3 pos;

uniform mat4 mvp;
uniform mat4 mv;
uniform mat4 m_shadow;

out vec3 frag_norm;
out vec4 frag_pos;
out vec4 lightview_pos;

void main()
{
	frag_pos = mv * vec4(pos, 1);
	frag_norm = (transpose(inverse(mv)) * vec4(0,1,0,0)).xyz;

	lightview_pos = m_shadow * vec4(pos, 1.0);

	gl_Position = mvp * vec4(pos, 1);
}