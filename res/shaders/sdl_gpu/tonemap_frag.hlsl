Texture2D hdr_texture : register(t0, space2);
SamplerState hdr_sampler : register(s0, space2);

cbuffer TonemapBuffer : register(b0, space3) {
    float exposure;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    const float gamma = 2.2f;
    float3 hdr_color = hdr_texture.Sample(hdr_sampler, input.uv).rgb;
    float3 mapped = 1.0f - exp(-hdr_color * exposure);
    mapped = pow(mapped, float3(1.0f / gamma, 1.0f / gamma, 1.0f / gamma));
    return float4(mapped, 1.0f);
}
