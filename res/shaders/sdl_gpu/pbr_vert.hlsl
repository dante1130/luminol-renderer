cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
};

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);

struct VSInput {
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    uint instance_id : SV_InstanceID;
};

struct VSOutput {
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    float3 world_tangent : TEXCOORD3;
    float4 position : SV_Position;
};

VSOutput main(VSInput input) {
    const row_major float4x4 instance_model = instance_models[input.instance_id];
    const float3x3 normal_matrix = (float3x3)instance_model;

    const float4 world_position = mul(float4(input.position, 1.0f), instance_model);

    VSOutput output;
    output.position = mul(world_position, view_proj);
    output.world_position = world_position.xyz;
    output.world_normal = mul(input.normal, normal_matrix);
    output.world_tangent = mul(input.tangent, normal_matrix);
    output.uv = input.uv;
    return output;
}
