#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/BoundingBox.hpp>
#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class RenderPass;
class CopyPass;
class CommandBuffer;

struct TextureImages {
    std::optional<Utilities::ImageLoader::Image> diffuse_texture;
    Utilities::ModelLoader::TextureWrap diffuse_texture_wrap;
    std::optional<Utilities::ImageLoader::Image> normal_texture;
    Utilities::ModelLoader::TextureWrap normal_texture_wrap;
    std::optional<Utilities::ImageLoader::Image> metallic_texture;
    Utilities::ModelLoader::TextureWrap metallic_texture_wrap;
    std::optional<Utilities::ImageLoader::Image> roughness_texture;
    Utilities::ModelLoader::TextureWrap roughness_texture_wrap;
    std::optional<Utilities::ImageLoader::Image> ambient_occlusion_texture;
    Utilities::ModelLoader::TextureWrap ambient_occlusion_texture_wrap;
    Utilities::ModelLoader::AlphaMode alpha_mode =
        Utilities::ModelLoader::AlphaMode::Opaque;
};

// A submesh's draw parameters into its renderable's shared vertex/index
// buffers (see RenderableMeshes) - not owned buffers of its own, so several
// submeshes can be batched into a single indirect multi-draw call sharing
// one bound vertex/index buffer.
class SDL_GPUMesh {
public:
    SDL_GPUMesh(
        GPUDevice& device,
        CopyPass& copy_pass,
        uint32_t first_index,
        uint32_t index_count,
        int32_t vertex_offset,
        const BoundingBox& local_bounds,
        const TexturePaths& texture_paths
    );

    SDL_GPUMesh(
        GPUDevice& device,
        CopyPass& copy_pass,
        uint32_t first_index,
        uint32_t index_count,
        int32_t vertex_offset,
        const BoundingBox& local_bounds,
        const TextureImages& texture_images
    );

    // Caller must have already bound this mesh's renderable's shared
    // vertex/index buffers (see RenderableMeshes) on sdl_gpu_pass.
    auto draw(RenderPass& sdl_gpu_pass) const -> void;
    auto draw_instanced(int32_t instance_count, RenderPass& sdl_gpu_pass) const
        -> void;

    // Issues the draw call without binding material samplers. Used by passes
    // whose fragment shader doesn't sample any of this mesh's material
    // textures (e.g. the SSAO normal prepass). Caller must have already
    // bound this mesh's renderable's shared vertex/index buffers.
    auto draw_instanced_geometry_only(
        int32_t instance_count, RenderPass& sdl_gpu_pass
    ) const -> void;

    // Binds this mesh's material samplers, then issues one indirect draw
    // command read from indirect_buffer at byte_offset (see
    // SDL_GPUInstanceCullPass - the command's num_instances is written by a
    // GPU culling compute pass, not known on the CPU). Caller must have
    // already bound this mesh's renderable's shared vertex/index buffers.
    auto draw_indirect(
        RenderPass& sdl_gpu_pass,
        const Buffer& indirect_buffer,
        uint32_t byte_offset
    ) const -> void;

    // Same as draw_indirect, but without binding material samplers (see
    // draw_instanced_geometry_only).
    auto draw_indirect_geometry_only(
        RenderPass& sdl_gpu_pass,
        const Buffer& indirect_buffer,
        uint32_t byte_offset
    ) const -> void;

    [[nodiscard]] auto alpha_mode() const -> Utilities::ModelLoader::AlphaMode;

    [[nodiscard]] auto get_first_index() const -> uint32_t;
    [[nodiscard]] auto get_index_count() const -> uint32_t;
    [[nodiscard]] auto get_vertex_offset() const -> int32_t;

    // Local-space bounding box derived from vertex positions at construction
    // time, used for CPU-side frustum culling (e.g. per-light shadow pass
    // draw skipping).
    [[nodiscard]] auto get_local_bounds() const -> const BoundingBox&;

    // Generates mip chains for the material textures. Must be called outside
    // any render/copy pass on command_buffer.
    auto generate_mipmaps(CommandBuffer& command_buffer) const -> void;

private:
    uint32_t first_index;
    uint32_t index_count;
    int32_t vertex_offset;
    BoundingBox local_bounds;

    Texture diffuse_texture;
    Texture normal_texture;
    Texture metallic_texture;
    Texture roughness_texture;
    Texture ambient_occlusion_texture;

    Sampler diffuse_sampler;
    Sampler normal_sampler;
    Sampler metallic_sampler;
    Sampler roughness_sampler;
    Sampler ambient_occlusion_sampler;

    Utilities::ModelLoader::AlphaMode mesh_alpha_mode =
        Utilities::ModelLoader::AlphaMode::Opaque;
};

// A renderable's shared vertex/index buffers plus its submeshes, each of
// which only stores draw-parameter offsets into these shared buffers (see
// SDL_GPUMesh). Sharing one buffer pair per renderable lets all of its
// submeshes be drawn via a single bound vertex/index buffer, which is
// required for indirect multi-draw batching (SDL_GPUPointSpotShadowPass).
struct RenderableMeshes {
    Buffer vertex_buffer;
    Buffer index_buffer;
    std::vector<SDL_GPUMesh> meshes;
};

// Axis-aligned bounding box over vertex positions (first 3 of the 11-float
// vertex stride: position, uv, normal, tangent), used for CPU-side frustum
// culling. Tighter than a bounding sphere for large flat/elongated meshes
// (e.g. building floors/walls), where a sphere's radius would be close to
// the shape's full diagonal.
[[nodiscard]] auto compute_mesh_local_bounds(gsl::span<const float> vertices)
    -> BoundingBox;

[[nodiscard]] auto load_meshes_from_model(
    GPUDevice& device, const std::filesystem::path& model_path
) -> RenderableMeshes;

}  // namespace Luminol::Graphics::SDL_GPU
