cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
    // x: base offset into visible_instance_indices for this draw call;
    // y/z/w unused (kept so the cbuffer stays a clean multiple of 16 bytes).
    uint4 instance_base_offset;
};

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);
// Indirection layer written by SDL_GPUInstanceCullPass's compute shader
// (instance_cull.hlsl): maps a compacted 0..num_instances-1 range back to
// the original instance index in instance_models, so a culled/compacted
// indirect draw can still fetch the right model matrix per instance. Passes
// that don't cull (shadow passes) bind an identity mapping instead (see
// SDL_GPUInstanceBufferCache::get_identity_indices_buffer).
StructuredBuffer<uint> visible_instance_indices : register(t1, space0);

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
    const uint original_instance_index =
        visible_instance_indices[instance_base_offset.x + input.instance_id];
    const row_major float4x4 instance_model = instance_models[original_instance_index];
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
