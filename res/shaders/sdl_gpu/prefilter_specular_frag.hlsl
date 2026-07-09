TextureCube source_texture : register(t0, space2);
SamplerState source_sampler : register(s0, space2);

cbuffer PrefilterBuffer : register(b0, space3) {
    float roughness;
};

struct PSInput {
    float3 view_dir : TEXCOORD0;
};

static const float PI = 3.14159265359f;
static const uint SAMPLE_COUNT = 32u;

float radical_inverse_vdc(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10f;
}

float2 hammersley(uint i, uint n) {
    return float2(float(i) / float(n), radical_inverse_vdc(i));
}

float3 importance_sample_ggx(float2 xi, float3 normal, float rough) {
    const float a = rough * rough;

    const float phi = 2.0f * PI * xi.x;
    const float cos_theta = sqrt((1.0f - xi.y) / (1.0f + (a * a - 1.0f) * xi.y));
    const float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    const float3 half_tangent =
        float3(cos(phi) * sin_theta, sin(phi) * sin_theta, cos_theta);

    float3 up = abs(normal.z) < 0.999f ? float3(0.0f, 0.0f, 1.0f) : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(up, normal));
    const float3 bitangent = cross(normal, tangent);

    return normalize(
        tangent * half_tangent.x + bitangent * half_tangent.y + normal * half_tangent.z
    );
}

float4 main(PSInput input) : SV_Target {
    const float3 normal = normalize(input.view_dir);
    const float3 view_direction = normal;

    float3 prefiltered_color = float3(0.0f, 0.0f, 0.0f);
    float total_weight = 0.0f;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        const float2 xi = hammersley(i, SAMPLE_COUNT);
        const float3 half_direction = importance_sample_ggx(xi, normal, roughness);
        const float3 light_direction =
            normalize(2.0f * dot(view_direction, half_direction) * half_direction - view_direction);

        const float n_dot_l = max(dot(normal, light_direction), 0.0f);
        if (n_dot_l > 0.0f) {
            prefiltered_color +=
                source_texture.SampleLevel(source_sampler, light_direction, 0.0f).rgb * n_dot_l;
            total_weight += n_dot_l;
        }
    }

    prefiltered_color /= max(total_weight, 0.0001f);

    return float4(prefiltered_color, 1.0f);
}
