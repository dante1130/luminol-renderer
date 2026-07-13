Texture2D glyph_texture : register(t0, space2);
SamplerState glyph_sampler : register(s0, space2);

struct PSInput {
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

float4 main(PSInput input) : SV_Target {
    const float coverage = glyph_texture.Sample(glyph_sampler, input.uv).a;
    return float4(input.color.rgb, input.color.a * coverage);
}
