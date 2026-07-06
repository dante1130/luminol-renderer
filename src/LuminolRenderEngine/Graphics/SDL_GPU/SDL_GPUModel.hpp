#pragma once

#include <filesystem>

#include <LuminolRenderEngine/Graphics/Model.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUMesh.hpp>

namespace Luminol::Graphics::SDL_GPU {

class GPUDevice;

class SDL_GPUModel : public Model {
public:
    SDL_GPUModel(GPUDevice& device, const std::filesystem::path& model_path);

    auto draw() const -> void override;
    auto draw_instanced(int32_t instance_count) const -> void override;

    auto draw(RenderPass& sdl_gpu_pass) const -> void override;
    auto draw_instanced(int32_t instance_count, RenderPass& sdl_gpu_pass) const
        -> void override;

private:
    std::vector<SDL_GPUMesh> meshes;
};

}  // namespace Luminol::Graphics::SDL_GPU
