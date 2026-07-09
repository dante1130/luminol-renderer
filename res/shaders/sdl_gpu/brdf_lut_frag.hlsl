struct PSInput {
    float2 uv : TEXCOORD0;
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

float3 importance_sample_ggx(float2 xi, float3 normal, float roughness) {
    const float a = roughness * roughness;

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

float geometry_schlick_ggx(float n_dot_v, float roughness) {
    const float a = roughness;
    const float k = (a * a) / 2.0f;

    return n_dot_v / (n_dot_v * (1.0f - k) + k);
}

float geometry_smith(float n_dot_v, float n_dot_l, float roughness) {
    return geometry_schlick_ggx(n_dot_v, roughness) * geometry_schlick_ggx(n_dot_l, roughness);
}

float2 integrate_brdf(float n_dot_v, float roughness) {
    const float3 view_direction =
        float3(sqrt(1.0f - n_dot_v * n_dot_v), 0.0f, n_dot_v);

    float a = 0.0f;
    float b = 0.0f;

    const float3 normal = float3(0.0f, 0.0f, 1.0f);

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        const float2 xi = hammersley(i, SAMPLE_COUNT);
        const float3 half_direction = importance_sample_ggx(xi, normal, roughness);
        const float3 light_direction = normalize(
            2.0f * dot(view_direction, half_direction) * half_direction - view_direction
        );

        const float n_dot_l = max(light_direction.z, 0.0f);
        const float n_dot_h = max(half_direction.z, 0.0f);
        const float v_dot_h = max(dot(view_direction, half_direction), 0.0f);

        if (n_dot_l > 0.0f) {
            const float geometry = geometry_smith(n_dot_v, n_dot_l, roughness);
            const float geometry_vis = (geometry * v_dot_h) / max(n_dot_h * n_dot_v, 0.0001f);
            const float fresnel_c = pow(1.0f - v_dot_h, 5.0f);

            a += (1.0f - fresnel_c) * geometry_vis;
            b += fresnel_c * geometry_vis;
        }
    }

    return float2(a, b) / float(SAMPLE_COUNT);
}

float4 main(PSInput input) : SV_Target {
    const float2 integrated = integrate_brdf(input.uv.x, input.uv.y);
    return float4(integrated, 0.0f, 1.0f);
}
