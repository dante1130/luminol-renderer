#pragma once

#include <array>
#include <unordered_map>
#include <vector>

#include <gsl/gsl>
#include <LuminolMaths/Matrix.hpp>
#include <LuminolMaths/Vector.hpp>

#include <LuminolRenderEngine/Graphics/RenderableManager.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBatch.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceBufferCache.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUInstanceCullPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class CopyPass;
class CommandBuffer;
class RenderPass;
class SDL_GPUFactory;

// Number of cascades the directional shadow map (SDL_GPUShadowPass) is
// split into. Lives here rather than SDL_GPUShadowPass.hpp so that LightData
// below can size its cascade arrays without SDL_GPUShadowPass.hpp and this
// header needing to include each other.
inline constexpr auto shadow_pass_num_cascades = 4U;

// Layout matches the LightBuffer cbuffer in pbr_frag.hlsl (register b0,
// space3): six float4s, then shadow_pass_num_cascades row_major float4x4s,
// then two more float4s, no manual padding needed beyond what's declared
// below. Point/spot lights are no longer part of this cbuffer - they're read
// from storage buffers (see SDL_GPUClusterPass) via the per-cluster light
// index list built by the clustered light-culling compute passes.
struct LightData {
    Maths::Vector4f direction;
    Maths::Vector4f color;
    Maths::Vector4f view_position;
    Maths::Vector4f screen_size;
    // x: shadow map resolution, y: normal-offset bias,
    // z: max prefiltered specular mip level (see SDL_GPUIBLRenderPass)
    Maths::Vector4f shadow_params;
    // x: camera near plane, y: camera far plane (for view-space depth
    // reconstruction when computing the cluster index), z/w: unused.
    Maths::Vector4f cluster_params;
    std::array<Maths::Matrix4x4f, shadow_pass_num_cascades>
        cascade_light_space_matrices;
    // View-space far distance of each cascade (see SDL_GPUShadowPass),
    // used by pbr_frag.hlsl to pick which cascade a fragment falls into.
    Maths::Vector4f cascade_split_depths;
    // xyz: camera forward direction (world space), used alongside
    // view_position to compute a fragment's linear view-space depth for
    // cascade selection. w: unused.
    Maths::Vector4f camera_forward;
};

// The Clustered Forward+ buffers produced by SDL_GPUClusterPass, bound as
// pbr_frag.hlsl fragment storage buffers starting right after its 10
// existing textures/samplers (see cluster_light_buffer_slot in
// SDL_GPUMeshRenderPass.cpp).
struct ClusteredLightBuffers {
    const Buffer* point_lights;
    const Buffer* spot_lights;
    const Buffer* cluster_light_grid;
    const Buffer* global_light_index_list;
};

// Bundles the precomputed split-sum IBL textures/samplers produced by
// SDL_GPUIBLRenderPass, bound as the pbr_frag.hlsl fragment samplers at
// slots 7-9 (see irradiance_sampler_slot etc. in SDL_GPUMeshRenderPass.cpp).
struct IBLTextures {
    const Texture* irradiance_texture;
    const Sampler* irradiance_sampler;
    const Texture* prefiltered_texture;
    const Sampler* prefiltered_sampler;
    uint32_t prefiltered_mip_count;
    const Texture* brdf_lut_texture;
    const Sampler* brdf_lut_sampler;
};

// Shadow maps produced by SDL_GPUPointSpotShadowPass for a capped,
// frame-selected subset of point/spot lights, bound as the pbr_frag.hlsl
// fragment samplers at slots 10-11 (see point_shadow_sampler_slot etc. in
// SDL_GPUMeshRenderPass.cpp) plus a per-slot spot shadow-matrix buffer.
struct PointSpotShadowTextures {
    const Texture* point_shadow_texture;
    const Sampler* point_shadow_sampler;
    const Texture* spot_shadow_texture;
    const Sampler* spot_shadow_sampler;
    const Buffer* spot_shadow_matrices;
};

// Owns the mesh pipeline (position/uv vertex layout, view_proj uniform,
// per-instance model matrices via a storage buffer indexed by
// SV_InstanceID) and the persistent instance buffers backing it. The pass
// owns its own GPU resources and exposes upload/draw steps, while
// SDL_GPURenderer just coordinates passes.
class SDL_GPUMeshRenderPass {
public:
    SDL_GPUMeshRenderPass(GPUDevice& device, SampleCount sample_count);

    [[nodiscard]] auto upload_instances(
        GPUDevice& device,
        CopyPass& copy_pass,
        const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
            queued_draws
    ) -> std::vector<InstanceBatch>;

    auto draw(
        const SDL_GPUFactory& graphics_factory,
        CommandBuffer& command_buffer,
        RenderPass& render_pass,
        gsl::span<const InstanceBatch> instance_batches,
        const std::unordered_map<RenderableId, std::vector<Maths::Matrix4x4f>>&
            queued_draws,
        const Maths::Matrix4x4f& view_proj,
        const std::array<Maths::Vector4f, 6>& camera_frustum_planes,
        const Buffer& indirect_command_buffer,
        const Buffer& visible_instance_indices_buffer,
        const InstanceCullLayout& instance_cull_layout,
        const LightData& light_data,
        const Texture& ssao_texture,
        const Sampler& ssao_sampler,
        const Texture& shadow_map_texture,
        const Sampler& shadow_map_sampler,
        const IBLTextures& ibl_textures,
        const ClusteredLightBuffers& clustered_light_buffers,
        const PointSpotShadowTextures& point_spot_shadow_textures,
        const Texture& ssr_texture,
        const Sampler& ssr_sampler
    ) -> void;

    [[nodiscard]] auto get_instance_buffer_cache() const
        -> const SDL_GPUInstanceBufferCache&;

private:
    Shader mesh_vertex_shader;
    Shader mesh_fragment_shader;
    Shader mesh_alpha_test_fragment_shader;
    GraphicsPipeline mesh_pipeline;
    GraphicsPipeline mesh_alpha_test_pipeline;
    GraphicsPipeline mesh_transparent_pipeline;

    SDL_GPUInstanceBufferCache instance_buffer_cache;
};

}  // namespace Luminol::Graphics::SDL_GPU
