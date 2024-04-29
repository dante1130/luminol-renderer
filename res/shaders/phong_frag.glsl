#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

const uint MAX_POINT_LIGHTS = 256;
const uint MAX_SPOT_LIGHTS = 256;

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

struct GeometryBuffer
{
    sampler2D position;
    sampler2D normal;
    sampler2D emissive;
    sampler2D albedo_spec;
};

uniform vec3 view_position;
uniform GeometryBuffer gbuffer;

const float PI = 3.14159265359;

float distribution_ggx(vec3 normal, vec3 half_direction, float roughness)
{
    const float a = roughness * roughness;
    const float numerator = a * a;

    const float n_dot_h = max(dot(normal, half_direction), 0.0);
    const float n_dot_h_2 = n_dot_h * n_dot_h;

    float denominator = (n_dot_h_2 * (numerator - 1.0) + 1.0);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float geometry_schlick_ggx(float n_dot_v, float roughness)
{
    const float r = (roughness + 1.0);
    const float k = (r * r) / 8.0;

    const float numerator = n_dot_v;
    const float denominator = n_dot_v * (1.0 - k) + k;

    return numerator / denominator;
}

float geometry_smith(vec3 normal, vec3 view_direction, vec3 light_direction, float roughness)
{
    const float n_dot_v = max(dot(normal, view_direction), 0.0);
    const float n_dot_l = max(dot(normal, light_direction), 0.0);

    const float ggx1 = geometry_schlick_ggx(n_dot_v, roughness);
    const float ggx2 = geometry_schlick_ggx(n_dot_l, roughness);

    return ggx1 * ggx2;
}

vec3 fresnel_schlick(float cos_theta, vec3 f0)
{
    return f0 + (1.0 - f0) * pow(clamp(1.0 - cos_theta, 0.0, 1.0), 5.0);
}

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

    return light_specular * spec * material_specular;
}

vec3 calculate_directional_light(
    DirectionalLight light,
    vec3 normal,
    vec3 view_position,
    vec3 frag_pos,
    vec3 albedo,
    float material_specular,
    float material_shininess
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
            material_shininess,
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
    float material_specular,
    float material_shininess
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
            material_shininess,
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
    float material_specular,
    float material_shininess
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
            material_shininess,
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
    const vec3 emission = texture(gbuffer.emissive, tex_coords_out).rgb;
    const vec3 albedo = texture(gbuffer.albedo_spec, tex_coords_out).rgb;
    const float specular = texture(gbuffer.albedo_spec, tex_coords_out).a;

    const float shininess = 256.0f;

    vec3 light_result = emission;

    light_result += calculate_directional_light(
            directional_light,
            normal,
            view_position,
            frag_pos,
            albedo,
            specular,
            shininess
        );

    for (uint i = 0; i < point_lights_count; i++)
    {
        light_result += calculate_point_light(
                point_lights[i],
                normal,
                view_position,
                frag_pos,
                albedo,
                specular,
                shininess
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
                specular,
                shininess
            );
    }

    frag_color = vec4(light_result, 1.0);
}
