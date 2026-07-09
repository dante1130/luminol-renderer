TextureCube source_texture : register(t0, space2);
SamplerState source_sampler : register(s0, space2);

struct PSInput {
    float3 view_dir : TEXCOORD0;
};

static const float PI = 3.14159265359f;

float4 main(PSInput input) : SV_Target {
    const float3 normal = normalize(input.view_dir);

    float3 up = float3(0.0f, 1.0f, 0.0f);
    if (abs(dot(up, normal)) > 0.999f) {
        up = float3(1.0f, 0.0f, 0.0f);
    }
    const float3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float3 irradiance = float3(0.0f, 0.0f, 0.0f);

    const float phi_step = 0.025f;
    const float theta_step = 0.025f;
    float sample_count = 0.0f;

    for (float phi = 0.0f; phi < 2.0f * PI; phi += phi_step) {
        for (float theta = 0.0f; theta < 0.5f * PI; theta += theta_step) {
            const float3 tangent_sample =
                float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            const float3 sample_dir =
                tangent_sample.x * right + tangent_sample.y * up + tangent_sample.z * normal;

            irradiance += source_texture.SampleLevel(source_sampler, sample_dir, 0.0f).rgb
                * cos(theta) * sin(theta);
            sample_count += 1.0f;
        }
    }

    irradiance = PI * irradiance / max(sample_count, 1.0f);

    return float4(irradiance, 1.0f);
}
