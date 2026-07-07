cbuffer ViewBuffer : register(b0, space3) {
    row_major float4x4 view_matrix;
};

struct PSInput {
    float2 uv : TEXCOORD0;
    float3 world_position : TEXCOORD1;
    float3 world_normal : TEXCOORD2;
    float3 world_tangent : TEXCOORD3;
};

float4 main(PSInput input) : SV_Target {
    const float3x3 view_rotation = (float3x3)view_matrix;
    const float3 view_normal = normalize(mul(normalize(input.world_normal), view_rotation));

    return float4(view_normal * 0.5f + 0.5f, 1.0f);
}
