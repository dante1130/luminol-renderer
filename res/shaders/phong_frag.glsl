#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;
in vec3 frag_pos_out;
in vec3 normal_out;
in vec3 tangent_out;

const uint MAX_POINT_LIGHTS = 4;

struct DirectionalLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

layout(std140, binding = 1) uniform Light
{
    DirectionalLight directional_light;
    PointLight point_lights[MAX_POINT_LIGHTS];
    uint point_lights_count;
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

    const vec3 n = normalize(normal);

    vec3 t = normalize(tangent);
    t = normalize(t - dot(t, n) * n);

    const vec3 b = cross(n, t);

    const mat3 tbn = mat3(t, b, n);

    return normalize(tbn * normal_map);
}

vec3 calculate_ambient(vec3 light_ambient, sampler2D material_texture_diffuse)
{
    return light_ambient * texture(material_texture_diffuse, tex_coords_out).rgb;
}

vec3 calculate_diffuse(
    vec3 light_direction,
    vec3 light_diffuse,
    sampler2D material_texture_diffuse,
    vec3 frag_pos,
    vec3 normal
)
{
    float diff = max(dot(normalize(normal), light_direction), 0.0);

    if (is_cell_shading_enabled)
    {
        diff = floor(diff * cell_shading_levels) / cell_shading_levels;
    }

    return light_diffuse * diff * texture(material_texture_diffuse, tex_coords_out).rgb;
}

vec3 calculate_specular(
    vec3 light_direction,
    vec3 view_position,
    vec3 light_specular,
    sampler2D material_texture_specular,
    float material_shininess,
    vec3 frag_pos,
    vec3 normal
)
{
    const vec3 view_direction = normalize(view_position - frag_pos);
    const vec3 half_direction = normalize(light_direction + view_direction);
    float spec = pow(max(dot(normalize(normal), half_direction), 0.0), material_shininess);

    if (is_cell_shading_enabled)
    {
        spec = floor(spec * cell_shading_levels) / cell_shading_levels;
    }

    return light_specular * spec * texture(material_texture_specular, tex_coords_out).rgb;
}

vec3 calculate_directional_light(DirectionalLight light, vec3 normal, vec3 view_position, vec3 frag_pos)
{
    const vec3 light_direction = normalize(-light.direction);

    const vec3 ambient = calculate_ambient(light.ambient, material.texture_diffuse);

    const vec3 diffuse = calculate_diffuse(
            light_direction,
            light.diffuse,
            material.texture_diffuse,
            frag_pos,
            normal
        );

    const vec3 specular = calculate_specular(
            light_direction,
            view_position,
            light.specular,
            material.texture_specular,
            material.shininess,
            frag_pos,
            normal
        );

    return ambient + diffuse + specular;
}

vec3 calculate_point_light(PointLight light, vec3 normal, vec3 view_position, vec3 frag_pos)
{
    const vec3 light_direction = normalize(light.position - frag_pos);

    const vec3 ambient = calculate_ambient(light.ambient, material.texture_diffuse);

    const vec3 diffuse = calculate_diffuse(
            light_direction,
            light.diffuse,
            material.texture_diffuse,
            frag_pos,
            normal
        );

    const vec3 specular = calculate_specular(
            light_direction,
            view_position,
            light.specular,
            material.texture_specular,
            material.shininess,
            frag_pos,
            normal
        );

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    const vec3 normal = calculate_normal(material.texture_normal, normal_out, tangent_out, tex_coords_out);

    const vec3 emission = texture(material.texture_emissive, tex_coords_out).rgb;

    vec3 light_result = emission;

    light_result += calculate_directional_light(directional_light, normal, view_position, frag_pos_out);

    for (uint i = 0; i < point_lights_count; i++)
    {
        light_result += calculate_point_light(point_lights[i], normal, view_position, frag_pos_out);
    }

    frag_color = vec4(light_result, 1.0);
}
