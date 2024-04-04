#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;
in vec3 tangent_out;

layout(std140, binding = 1) uniform Light
{
    vec3 light_position;
    vec3 light_ambient;
    vec3 light_diffuse;
    vec3 light_specular;
};

struct Material
{
    sampler2D texture_diffuse;
    sampler2D texture_specular;
    sampler2D texture_emissive;
    sampler2D texture_normal;
    float shininess;
};

uniform vec3 view_position;
uniform Material material;
uniform bool is_cell_shading_enabled;
uniform float cell_shading_levels;

vec3 calculate_normal(sampler2D material_texture_normal, vec3 normal, vec3 tangent, vec2 tex_coords)
{
    vec3 normal_map = texture(material_texture_normal, tex_coords).rgb;
    normal_map = normalize(normal_map * 2.0 - 1.0);

    vec3 n = normalize(normal);
    vec3 t = normalize(tangent);
    vec3 b = cross(n, t);

    mat3 tbn = mat3(t, b, n);
    return normalize(tbn * normal_map);
}

vec3 calculate_ambient(vec3 light_ambient, sampler2D material_texture_diffuse)
{
    return light_ambient * texture(material_texture_diffuse, tex_coords_out).rgb;
}

vec3 calculate_diffuse(
    vec3 light_position,
    vec3 light_diffuse,
    sampler2D material_texture_diffuse,
    vec3 frag_pos,
    vec3 normal
)
{
    vec3 light_direction = normalize(light_position - frag_pos);
    float diff = max(dot(normalize(normal), light_direction), 0.0);

    if (is_cell_shading_enabled)
    {
        diff = floor(diff * cell_shading_levels) / cell_shading_levels;
    }

    return light_diffuse * diff * texture(material_texture_diffuse, tex_coords_out).rgb;
}

vec3 calculate_specular(
    vec3 light_position,
    vec3 view_position,
    vec3 light_specular,
    sampler2D material_texture_specular,
    float material_shininess,
    vec3 frag_pos,
    vec3 normal
)
{
    vec3 view_direction = normalize(view_position - frag_pos);
    vec3 light_direction = normalize(light_position - frag_pos);
    vec3 half_direction = normalize(light_direction + view_direction);
    float spec = pow(max(dot(normalize(normal), half_direction), 0.0), material_shininess);

    if (is_cell_shading_enabled)
    {
        spec = floor(spec * cell_shading_levels) / cell_shading_levels;
    }

    return light_specular * spec * texture(material_texture_specular, tex_coords_out).rgb;
}

void main()
{
    vec3 normal = calculate_normal(material.texture_normal, normal_out, tangent_out, tex_coords_out);

    vec3 ambient = calculate_ambient(light_ambient, material.texture_diffuse);

    vec3 diffuse = calculate_diffuse(
            light_position,
            light_diffuse,
            material.texture_diffuse,
            frag_pos_out,
            normal
        );

    vec3 specular = calculate_specular(
            light_position,
            view_position,
            light_specular,
            material.texture_specular,
            material.shininess,
            frag_pos_out,
            normal
        );

    vec3 emission = texture(material.texture_emissive, tex_coords_out).rgb;

    vec3 light_result = ambient + diffuse + specular + emission;

    frag_color = vec4(light_result, 1.0);
}
