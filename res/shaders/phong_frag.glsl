#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;

uniform sampler2D texture_diffuse;

uniform vec3 view_position;

layout(std140, binding = 1) uniform Light
{
    vec3 light_position;
    vec3 light_color;
    float light_ambient_intensity;
    float light_specular_intensity;
};

layout(std140, binding = 2) uniform Material
{
    vec3 material_ambient;
    vec3 material_diffuse;
    vec3 material_specular;
    float material_shininess;
};

vec3 calculate_ambient(vec3 light_color, float light_ambient_intensity)
{
    return light_ambient_intensity * material_ambient * light_color;
}

vec3 calculate_diffuse(vec3 light_color, vec3 light_position, vec3 frag_pos, vec3 normal)
{
    vec3 light_direction = normalize(light_position - frag_pos);
    float diff = max(dot(normalize(normal), light_direction), 0.0);
    return diff * material_diffuse * light_color;
}

vec3 calculate_specular(vec3 light_color, vec3 light_position, vec3 frag_pos, vec3 normal)
{
    vec3 view_direction = normalize(view_position - frag_pos);
    vec3 light_direction = normalize(light_position - frag_pos);
    vec3 reflect_direction = reflect(-light_direction, normalize(normal));
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), material_shininess);
    return light_specular_intensity * material_specular * spec * light_color;
}

void main()
{
    vec3 ambient = calculate_ambient(light_color, light_ambient_intensity);
    vec3 diffuse = calculate_diffuse(light_color, light_position, frag_pos_out, normal_out);
    vec3 specular = calculate_specular(light_color, light_position, frag_pos_out, normal_out);

    vec3 light_result = ambient + diffuse + specular;

    frag_color = vec4(light_result, 1.0) * texture(texture_diffuse, tex_coords_out);
}
