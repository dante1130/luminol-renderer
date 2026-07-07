cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
};

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);

struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    uint instance_id : SV_InstanceID;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    const row_major float4x4 instance_model = instance_models[input.instance_id];

    VSOutput output;
    output.position = mul(
        mul(float4(input.position, 1.0f), instance_model), view_proj
    );
    output.uv = input.uv;
    return output;
}
