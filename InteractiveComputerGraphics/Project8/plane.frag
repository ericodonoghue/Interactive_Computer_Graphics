#version 330 core

uniform sampler2D norm_tex;

in vec4 v_pos;
in vec2 v_tcx;

out vec4 color;

void main()
{
	vec3 light_dir = normalize(vec3(1,1,5));
	vec3 n = texture2D(norm_tex, v_tcx).rgb;
	float alpha = 18.0;

	vec3 v = vec3(normalize(v_pos)) * vec3(-1,-1,-1);
	vec3 h = normalize(light_dir + v);

	float cos_theta = dot(normalize(n), light_dir);
	float cos_phi = dot(normalize(n), h);

	vec3 I = vec3(1,1,1);
	vec3 K_s = vec3(1,1,1);
	vec3 K_d = vec3(0.8,0,0);

	color = vec4(I * ( (cos_theta * K_d) + (K_s * pow(cos_phi, alpha)) ), 1);

}