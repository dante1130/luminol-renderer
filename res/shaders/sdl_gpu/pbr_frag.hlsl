Texture2D albedo_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);
Texture2D metallic_texture : register(t2, space2);
Texture2D roughness_texture : register(t3, space2);
Texture2D ao_texture : register(t4, space2);
Texture2D ssao_texture : register(t5, space2);
Texture2D shadow_map_texture : register(t6, space2);

SamplerState albedo_sampler : register(s0, space2);
SamplerState normal_sampler : register(s1, space2);
SamplerState metallic_sampler : register(s2, space2);
SamplerState roughness_sampler : register(s3, space2);
SamplerState ao_sampler : register(s4, space2);
SamplerState ssao_sampler : register(s5, space2);
SamplerComparisonState shadow_map_sampler : register(s6, space2);

cbuffer LightBuffer : register(b0, space3) {
    float4 light_direction;
    float4 light_color;
    float4 view_position;
    float4 screen_size;
    row_major float4x4 light_space_matrix;
    float4 shadow_params; // x: shadow map resolution, y: normal-offset bias
};

struct PSInput {
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    float3 world_tangent : TEXCOORD3;
    float4 screen_position : SV_Position;
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

float visibility_smith_ggx_correlated(
    float n_dot_v, float n_dot_l, float roughness
) {
    const float a2 = roughness * roughness;
    const float ggx_v = n_dot_l * sqrt(n_dot_v * n_dot_v * (1.0f - a2) + a2);
    const float ggx_l = n_dot_v * sqrt(n_dot_l * n_dot_l * (1.0f - a2) + a2);

    return 0.5f / max(ggx_v + ggx_l, 0.0001f);
}

float3 fresnel_schlick(float cos_theta, float3 f0) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - cos_theta, 0.0f, 1.0f), 5.0f);
}

float specular_antialiasing(float3 normal, float roughness) {
    const float SPECULAR_AA_VARIANCE = 0.25f;
    const float SPECULAR_AA_THRESHOLD = 0.18f;

    const float3 dndu = ddx(normal);
    const float3 dndv = ddy(normal);
    const float variance = SPECULAR_AA_VARIANCE * (dot(dndu, dndu) + dot(dndv, dndv));

    const float kernel_roughness2 = min(2.0f * variance, SPECULAR_AA_THRESHOLD);
    const float filtered_roughness2 = saturate(roughness * roughness + kernel_roughness2);

    return sqrt(filtered_roughness2);
}

float2 env_brdf_approx(float n_dot_v, float roughness) {
    const float4 c0 = float4(-1.0f, -0.0275f, -0.572f, 0.022f);
    const float4 c1 = float4(1.0f, 0.0425f, 1.04f, -0.04f);
    const float4 r = roughness * c0 + c1;

    const float a004 = min(r.x * r.x, exp2(-9.28f * n_dot_v)) * r.x + r.y;

    return float2(-1.04f, 1.04f) * a004 + r.zw;
}

float3 energy_compensation(float3 f0, float n_dot_v, float roughness) {
    const float2 ab = env_brdf_approx(n_dot_v, roughness);
    const float ess = max(ab.x + ab.y, 0.001f);

    return 1.0f + f0 * (1.0f / ess - 1.0f);
}

float3 calculate_specular_brdf(
    float3 fresnel,
    float3 light_direction,
    float3 half_direction,
    float3 normal,
    float3 view_direction,
    float3 f0,
    float roughness
) {
    const float n_dot_v = max(dot(normal, view_direction), 0.0f);
    const float n_dot_l = max(dot(normal, light_direction), 0.0f);

    const float normal_distribution = distribution_ggx(normal, half_direction, roughness);
    const float visibility = visibility_smith_ggx_correlated(n_dot_v, n_dot_l, roughness);

    const float3 specular = normal_distribution * visibility * fresnel;

    return specular * energy_compensation(f0, n_dot_v, roughness);
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
        fresnel, light_dir, half_direction, normal, view_direction, f0, roughness
    );

    const float3 k_s = fresnel;
    float3 k_d = 1.0f - k_s;
    k_d *= 1.0f - metallic;

    const float n_dot_l = max(dot(normal, light_dir), 0.0f);

    return (k_d * albedo / PI + specular) * radiance * n_dot_l;
}

float calculate_shadow(float3 world_position, float3 normal) {
    const float normal_offset_bias = shadow_params.y;
    const float3 offset_position = world_position + normal * normal_offset_bias;

    const float4 light_space_position =
        mul(float4(offset_position, 1.0f), light_space_matrix);

    float2 shadow_uv = light_space_position.xy * 0.5f + 0.5f;
    shadow_uv.y = 1.0f - shadow_uv.y;
    const float fragment_depth = light_space_position.z;

    if (fragment_depth < 0.0f || fragment_depth > 1.0f ||
        any(shadow_uv < 0.0f) || any(shadow_uv > 1.0f)) {
        return 1.0f;
    }

    const float texel_size = 1.0f / max(shadow_params.x, 1.0f);
    const float constant_bias = 0.0015f;

    float visibility = 0.0f;
    [unroll]
    for (int x = -1; x <= 1; ++x) {
        [unroll]
        for (int y = -1; y <= 1; ++y) {
            const float2 offset = float2(x, y) * texel_size;
            visibility += shadow_map_texture.SampleCmpLevelZero(
                shadow_map_sampler, shadow_uv + offset, fragment_depth - constant_bias
            );
        }
    }

    return visibility / 9.0f;
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
    const float roughness =
        specular_antialiasing(normal, roughness_texture.Sample(roughness_sampler, input.uv).g);
    const float ao = ao_texture.Sample(ao_sampler, input.uv).r;

    const float2 screen_uv = input.screen_position.xy / screen_size.xy;
    const float ssao = ssao_texture.Sample(ssao_sampler, screen_uv).r;

    const float3 view_direction = normalize(view_position.xyz - input.world_position);

    float3 f0 = float3(0.04f, 0.04f, 0.04f);
    f0 = lerp(f0, albedo, metallic);

    const float3 Lo = calculate_directional_light(
        normal, view_direction, albedo, f0, metallic, roughness
    );
    const float shadow = calculate_shadow(input.world_position, N);

    const float3 ambient = 0.03f * albedo * ao * ssao;
    const float3 color = ambient + Lo * shadow;

    return float4(color, albedo_alpha.a);
}
