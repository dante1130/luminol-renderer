#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_specular;
uniform sampler2D texture_emissive;

uniform vec3 view_position;

layout(std140, binding = 1) uniform Light
{
    vec3 light_position;
    vec3 light_ambient;
    vec3 light_diffuse;
    vec3 light_specular;
};

layout(std140, binding = 2) uniform Material
{
    vec3 material_ambient;
    vec3 material_diffuse;
    vec3 material_specular;
    float material_shininess;
};

vec3 calculate_ambient(vec3 light_ambient, vec3 material_ambient)
{
    return light_ambient * material_ambient;
}

vec3 calculate_diffuse(
    vec3 light_position,
    vec3 light_diffuse,
    vec3 material_diffuse,
    vec3 frag_pos,
    vec3 normal
)
{
    vec3 light_direction = normalize(light_position - frag_pos);
    float diff = max(dot(normalize(normal), light_direction), 0.0);
    return light_diffuse * (diff * material_diffuse);
}

vec3 calculate_specular(
    vec3 light_position,
    vec3 view_position,
    vec3 light_specular,
    vec3 material_specular,
    float material_shininess,
    vec3 frag_pos,
    vec3 normal
)
{
    vec3 view_direction = normalize(view_position - frag_pos);
    vec3 light_direction = normalize(light_position - frag_pos);
    vec3 reflect_direction = reflect(-light_direction, normalize(normal));
    float spec = pow(max(dot(view_direction, reflect_direction), 0.0), material_shininess);
    return light_specular * (spec * material_specular);
}

void main()
{
    vec3 ambient = calculate_ambient(light_ambient, material_ambient);

    vec3 diffuse = calculate_diffuse(
            light_position,
            light_diffuse,
            material_diffuse,
            frag_pos_out,
            normal_out
        );

    vec3 specular = calculate_specular(
            light_position,
            view_position,
            light_specular,
            material_specular,
            material_shininess,
            frag_pos_out,
            normal_out
        );

    vec3 light_result = ambient + diffuse + specular;

    frag_color = vec4(light_result, 1.0) * texture(texture_diffuse, tex_coords_out);
}
