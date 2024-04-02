#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

uniform sampler2D texture_diffuse;

layout(std140, binding = 1) uniform Light
{
    vec3 light_position;
    vec3 light_color;
    float light_ambient_intensity;
};

void main()
{
    vec3 ambient = light_ambient_intensity * light_color;

    vec3 light_result = ambient;

    frag_color = vec4(light_result, 1.0) * texture(texture_diffuse, tex_coords_out);
}
