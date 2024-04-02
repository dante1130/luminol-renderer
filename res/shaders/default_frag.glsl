#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;

uniform sampler2D texture_diffuse;

layout(std140, binding = 1) uniform Light
{
    vec3 light_position;
    vec3 light_color;
    float light_ambient_intensity;
};

vec3 calculate_ambient(vec3 light_color, float light_ambient_intensity)
{
    return light_ambient_intensity * light_color;
}

vec3 calculate_diffuse(vec3 light_color, vec3 light_position, vec3 frag_pos, vec3 normal)
{
    vec3 light_direction = normalize(light_position - frag_pos);
    float diff = max(dot(normalize(normal), light_direction), 0.0);
    return diff * light_color;
}

void main()
{
    vec3 ambient = calculate_ambient(light_color, light_ambient_intensity);
    vec3 diffuse = calculate_diffuse(light_color, light_position, frag_pos_out, normal_out);

    vec3 light_result = ambient + diffuse;

    frag_color = vec4(light_result, 1.0) * texture(texture_diffuse, tex_coords_out);
}
