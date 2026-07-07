Texture2D albedo_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);
Texture2D metallic_texture : register(t2, space2);
Texture2D roughness_texture : register(t3, space2);
Texture2D ao_texture : register(t4, space2);

SamplerState albedo_sampler : register(s0, space2);
SamplerState normal_sampler : register(s1, space2);
SamplerState metallic_sampler : register(s2, space2);
SamplerState roughness_sampler : register(s3, space2);
SamplerState ao_sampler : register(s4, space2);

cbuffer LightBuffer : register(b0, space3) {
    float4 light_direction;
    float4 light_color;
    float4 view_position;
};

struct PSInput {
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    float3 world_tangent : TEXCOORD3;
};

static const float PI = 3.14159265359f;

float distribution_ggx(float3 normal, float3 half_direction, float roughness) {
    const float a = roughness * roughness;
    const float numerator = a * a;

    const float n_dot_h = max(dot(normal, half_direction), 0.0f);
    const float n_dot_h_2 = n_dot_h * n_dot_h;

    float denominator = (n_dot_h_2 * (numerator - 1.0f) + 1.0f);
    denominator = PI * denominator * denominator;

    return numerator / denominator;
}

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    const float r = roughness + 1.0f;
    const float k = (r * r) / 8.0f;

    const float numerator = n_dot_v;
    const float denominator = n_dot_v * (1.0f - k) + k;

    return numerator / denominator;
}

float geometry_smith(
    float3 normal, float3 view_direction, float3 light_direction, float roughness
) {
    const float n_dot_v = max(dot(normal, view_direction), 0.0f);
    const float n_dot_l = max(dot(normal, light_direction), 0.0f);

    const float ggx1 = geometry_schlick_ggx(n_dot_v, roughness);
    const float ggx2 = geometry_schlick_ggx(n_dot_l, roughness);

    return ggx1 * ggx2;
}

float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float3 calculate_specular_brdf(
    float3 fresnel,
    float3 light_direction,
    float3 half_direction,
    float3 normal,
    float3 view_direction,
    float roughness
) {
    const float normal_distribution = distribution_ggx(normal, half_direction, roughness);
    const float geometry = geometry_smith(normal, view_direction, light_direction, roughness);

    const float3 numerator = normal_distribution * geometry * fresnel;
    const float denominator = 4.0f * max(dot(normal, view_direction), 0.0f) *
        max(dot(normal, light_direction), 0.0f) + 0.0001f;

    return numerator / denominator;
}

float3 calculate_directional_light(
    float3 normal,
    float3 view_direction,
    float3 albedo,
    float3 f0,
    float metallic,
    float roughness
) {
    const float3 radiance = light_color.rgb;

    const float3 light_dir = normalize(-light_direction.xyz);
    const float3 half_direction = normalize(light_dir + view_direction);

    const float3 fresnel =
        fresnel_schlick(max(dot(half_direction, view_direction), 0.0f), f0);

    const float3 specular = calculate_specular_brdf(
        fresnel, light_dir, half_direction, normal, view_direction, roughness
    );

    const float3 k_s = fresnel;
    float3 k_d = 1.0f - k_s;
    k_d *= 1.0f - metallic;

    const float n_dot_l = max(dot(normal, light_dir), 0.0f);

    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

float4 main(PSInput input) : SV_Target {
    const float4 albedo_alpha = albedo_texture.Sample(albedo_sampler, input.uv);
    const float3 albedo = albedo_alpha.rgb;

    float3 normal_map = normal_texture.Sample(normal_sampler, input.uv).rgb;
    normal_map = normalize(normal_map * 2.0f - 1.0f);

    const float3 N = normalize(input.world_normal);
    const float3 T = normalize(input.world_tangent - N * dot(input.world_tangent, N));
    const float3 B = cross(N, T);
    const float3x3 tbn = float3x3(T, B, N);

    const float3 normal = normalize(mul(normal_map, tbn));

    const float metallic = metallic_texture.Sample(metallic_sampler, input.uv).b;
    const float roughness = roughness_texture.Sample(roughness_sampler, input.uv).g;
    const float ao = ao_texture.Sample(ao_sampler, input.uv).r;

    const float3 view_direction = normalize(view_position.xyz - input.world_position);

    float3 f0 = float3(0.04f, 0.04f, 0.04f);
    f0 = lerp(f0, albedo, metallic);

    const float3 Lo = calculate_directional_light(
        normal, view_direction, albedo, f0, metallic, roughness
    );

    const float3 ambient = 0.03f * albedo * ao;
    const float3 color = ambient + Lo;

    return float4(color, albedo_alpha.a);
}
