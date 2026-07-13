cbuffer TextVertexUniforms : register(b0, space1) {
    float2 screen_size;
    float2 padding;
};

struct VSInput {
    float2 position : POSITION;
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 color : TEXCOORD1;
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    VSOutput output;
    output.position = float4(
        (input.position.x / screen_size.x) * 2.0f - 1.0f,
        1.0f - (input.position.y / screen_size.y) * 2.0f,
        0.0f,
        1.0f
    );
    output.uv = input.uv;
    output.color = input.color;
    return output;
}
