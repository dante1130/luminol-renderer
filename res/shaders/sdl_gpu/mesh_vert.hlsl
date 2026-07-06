cbuffer UBO : register(b0, space1) {
    row_major float4x4 mvp;
};

struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(float4(input.position, 1.0f), mvp);
    output.uv = input.uv;
    return output;
}
