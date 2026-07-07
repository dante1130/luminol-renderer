Texture2D ssao_texture : register(t0, space2);
SamplerState ssao_sampler : register(s0, space2);

// x = width, y = height, zw = unused.
cbuffer BlurBuffer : register(b0, space3) {
    float4 viewport_size;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    const float2 texel_size = 1.0f / viewport_size.xy;

    float sum = 0.0f;
    for (int x = -2; x < 2; ++x) {
        for (int y = -2; y < 2; ++y) {
            const float2 offset = float2(float(x), float(y)) * texel_size;
            sum += ssao_texture.Sample(ssao_sampler, input.uv + offset).r;
        }
    }

    return float4((sum / 16.0f).xxx, 1.0f);
}
