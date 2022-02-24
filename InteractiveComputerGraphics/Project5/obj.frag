#version 330 core

uniform sampler2D tex;

in vec3 frag_norm;
in vec4 frag_pos;
in vec2 frag_tcx;

out vec4 color;

void main()
{
	vec3 light_dir = normalize(vec3(1,1,5));
	float alpha = 18.0;

	vec3 v = vec3(normalize(frag_pos)) * vec3(-1,-1,-1);
	vec3 h = normalize(light_dir + v);

	float cos_theta = dot(normalize(frag_norm), light_dir);
	float cos_phi = dot(normalize(frag_norm), h);

	vec3 tcx_kd = texture2D(tex, frag_tcx).rgb;

	vec3 I = vec3(1,1,1);
	vec3 K_s = vec3(1,1,1);
	vec3 K_d = vec3(0.8,0,0) * tcx_kd;

	color = vec4(I * ( (cos_theta * K_d) + (K_s * pow(cos_phi, alpha)) ),1);
}