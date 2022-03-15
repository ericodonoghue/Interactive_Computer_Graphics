#version 330 core

in vec3 frag_norm;
in vec4 frag_pos;
in vec4 lightview_pos;

uniform sampler2DShadow shadow_map;

out vec4 color;

void main()
{
	vec3 light_dir = normalize(vec3(1,1,5));
	float alpha = 18.0;

	vec3 v = vec3(normalize(frag_pos)) * vec3(-1,-1,-1);
	vec3 h = normalize(light_dir + v);

	float cos_theta = dot(normalize(frag_norm), light_dir);
	float cos_phi = dot(normalize(frag_norm), h);

	vec3 I = vec3(1,1,1);
	vec3 K_s = vec3(1,1,1);
	vec3 K_d = vec3(0.8,0,0);

	vec4 c = vec4(I * ( (cos_theta * K_d) + (K_s * pow(cos_phi, alpha)) ),1);
	c = vec4(0.1,0.1,0.1,1);

	color = c * textureProj(shadow_map, lightview_pos);
}