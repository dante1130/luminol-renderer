#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

const uint MAX_POINT_LIGHTS = 4;
const uint MAX_SPOT_LIGHTS = 4;

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

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
    float cut_off;
    float outer_cut_off;
};

layout(std140, binding = 1) uniform Light
{
    DirectionalLight directional_light;
    PointLight point_lights[MAX_POINT_LIGHTS];
    SpotLight spot_lights[MAX_SPOT_LIGHTS];
    uint point_lights_count;
    uint spot_lights_count;
};

struct Material
{
    sampler2D texture_emissive;
    float shininess;
};

struct GeometryBuffer
{
    sampler2D position;
    sampler2D normal;
    sampler2D albedo_spec;
};

uniform vec3 view_position;
uniform Material material;
uniform GeometryBuffer gbuffer;
uniform bool is_cell_shading_enabled;
uniform float cell_shading_levels;

vec3 calculate_ambient(
    vec3 light_ambient,
    vec3 albedo
)
{
    return light_ambient * albedo;
}

vec3 calculate_diffuse(
    vec3 light_direction,
    vec3 light_diffuse,
    vec3 frag_pos,
    vec3 albedo,
    vec3 normal
)
{
    float diff = max(dot(normal, light_direction), 0.0);

    if (is_cell_shading_enabled)
    {
        diff = floor(diff * cell_shading_levels) / cell_shading_levels;
    }

    return light_diffuse * diff * albedo;
}

vec3 calculate_specular(
    vec3 light_direction,
    vec3 view_position,
    vec3 light_specular,
    float material_specular,
    float material_shininess,
    vec3 frag_pos,
    vec3 normal
)
{
    const vec3 view_direction = normalize(view_position - frag_pos);
    const vec3 half_direction = normalize(light_direction + view_direction);
    float spec = pow(max(dot(normal, half_direction), 0.0), material_shininess);

    if (is_cell_shading_enabled)
    {
        spec = floor(spec * cell_shading_levels) / cell_shading_levels;
    }

    return light_specular * spec * material_specular;
}

vec3 calculate_directional_light(
    DirectionalLight light,
    vec3 normal,
    vec3 view_position,
    vec3 frag_pos,
    vec3 albedo,
    float material_specular
)
{
    const vec3 light_direction = normalize(-light.direction);

    const vec3 ambient = calculate_ambient(light.ambient, albedo);

    const vec3 diffuse = calculate_diffuse(
            light_direction,
            light.diffuse,
            frag_pos,
            albedo,
            normal
        );

    const vec3 specular = calculate_specular(
            light_direction,
            view_position,
            light.specular,
            material_specular,
            material.shininess,
            frag_pos,
            normal
        );

    return ambient + diffuse + specular;
}

vec3 calculate_point_light(
    PointLight light,
    vec3 normal,
    vec3 view_position,
    vec3 frag_pos,
    vec3 albedo,
    float material_specular
)
{
    const vec3 light_direction = normalize(light.position - frag_pos);

    const vec3 ambient = calculate_ambient(light.ambient, albedo);

    const vec3 diffuse = calculate_diffuse(
            light_direction,
            light.diffuse,
            frag_pos,
            albedo,
            normal
        );

    const vec3 specular = calculate_specular(
            light_direction,
            view_position,
            light.specular,
            material_specular,
            material.shininess,
            frag_pos,
            normal
        );

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return ambient + ((diffuse + specular) * attenuation);
}

vec3 calculate_spot_light(
    SpotLight light,
    vec3 normal,
    vec3 view_position,
    vec3 frag_pos,
    vec3 albedo,
    float material_specular
)
{
    const vec3 light_direction = normalize(light.position - frag_pos);

    const float theta = dot(light_direction, normalize(-light.direction));
    const float epsilon = light.cut_off - light.outer_cut_off;
    const float intensity = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);

    const vec3 ambient = calculate_ambient(light.ambient, albedo);

    if (intensity <= 0.0)
    {
        return ambient;
    }

    const vec3 diffuse = calculate_diffuse(
            light_direction,
            light.diffuse,
            frag_pos,
            albedo,
            normal
        );

    const vec3 specular = calculate_specular(
            light_direction,
            view_position,
            light.specular,
            material_specular,
            material.shininess,
            frag_pos,
            normal
        );

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return ambient + ((diffuse + specular) * attenuation) * intensity;
}

void main()
{
    const vec3 frag_pos = texture(gbuffer.position, tex_coords_out).rgb;
    const vec3 normal = texture(gbuffer.normal, tex_coords_out).rgb;
    const vec3 albedo = texture(gbuffer.albedo_spec, tex_coords_out).rgb;
    float specular = texture(gbuffer.albedo_spec, tex_coords_out).a;

    const vec3 emission = texture(material.texture_emissive, tex_coords_out).rgb;

    vec3 light_result = emission;

    light_result += calculate_directional_light(
            directional_light,
            normal,
            view_position,
            frag_pos,
            albedo,
            specular
        );

    for (uint i = 0; i < point_lights_count; i++)
    {
        light_result += calculate_point_light(
                point_lights[i],
                normal,
                view_position,
                frag_pos,
                albedo,
                specular
            );
    }

    for (uint i = 0; i < spot_lights_count; i++)
    {
        light_result += calculate_spot_light(
                spot_lights[i],
                normal,
                view_position,
                frag_pos,
                albedo,
                specular
            );
    }

    frag_color = vec4(light_result, 1.0);
}
