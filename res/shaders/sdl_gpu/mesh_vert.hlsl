cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
};

struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    row_major float4x4 instance_model : TEXCOORD2;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = mul(
        mul(float4(input.position, 1.0f), input.instance_model), view_proj
    );
    output.uv = input.uv;
    return output;
}
