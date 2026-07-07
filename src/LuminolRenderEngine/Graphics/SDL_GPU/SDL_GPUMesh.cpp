#include "SDL_GPUMesh.hpp"

#include <array>
#include <cstring>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>
#include <LuminolRenderEngine/Utilities/ModelLoader.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

auto load_first_texture_or_nothing(
    const std::vector<std::filesystem::path>& texture_paths,
    const std::unordered_map<
        std::filesystem::path,
        Luminol::Utilities::ImageLoader::Image>& textures_map
) -> std::optional<Luminol::Utilities::ImageLoader::Image> {
    if (texture_paths.empty()) {
        return std::nullopt;
    }

    return textures_map.at(texture_paths[0]);
}

auto create_uploaded_buffer(
    GPUDevice& device,
    const void* source_data,
    uint32_t size_bytes,
    BufferUsage usage
) -> Buffer {
    auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = size_bytes,
    });

    const auto mapped = transfer_buffer.map(false);
    std::memcpy(mapped.data(), source_data, size_bytes);
    transfer_buffer.unmap();

    auto device_buffer = device.create_buffer(BufferInfo{
        .usage = usage,
        .size = size_bytes,
    });

    auto command_buffer = device.create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.upload_to_buffer(
            transfer_buffer, 0, device_buffer, 0, size_bytes, false
        );
    }
    command_buffer.submit();

    return device_buffer;
}

auto create_uploaded_texture(
    GPUDevice& device, uint32_t width, uint32_t height, const uint8_t* rgba_pixels
) -> Texture {
    const auto size_bytes = width * height * 4U;

    auto texture = device.create_texture(TextureInfo{
        .width = width,
        .height = height,
    });

    auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = size_bytes,
    });

    const auto mapped = transfer_buffer.map(false);
    std::memcpy(mapped.data(), rgba_pixels, size_bytes);
    transfer_buffer.unmap();

    auto command_buffer = device.create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.upload_to_texture(
            transfer_buffer, 0, texture, width, height, false
        );
    }
    command_buffer.submit();

    return texture;
}

constexpr auto desired_rgba_channels = int32_t{4};

auto create_white_pixel_texture(GPUDevice& device) -> Texture {
    constexpr auto white_pixel =
        std::array<uint8_t, 4>{0xFF, 0xFF, 0xFF, 0xFF};
    return create_uploaded_texture(device, 1, 1, white_pixel.data());
}

auto create_diffuse_texture(
    GPUDevice& device, const Luminol::Graphics::TexturePaths& texture_paths
) -> Texture {
    if (!texture_paths.diffuse_texture_path.has_value()) {
        return create_white_pixel_texture(device);
    }

    const auto image = Luminol::Utilities::ImageLoader::load_image(
        texture_paths.diffuse_texture_path.value(), desired_rgba_channels
    );

    const auto width = static_cast<uint32_t>(image.width);
    const auto height = static_cast<uint32_t>(image.height);

    return create_uploaded_texture(device, width, height, image.data.data());
}

auto create_diffuse_texture(
    GPUDevice& device,
    const std::optional<Luminol::Utilities::ImageLoader::Image>&
        diffuse_texture_image
) -> Texture {
    if (!diffuse_texture_image.has_value()) {
        return create_white_pixel_texture(device);
    }

    const auto& image = diffuse_texture_image.value();

    const auto width = static_cast<uint32_t>(image.width);
    const auto height = static_cast<uint32_t>(image.height);

    return create_uploaded_texture(device, width, height, image.data.data());
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
)
    : vertex_buffer{create_uploaded_buffer(
          device,
          vertices.data(),
          static_cast<uint32_t>(vertices.size_bytes()),
          BufferUsage::Vertex
      )},
      index_buffer{create_uploaded_buffer(
          device,
          indices.data(),
          static_cast<uint32_t>(indices.size_bytes()),
          BufferUsage::Index
      )},
      index_count{static_cast<uint32_t>(indices.size())},
      texture{create_diffuse_texture(device, texture_paths)},
      sampler{device.create_sampler(SamplerInfo{})} {}

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const std::optional<Luminol::Utilities::ImageLoader::Image>&
        diffuse_texture_image
)
    : vertex_buffer{create_uploaded_buffer(
          device,
          vertices.data(),
          static_cast<uint32_t>(vertices.size_bytes()),
          BufferUsage::Vertex
      )},
      index_buffer{create_uploaded_buffer(
          device,
          indices.data(),
          static_cast<uint32_t>(indices.size_bytes()),
          BufferUsage::Index
      )},
      index_count{static_cast<uint32_t>(indices.size())},
      texture{create_diffuse_texture(device, diffuse_texture_image)},
      sampler{device.create_sampler(SamplerInfo{})} {}

auto SDL_GPUMesh::draw(RenderPass& sdl_gpu_pass) const -> void {
    draw_instanced(1, sdl_gpu_pass);
}

auto SDL_GPUMesh::draw_instanced(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
    const auto vertex_bindings = std::array{VertexBufferBinding{
        .buffer = &vertex_buffer,
        .offset = 0,
    }};
    sdl_gpu_pass.bind_vertex_buffers(0, vertex_bindings);
    sdl_gpu_pass.bind_index_buffer(index_buffer, IndexElementSize::Bits32, 0);

    const auto sampler_bindings = std::array{
        TextureSamplerBinding{.texture = &texture, .sampler = &sampler}
    };
    sdl_gpu_pass.bind_fragment_samplers(0, sampler_bindings);

    sdl_gpu_pass.draw_indexed_primitives(
        index_count, static_cast<uint32_t>(instance_count)
    );
}

auto load_meshes_from_model(
    GPUDevice& device, const std::filesystem::path& model_path
) -> std::vector<SDL_GPUMesh> {
    const auto model_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(model_data_opt.has_value());

    const auto& model_data = model_data_opt.value();

    auto meshes = std::vector<SDL_GPUMesh>{};
    meshes.reserve(model_data.meshes.size());

    for (const auto& mesh_data : model_data.meshes) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

        constexpr auto vertex_components = 11;

        auto mesh_vertices = std::vector<float>{};
        mesh_vertices.reserve(mesh_data.vertices.size() * vertex_components);

        for (size_t i = 0; i < mesh_data.vertices.size(); ++i) {
            mesh_vertices.push_back(mesh_data.vertices[i].x());
            mesh_vertices.push_back(mesh_data.vertices[i].y());
            mesh_vertices.push_back(mesh_data.vertices[i].z());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].x());
            mesh_vertices.push_back(mesh_data.texture_coordinates[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].x());
            mesh_vertices.push_back(mesh_data.normals[i].y());
            mesh_vertices.push_back(mesh_data.normals[i].z());
            mesh_vertices.push_back(mesh_data.tangents[i].x());
            mesh_vertices.push_back(mesh_data.tangents[i].y());
            mesh_vertices.push_back(mesh_data.tangents[i].z());
        }

        const auto diffuse_texture_image = load_first_texture_or_nothing(
            mesh_data.diffuse_texture_paths, model_data.textures_map
        );

        meshes.emplace_back(
            device, mesh_vertices, mesh_data.indices, diffuse_texture_image
        );
    }

    return meshes;
}

}  // namespace Luminol::Graphics::SDL_GPU
