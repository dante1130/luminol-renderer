#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class RenderPass;
class CopyPass;

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
};

class SDL_GPUMesh {
public:
    SDL_GPUMesh(
        GPUDevice& device,
        CopyPass& copy_pass,
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    );

    SDL_GPUMesh(
        GPUDevice& device,
        CopyPass& copy_pass,
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TextureImages& texture_images
    );

    auto draw(RenderPass& sdl_gpu_pass) const -> void;
    auto draw_instanced(int32_t instance_count, RenderPass& sdl_gpu_pass) const
        -> void;

    // Binds only vertex/index buffers and issues the draw call, without
    // binding material samplers. Used by passes whose fragment shader
    // doesn't sample any of this mesh's material textures (e.g. the SSAO
    // normal prepass).
    auto draw_instanced_geometry_only(
        int32_t instance_count, RenderPass& sdl_gpu_pass
    ) const -> void;

private:
    Buffer vertex_buffer;
    Buffer index_buffer;
    uint32_t index_count;

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
};

[[nodiscard]] auto load_meshes_from_model(
    GPUDevice& device, const std::filesystem::path& model_path
) -> std::vector<SDL_GPUMesh>;

}  // namespace Luminol::Graphics::SDL_GPU
