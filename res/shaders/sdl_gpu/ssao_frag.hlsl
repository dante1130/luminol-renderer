Texture2D depth_texture : register(t0, space2);
Texture2D normal_texture : register(t1, space2);

SamplerState depth_sampler : register(s0, space2);
SamplerState normal_sampler : register(s1, space2);

// x = radius, y = bias, z = power, w = unused.
// viewport_size: x = width, y = height, zw = unused.
cbuffer SSAOBuffer : register(b0, space3) {
    row_major float4x4 projection_matrix;
    row_major float4x4 inverse_projection_matrix;
    float4 params;
    float4 viewport_size;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

static const int kernel_size = 16;
static const float3 kernel[kernel_size] = {
    float3(0.099951f, 0.000000f, 0.003125f),
    float3(-0.075993f, 0.069616f, 0.009705f),
    float3(0.009850f, -0.112230f, 0.017822f),
    float3(0.078155f, 0.101940f, 0.028796f),
    float3(-0.147651f, -0.026117f, 0.043945f),
    float3(0.148873f, -0.094701f, 0.064587f),
    float3(-0.053744f, 0.199926f, 0.092041f),
    float3(-0.110848f, -0.213432f, 0.127625f),
    float3(0.258637f, 0.094454f, 0.172656f),
    float3(-0.286179f, 0.118130f, 0.228455f),
    float3(0.144414f, -0.308605f, 0.296338f),
    float3(0.109325f, 0.348544f, 0.377625f),
    float3(-0.327424f, -0.189749f, 0.473633f),
    float3(0.363881f, -0.079998f, 0.585681f),
    float3(-0.191846f, 0.272881f, 0.715088f),
    float3(-0.028402f, -0.219174f, 0.863171f),
};

float3 reconstruct_view_position(float2 uv, float depth) {
    const float2 ndc = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    float4 view_position = mul(float4(ndc, depth, 1.0f), inverse_projection_matrix);
    view_position /= view_position.w;
    return view_position.xyz;
}

// Interleaved gradient noise: cheap per-pixel pseudo-random rotation angle,
// used instead of an uploaded noise texture.
float interleaved_gradient_noise(float2 pixel_coord) {
    const float3 magic = float3(0.06711056f, 0.00583715f, 52.9829189f);
    return frac(magic.z * frac(dot(pixel_coord, magic.xy)));
}

float4 main(PSInput input) : SV_Target {
    const float radius = params.x;
    const float bias = params.y;
    const float power = params.z;

    const float depth = depth_texture.Sample(depth_sampler, input.uv).r;
    if (depth >= 1.0f) {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }

    const float3 view_position = reconstruct_view_position(input.uv, depth);
    const float3 view_normal =
        normalize(normal_texture.Sample(normal_sampler, input.uv).rgb * 2.0f - 1.0f);

    const float noise_angle =
        interleaved_gradient_noise(input.uv * viewport_size.xy) * 6.28318530718f;
    const float noise_sin = sin(noise_angle);
    const float noise_cos = cos(noise_angle);

    const float3 up = abs(view_normal.z) < 0.999f
        ? float3(0.0f, 0.0f, 1.0f)
        : float3(1.0f, 0.0f, 0.0f);
    const float3 tangent = normalize(cross(up, view_normal));
    const float3 bitangent = cross(view_normal, tangent);
    const float3x3 tbn = float3x3(tangent, bitangent, view_normal);

    float occlusion = 0.0f;
    for (int i = 0; i < kernel_size; ++i) {
        const float3 rotated_sample = float3(
            kernel[i].x * noise_cos - kernel[i].y * noise_sin,
            kernel[i].x * noise_sin + kernel[i].y * noise_cos,
            kernel[i].z
        );

        const float3 sample_view_position =
            view_position + mul(rotated_sample, tbn) * radius;

        float4 sample_clip =
            mul(float4(sample_view_position, 1.0f), projection_matrix);
        sample_clip.xyz /= sample_clip.w;

        const float2 sample_uv =
            sample_clip.xy * float2(0.5f, -0.5f) + 0.5f;

        const float scene_depth = depth_texture.Sample(depth_sampler, sample_uv).r;
        const float3 scene_view_position =
            reconstruct_view_position(sample_uv, scene_depth);

        const float range_check = smoothstep(
            0.0f, 1.0f, radius / max(abs(view_position.z - scene_view_position.z), 0.0001f)
        );

        occlusion += (scene_view_position.z <= sample_view_position.z - bias ? 1.0f : 0.0f)
            * range_check;
    }

    const float ao = pow(saturate(1.0f - occlusion / kernel_size), power);

    return float4(ao.xxx, 1.0f);
}
