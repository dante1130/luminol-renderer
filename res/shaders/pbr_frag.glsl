#version 460 core

out vec4 frag_color;

in vec2 tex_coords_out;

const uint MAX_POINT_LIGHTS = 256;
const uint MAX_SPOT_LIGHTS = 256;

struct DirectionalLight
{
    vec3 direction;
    vec3 color;
};

struct PointLight
{
    vec3 position;
    vec3 color;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float cut_off;
    float outer_cut_off;
};

layout(std140, binding = 1) uniform Light
{
    DirectionalLight directional_light;
    uint point_lights_count;
    uint spot_lights_count;
    PointLight point_lights[MAX_POINT_LIGHTS];
    SpotLight spot_lights[MAX_SPOT_LIGHTS];
};

struct GeometryBuffer
{
    sampler2D position_metallic;
    sampler2D normal_roughness;
    sampler2D emissive_ao;
    sampler2D albedo;
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

vec3 calculate_specular_brdf(
    vec3 fresnel,
    vec3 light_direction,
    vec3 half_direction,
    vec3 normal,
    vec3 view_direction,
    vec3 f0,
    float metallic,
    float roughness
)
{
    const float normal_distribution = distribution_ggx(normal, half_direction, roughness);
    const float geometry = geometry_smith(normal, view_direction, light_direction, roughness);

    const vec3 numerator = normal_distribution * geometry * fresnel;
    const float denominator = 4.0 * max(dot(normal, view_direction), 0.0) * max(dot(normal, light_direction), 0.0) + 0.0001;

    return numerator / denominator;
}

vec3 calculate_directional_light(
    DirectionalLight light,
    vec3 normal,
    vec3 view_direction,
    vec3 frag_pos,
    vec3 albedo,
    vec3 f0,
    float metallic,
    float roughness
)
{
    const vec3 radiance = light.color;

    const vec3 light_direction = normalize(-light.direction);
    const vec3 half_direction = normalize(light_direction + view_direction);

    const vec3 fresnel = fresnel_schlick(max(dot(half_direction, view_direction), 0.0), f0);

    const vec3 specular = calculate_specular_brdf(
            fresnel,
            light_direction,
            half_direction,
            normal,
            view_direction,
            f0,
            metallic,
            roughness
        );

    const vec3 k_s = fresnel;
    vec3 k_d = 1.0 - k_s;
    k_d *= 1.0 - metallic;

    const float n_dot_l = max(dot(normal, light_direction), 0.0);

    const vec3 Lo = (k_d * albedo / PI + specular) * radiance * n_dot_l;

    return Lo;
}

vec3 calculate_point_light(
    PointLight light,
    vec3 normal,
    vec3 view_direction,
    vec3 frag_pos,
    vec3 albedo,
    vec3 f0,
    float metallic,
    float roughness
)
{
    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0 / (distance * distance);

    const vec3 radiance = light.color * attenuation;

    const vec3 light_direction = normalize(light.position - frag_pos);
    const vec3 half_direction = normalize(light_direction + view_direction);

    const vec3 fresnel = fresnel_schlick(max(dot(half_direction, view_direction), 0.0), f0);

    const vec3 specular = calculate_specular_brdf(
            fresnel,
            light_direction,
            half_direction,
            normal,
            view_direction,
            f0,
            metallic,
            roughness
        );

    const vec3 k_s = fresnel;
    vec3 k_d = 1.0 - k_s;
    k_d *= 1.0 - metallic;

    const float n_dot_l = max(dot(normal, light_direction), 0.0);

    const vec3 Lo = (k_d * albedo / PI + specular) * radiance * n_dot_l;

    return Lo;
}

vec3 calculate_spot_light(
    SpotLight light,
    vec3 normal,
    vec3 view_direction,
    vec3 frag_pos,
    vec3 albedo,
    vec3 f0,
    float metallic,
    float roughness
)
{
    const vec3 light_direction = normalize(light.position - frag_pos);

    const float theta = dot(light_direction, normalize(-light.direction));
    const float epsilon = light.cut_off - light.outer_cut_off;
    const float intensity = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);

    if (intensity <= 0.0)
    {
        return vec3(0.0);
    }

    const float distance = length(light.position - frag_pos);
    const float attenuation = 1.0 / (distance * distance);

    const vec3 radiance = light.color * attenuation;

    const vec3 half_direction = normalize(light_direction + view_direction);

    const vec3 fresnel = fresnel_schlick(max(dot(half_direction, view_direction), 0.0), f0);

    const vec3 specular = calculate_specular_brdf(
            fresnel,
            light_direction,
            half_direction,
            normal,
            view_direction,
            f0,
            metallic,
            roughness
        );

    const vec3 k_s = fresnel;
    vec3 k_d = 1.0 - k_s;
    k_d *= 1.0 - metallic;

    const float n_dot_l = max(dot(normal, light_direction), 0.0);

    const vec3 Lo = (k_d * albedo / PI + specular) * radiance * n_dot_l * intensity;

    return Lo;
}

void main()
{
    const vec3 frag_pos = texture(gbuffer.position_metallic, tex_coords_out).rgb;
    const vec3 normal = texture(gbuffer.normal_roughness, tex_coords_out).rgb;
    const vec3 emission = texture(gbuffer.emissive_ao, tex_coords_out).rgb;
    const vec3 albedo = texture(gbuffer.albedo, tex_coords_out).rgb;
    const float metallic = texture(gbuffer.position_metallic, tex_coords_out).a;
    const float roughness = texture(gbuffer.normal_roughness, tex_coords_out).a;
    const float ao = texture(gbuffer.emissive_ao, tex_coords_out).a;

    const vec3 view_direction = normalize(view_position - frag_pos);

    vec3 f0 = vec3(0.04);
    f0 = mix(f0, albedo, metallic);

    vec3 Lo = vec3(0.0);

    Lo += calculate_directional_light(
            directional_light,
            normal,
            view_direction,
            frag_pos,
            albedo,
            f0,
            metallic,
            roughness
        );

    for (uint i = 0; i < point_lights_count; i++)
    {
        Lo += calculate_point_light(
                point_lights[i],
                normal,
                view_direction,
                frag_pos,
                albedo,
                f0,
                metallic,
                roughness
            );
    }

    for (uint i = 0; i < spot_lights_count; i++)
    {
        Lo += calculate_spot_light(
                spot_lights[i],
                normal,
                view_direction,
                frag_pos,
                albedo,
                f0,
                metallic,
                roughness
            );
    }

    const vec3 ambient = vec3(0.03) * albedo * ao;
    const vec3 color = ambient + Lo + emission;

    frag_color = vec4(color, 1.0);
}
