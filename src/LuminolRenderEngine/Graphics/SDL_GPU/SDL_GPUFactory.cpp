#include "SDL_GPUFactory.hpp"

#include <cstring>
#include <optional>

#include <gsl/gsl>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

// Rounds down to the nearest sample count SDL_GPU supports.
auto to_sample_count(uint32_t msaa_sample_count) -> SampleCount {
    if (msaa_sample_count >= 8) {
        return SampleCount::x8;
    }
    if (msaa_sample_count >= 4) {
        return SampleCount::x4;
    }
    if (msaa_sample_count >= 2) {
        return SampleCount::x2;
    }
    return SampleCount::x1;
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUFactory::SDL_GPUFactory(uint32_t msaa_sample_count)
    : requested_msaa_sample_count{to_sample_count(msaa_sample_count)} {
    Expects(TTF_Init());
}

SDL_GPUFactory::~SDL_GPUFactory() {
    fonts_by_id.clear();
    TTF_Quit();
}

auto SDL_GPUFactory::create_renderer(Window& window)
    -> std::unique_ptr<SDL_GPURenderer> {
    auto* sdl_window = static_cast<SDL_Window*>(window.get_window_handle());
    gpu_device = std::make_shared<GPUDevice>(sdl_window);
    return std::make_unique<SDL_GPURenderer>(
        window, shared_from_this(), gpu_device, requested_msaa_sample_count
    );
}

auto SDL_GPUFactory::get_gpu_device() const -> std::shared_ptr<GPUDevice> {
    return gpu_device;
}

auto SDL_GPUFactory::create_mesh(
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
) -> RenderableId {
    Expects(gpu_device != nullptr);

    const auto renderable_id = this->renderable_manager.allocate_id();

    auto meshes = std::vector<SDL_GPUMesh>{};

    auto command_buffer = gpu_device->create_command_buffer();
    auto vertex_buffer = std::optional<Buffer>{};
    auto index_buffer = std::optional<Buffer>{};
    {
        auto copy_pass = command_buffer.begin_copy_pass();

        vertex_buffer = gpu_device->create_buffer(BufferInfo{
            .usage = BufferUsage::Vertex,
            .size = static_cast<uint32_t>(vertices.size_bytes()),
        });
        index_buffer = gpu_device->create_buffer(BufferInfo{
            .usage = BufferUsage::Index,
            .size = static_cast<uint32_t>(indices.size_bytes()),
        });

        auto vertex_transfer_buffer =
            gpu_device->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = static_cast<uint32_t>(vertices.size_bytes()),
            });
        {
            const auto mapped = vertex_transfer_buffer.map(false);
            std::memcpy(
                mapped.data(), vertices.data(), vertices.size_bytes()
            );
        }
        vertex_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            vertex_transfer_buffer, 0, *vertex_buffer, 0,
            static_cast<uint32_t>(vertices.size_bytes()), false
        );

        auto index_transfer_buffer =
            gpu_device->create_transfer_buffer(TransferBufferInfo{
                .usage = TransferBufferUsage::Upload,
                .size = static_cast<uint32_t>(indices.size_bytes()),
            });
        {
            const auto mapped = index_transfer_buffer.map(false);
            std::memcpy(mapped.data(), indices.data(), indices.size_bytes());
        }
        index_transfer_buffer.unmap();
        copy_pass.upload_to_buffer(
            index_transfer_buffer, 0, *index_buffer, 0,
            static_cast<uint32_t>(indices.size_bytes()), false
        );

        const auto local_bounds = compute_mesh_local_bounds(vertices);

        // Procedural meshes aren't simplified (no meshoptimizer LOD
        // generation for caller-supplied raw vertex data) - every LOD level
        // points at the same single full-detail range.
        auto lod_ranges = std::array<LodRange, max_lod_levels>{};
        lod_ranges.fill(LodRange{
            .first_index = 0U,
            .index_count = static_cast<uint32_t>(indices.size()),
        });

        meshes.emplace_back(
            *gpu_device,
            copy_pass,
            lod_ranges,
            0,
            local_bounds,
            texture_paths
        );
    }

    for (const auto& mesh : meshes) {
        mesh.generate_mipmaps(command_buffer);
    }

    command_buffer.submit();

    if (renderable_id >= this->meshes_by_id.size()) {
        this->meshes_by_id.resize(renderable_id + 1);
    }
    this->meshes_by_id[renderable_id] = RenderableMeshes{
        .vertex_buffer = std::move(vertex_buffer).value(),
        .index_buffer = std::move(index_buffer).value(),
        .meshes = std::move(meshes),
    };

    return renderable_id;
}

auto SDL_GPUFactory::create_model(const std::filesystem::path& model_path)
    -> RenderableId {
    Expects(gpu_device != nullptr);

    const auto renderable_id = this->renderable_manager.allocate_id(model_path);

    if (renderable_id < this->meshes_by_id.size() &&
        this->meshes_by_id[renderable_id].has_value()) {
        return renderable_id;
    }

    if (renderable_id >= this->meshes_by_id.size()) {
        this->meshes_by_id.resize(renderable_id + 1);
    }
    this->meshes_by_id[renderable_id] =
        load_meshes_from_model(*gpu_device, model_path);

    return renderable_id;
}

auto SDL_GPUFactory::remove_renderable(RenderableId renderable_id) -> void {
    if (renderable_id < this->meshes_by_id.size()) {
        this->meshes_by_id[renderable_id].reset();
    }
    this->renderable_manager.remove_renderable(renderable_id);
}

auto SDL_GPUFactory::get_meshes(RenderableId renderable_id) const
    -> gsl::span<const SDL_GPUMesh> {
    return gsl::at(this->meshes_by_id, renderable_id).value().meshes;
}

auto SDL_GPUFactory::get_vertex_buffer(RenderableId renderable_id) const
    -> const Buffer& {
    return gsl::at(this->meshes_by_id, renderable_id).value().vertex_buffer;
}

auto SDL_GPUFactory::get_index_buffer(RenderableId renderable_id) const
    -> const Buffer& {
    return gsl::at(this->meshes_by_id, renderable_id).value().index_buffer;
}

auto SDL_GPUFactory::create_font(
    const std::filesystem::path& font_path, float point_size
) -> FontId {
    const auto font_id = this->font_manager.allocate_id(font_path);

    if (this->fonts_by_id.contains(font_id)) {
        return font_id;
    }

    auto command_buffer = gpu_device->create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        this->fonts_by_id.emplace(
            font_id,
            SDL_GPUFont{*gpu_device, copy_pass, font_path, point_size}
        );
    }
    command_buffer.submit();

    return font_id;
}

auto SDL_GPUFactory::get_font(FontId font_id) const -> const SDL_GPUFont& {
    return this->fonts_by_id.at(font_id);
}

}  // namespace Luminol::Graphics::SDL_GPU
