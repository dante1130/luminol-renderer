#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/TexturePaths.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;
class RenderPass;

class SDL_GPUMesh {
public:
    SDL_GPUMesh(
        GPUDevice& device,
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const TexturePaths& texture_paths
    );

    SDL_GPUMesh(
        GPUDevice& device,
        gsl::span<const float> vertices,
        gsl::span<const uint32_t> indices,
        const std::optional<Utilities::ImageLoader::Image>& diffuse_texture_image
    );

    auto draw(RenderPass& sdl_gpu_pass) const -> void;
    auto draw_instanced(int32_t instance_count, RenderPass& sdl_gpu_pass) const
        -> void;

private:
    Buffer vertex_buffer;
    Buffer index_buffer;
    uint32_t index_count;

    Texture texture;
    Sampler sampler;
};

[[nodiscard]] auto load_meshes_from_model(
    GPUDevice& device, const std::filesystem::path& model_path
) -> std::vector<SDL_GPUMesh>;

}  // namespace Luminol::Graphics::SDL_GPU
