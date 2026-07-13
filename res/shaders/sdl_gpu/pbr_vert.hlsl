cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
};

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);
// Indirection layer written by SDL_GPUInstanceCullPass's compute shader
// (instance_cull.hlsl): maps a compacted 0..num_instances-1 range back to
// the original instance index in instance_models, so a culled/compacted
// indirect draw can still fetch the right model matrix per instance. Passes
// that don't cull (point/spot shadows) bind an identity mapping instead (see
// SDL_GPUInstanceBufferCache::get_identity_indices_buffer).
//
// Indexed directly by input.instance_id, with no per-draw base-offset
// uniform: each submesh's IndirectDrawCommand carries its own base offset
// into visible_instance_indices via first_instance
// (SDL_GPUInstanceCullPass::cull), and every target graphics API guarantees
// SV_InstanceID for an indirect/instanced draw already incorporates
// first_instance. That's what lets multiple submeshes' commands be drawn in
// one multi-draw-indirect call instead of one draw per submesh - varying
// first_instance per sub-draw works where a per-draw uniform push wouldn't.
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
        visible_instance_indices[input.instance_id];
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
