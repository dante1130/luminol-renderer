cbuffer UBO : register(b0, space1) {
    row_major float4x4 inv_view_proj;
};

struct VSOutput {
    float3 view_dir : TEXCOORD0;
    float4 position : SV_Position;
};

VSOutput main(uint vertex_id : SV_VertexID) {
    const float2 uv = float2((vertex_id << 1) & 2, vertex_id & 2);
    const float2 pos = uv * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);

    VSOutput output;
    output.position = float4(pos, 1.0f, 1.0f);

    const float4 world = mul(float4(pos, 1.0f, 1.0f), inv_view_proj);
    output.view_dir = world.xyz / world.w;

    return output;
}
