Texture2D glyph_texture : register(t0, space2);
SamplerState glyph_sampler : register(s0, space2);

cbuffer TextFragmentUniforms : register(b0, space3) {
    float4 tint;
};

struct PSInput {
    float2 uv : TEXCOORD0;
};

float4 main(PSInput input) : SV_Target {
    return glyph_texture.Sample(glyph_sampler, input.uv) * tint;
}
