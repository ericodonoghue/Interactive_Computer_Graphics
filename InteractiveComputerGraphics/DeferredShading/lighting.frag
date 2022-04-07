#version 330 core

in vec2 f_texpos;

uniform sampler2D g_pos;
uniform sampler2D g_normal;
uniform sampler2D g_diffuse_spec;

struct Light {
    vec3 position;
    vec3 color;
    float linear;
    float quadratic;
    float radius;
};
const int MAX_LIGHTS = 128;
uniform int num_lights;
uniform Light lights[MAX_LIGHTS];
uniform vec3 camera_pos;

out vec4 color;

void main()
{             
    // retrieve data from gbuffer
    vec3 f_pos = texture(g_pos, f_texpos).rgb;
    vec3 f_norm = texture(g_normal, f_texpos).rgb;
    vec3 f_color = texture(g_diffuse_spec, f_texpos).rgb;
    float f_shine = texture(g_diffuse_spec, f_texpos).a;
    
    // then calculate lighting as usual
    vec3 lighting  = f_color * 0.1; // hard-coded ambient component
    vec3 view_dir  = normalize(camera_pos - f_pos);
    for(int i = 0; i < num_lights; ++i)
    {
        float distance = length(lights[i].position - f_pos);
        if (distance < lights[i].radius) 
        {
            // diffuse
            vec3 light_dir = normalize(lights[i].position - f_pos);
            vec3 diffuse = max(dot(f_norm, light_dir), 0.0) * f_color * lights[i].color;
            // specular
            vec3 halfway_dir = normalize(light_dir + view_dir);  
            float spec = pow(max(dot(f_norm, halfway_dir), 0.0), 16.0);
            vec3 specular = lights[i].color * spec * f_shine;
            // attenuation
            float attenuation = 1.0 / (1.0 + lights[i].linear * distance + lights[i].quadratic * distance * distance);
            diffuse *= attenuation;
            specular *= attenuation;
            lighting += diffuse + specular;
        }
    }
    color = vec4(lighting, 1.0);
}