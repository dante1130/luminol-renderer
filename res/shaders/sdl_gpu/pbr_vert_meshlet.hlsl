// Vertex-pull vertex shader for the main color pass's meshlet-culled draws
// (Opaque/Mask only - see SDL_GPUMeshletCullPass, meshlet_cull.hlsl). Unlike
// pbr_vert.hlsl, this shader is bound with zero fixed-function vertex/index
// buffers: every attribute is manually fetched from StructuredBuffers via
// SV_VertexID/SV_InstanceID, because different meshlet-instances drawn by
// the same non-indexed indirect command need different vertex ranges, which
// a single per-draw base_vertex can't express. Precedent for a zero-vertex-
// buffer pipeline already exists in this engine (skybox_vert.hlsl,
// fullscreen_vert.hlsl).
//
// Each draw's num_vertices is fixed at MESHLET_MAX_TRIANGLES * 3 regardless
// of a meshlet's real triangle_count (SDL_GPUMeshletCullPass.cpp) - padding
// vertices past the real count collapse to a zero-area triangle below
// instead of branching the draw call itself.

#define MESHLET_MAX_TRIANGLES 64
#define VERTEX_STRIDE_FLOATS 11

cbuffer UBO : register(b0, space1) {
    row_major float4x4 view_proj;
};

StructuredBuffer<row_major float4x4> instance_models : register(t0, space0);
// x = original instance index (into instance_models), y = meshlet index
// (into meshlet_metadata) - written by meshlet_cull.hlsl. Indexed directly
// by input.instance_id, same first_instance/SV_InstanceID indirection trick
// as pbr_vert.hlsl's visible_instance_indices.
StructuredBuffer<uint2> visible_meshlet_instances : register(t1, space0);

// Mirrors GpuMeshletMetadata (SDL_GPUMesh.hpp) exactly.
struct GpuMeshletMetadata {
    uint vertex_offset;
    uint triangle_offset;
    uint vertex_count;
    uint triangle_count;
    float3 bounds_center;
    float bounds_radius;
};
StructuredBuffer<GpuMeshletMetadata> meshlet_metadata : register(t2, space0);
// Per (meshlet, local vertex slot 0..vertex_count-1): absolute index into
// combined_vertices - see SDL_GPUMesh.cpp's build_meshlets.
StructuredBuffer<uint> meshlet_vertices : register(t3, space0);
// Per (meshlet, local triangle, vertex-in-triangle), flattened: local vertex
// slot 0..vertex_count-1 - see SDL_GPUMesh.cpp's build_meshlets.
StructuredBuffer<uint> meshlet_triangles : register(t4, space0);
// The renderable's shared interleaved vertex data (position3/uv2/normal3/
// tangent3 = 11 floats/vertex), bound as a plain StructuredBuffer<float>
// instead of a struct - deliberately sidesteps StructuredBuffer struct-
// layout ambiguity entirely (see SDL_GPUInstanceCullPass.cpp's
// SubmeshCullMetadata comment for the two bugs that motivate this) since a
// tightly-packed scalar buffer has no stride/padding to get wrong.
StructuredBuffer<float> combined_vertices : register(t5, space0);

struct VSInput {
    uint vertex_id : SV_VertexID;
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
    uint2 instance_meshlet = visible_meshlet_instances[input.instance_id];
    uint original_instance_index = instance_meshlet.x;
    uint meshlet_index = instance_meshlet.y;

    GpuMeshletMetadata meshlet = meshlet_metadata[meshlet_index];

    uint triangle_in_meshlet = input.vertex_id / 3;
    uint vertex_in_triangle = input.vertex_id % 3;

    // Padding vertices (triangle_in_meshlet >= meshlet.triangle_count)
    // collapse every vertex of the triangle to the same local vertex slot,
    // producing a zero-area triangle the rasterizer doesn't cover.
    bool is_padding = triangle_in_meshlet >= meshlet.triangle_count;
    uint clamped_triangle =
        min(triangle_in_meshlet, meshlet.triangle_count - 1);
    uint effective_vertex_in_triangle = is_padding ? 0 : vertex_in_triangle;

    uint local_vertex_slot = meshlet_triangles[
        meshlet.triangle_offset + (clamped_triangle * 3) +
        effective_vertex_in_triangle
    ];
    uint absolute_vertex_index =
        meshlet_vertices[meshlet.vertex_offset + local_vertex_slot];

    uint vertex_base = absolute_vertex_index * VERTEX_STRIDE_FLOATS;
    float3 position = float3(
        combined_vertices[vertex_base + 0],
        combined_vertices[vertex_base + 1],
        combined_vertices[vertex_base + 2]
    );
    float2 uv = float2(
        combined_vertices[vertex_base + 3], combined_vertices[vertex_base + 4]
    );
    float3 normal = float3(
        combined_vertices[vertex_base + 5],
        combined_vertices[vertex_base + 6],
        combined_vertices[vertex_base + 7]
    );
    float3 tangent = float3(
        combined_vertices[vertex_base + 8],
        combined_vertices[vertex_base + 9],
        combined_vertices[vertex_base + 10]
    );

    row_major float4x4 instance_model = instance_models[original_instance_index];
    float3x3 normal_matrix = (float3x3)instance_model;

    float4 world_position = mul(float4(position, 1.0f), instance_model);

    VSOutput output;
    output.position = mul(world_position, view_proj);
    output.world_position = world_position.xyz;
    output.world_normal = mul(normal, normal_matrix);
    output.world_tangent = mul(tangent, normal_matrix);
    output.uv = uv;
    return output;
}
