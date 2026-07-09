#include "SDL_GPUMesh.hpp"

#include <array>
#include <cstring>
#include <stdexcept>

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

auto to_texture_images(
    const Luminol::Utilities::ModelLoader::MeshData& mesh_data,
    const std::unordered_map<
        std::filesystem::path,
        Luminol::Utilities::ImageLoader::Image>& textures_map
) -> TextureImages {
    return TextureImages{
        .diffuse_texture = load_first_texture_or_nothing(
            mesh_data.diffuse_texture_paths, textures_map
        ),
        .diffuse_texture_wrap = mesh_data.diffuse_texture_wrap,
        .normal_texture = load_first_texture_or_nothing(
            mesh_data.normal_texture_paths, textures_map
        ),
        .normal_texture_wrap = mesh_data.normal_texture_wrap,
        .metallic_texture = load_first_texture_or_nothing(
            mesh_data.metallic_texture_paths, textures_map
        ),
        .metallic_texture_wrap = mesh_data.metallic_texture_wrap,
        .roughness_texture = load_first_texture_or_nothing(
            mesh_data.roughness_texture_paths, textures_map
        ),
        .roughness_texture_wrap = mesh_data.roughness_texture_wrap,
        .ambient_occlusion_texture = load_first_texture_or_nothing(
            mesh_data.ambient_occlusion_texture_paths, textures_map
        ),
        .ambient_occlusion_texture_wrap =
            mesh_data.ambient_occlusion_texture_wrap,
    };
}

auto to_sdl_gpu_address_mode(Luminol::Utilities::ModelLoader::TextureWrapMode mode)
    -> SamplerAddressMode {
    switch (mode) {
        case Luminol::Utilities::ModelLoader::TextureWrapMode::Repeat:
            return SamplerAddressMode::Repeat;
        case Luminol::Utilities::ModelLoader::TextureWrapMode::ClampToEdge:
            return SamplerAddressMode::ClampToEdge;
        case Luminol::Utilities::ModelLoader::TextureWrapMode::MirroredRepeat:
            return SamplerAddressMode::MirroredRepeat;
    }
    throw std::runtime_error{"Invalid texture wrap mode"};
}

auto make_sampler(
    GPUDevice& device, const Luminol::Utilities::ModelLoader::TextureWrap& wrap
) -> Sampler {
    return device.create_sampler(SamplerInfo{
        .address_mode_u = to_sdl_gpu_address_mode(wrap.u),
        .address_mode_v = to_sdl_gpu_address_mode(wrap.v),
    });
}

auto create_uploaded_buffer(
    GPUDevice& device,
    CopyPass& copy_pass,
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

    copy_pass.upload_to_buffer(
        transfer_buffer, 0, device_buffer, 0, size_bytes, false
    );

    return device_buffer;
}

auto create_uploaded_texture(
    GPUDevice& device,
    CopyPass& copy_pass,
    uint32_t width,
    uint32_t height,
    const uint8_t* rgba_pixels,
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm
) -> Texture {
    const auto size_bytes = width * height * 4U;

    auto texture = device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = format,
    });

    auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = size_bytes,
    });

    const auto mapped = transfer_buffer.map(false);
    std::memcpy(mapped.data(), rgba_pixels, size_bytes);
    transfer_buffer.unmap();

    copy_pass.upload_to_texture(
        transfer_buffer, 0, texture, width, height, false
    );

    return texture;
}

constexpr auto desired_rgba_channels = int32_t{4};

auto create_white_pixel_texture(GPUDevice& device, CopyPass& copy_pass)
    -> Texture {
    constexpr auto white_pixel =
        std::array<uint8_t, 4>{0xFF, 0xFF, 0xFF, 0xFF};
    return create_uploaded_texture(device, copy_pass, 1, 1, white_pixel.data());
}

auto create_flat_normal_texture(GPUDevice& device, CopyPass& copy_pass)
    -> Texture {
    constexpr auto flat_normal_pixel =
        std::array<uint8_t, 4>{0x80, 0x80, 0xFF, 0xFF};
    return create_uploaded_texture(
        device, copy_pass, 1, 1, flat_normal_pixel.data()
    );
}

auto create_texture_from_path(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::optional<std::filesystem::path>& texture_path,
    Texture (*default_texture)(GPUDevice&, CopyPass&),
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm
) -> Texture {
    if (!texture_path.has_value()) {
        return default_texture(device, copy_pass);
    }

    const auto image = Luminol::Utilities::ImageLoader::load_image(
        texture_path.value(), desired_rgba_channels
    );

    const auto width = static_cast<uint32_t>(image.width);
    const auto height = static_cast<uint32_t>(image.height);

    return create_uploaded_texture(
        device, copy_pass, width, height, image.data.data(), format
    );
}

auto create_texture_from_image(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::optional<Luminol::Utilities::ImageLoader::Image>& texture_image,
    Texture (*default_texture)(GPUDevice&, CopyPass&),
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm
) -> Texture {
    if (!texture_image.has_value()) {
        return default_texture(device, copy_pass);
    }

    const auto& image = texture_image.value();

    const auto width = static_cast<uint32_t>(image.width);
    const auto height = static_cast<uint32_t>(image.height);

    return create_uploaded_texture(
        device, copy_pass, width, height, image.data.data(), format
    );
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    CopyPass& copy_pass,
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TexturePaths& texture_paths
)
    : vertex_buffer{create_uploaded_buffer(
          device,
          copy_pass,
          vertices.data(),
          static_cast<uint32_t>(vertices.size_bytes()),
          BufferUsage::Vertex
      )},
      index_buffer{create_uploaded_buffer(
          device,
          copy_pass,
          indices.data(),
          static_cast<uint32_t>(indices.size_bytes()),
          BufferUsage::Index
      )},
      index_count{static_cast<uint32_t>(indices.size())},
      diffuse_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.diffuse_texture_path,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm_Srgb
      )},
      normal_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.normal_texture_path,
          create_flat_normal_texture
      )},
      metallic_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.metallic_texture_path,
          create_white_pixel_texture
      )},
      roughness_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.roughness_texture_path,
          create_white_pixel_texture
      )},
      ambient_occlusion_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.ambient_occlusion_texture_path,
          create_white_pixel_texture
      )},
      diffuse_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
      })},
      normal_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
      })},
      metallic_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
      })},
      roughness_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
      })},
      ambient_occlusion_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
      })} {}

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    CopyPass& copy_pass,
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices,
    const TextureImages& texture_images
)
    : vertex_buffer{create_uploaded_buffer(
          device,
          copy_pass,
          vertices.data(),
          static_cast<uint32_t>(vertices.size_bytes()),
          BufferUsage::Vertex
      )},
      index_buffer{create_uploaded_buffer(
          device,
          copy_pass,
          indices.data(),
          static_cast<uint32_t>(indices.size_bytes()),
          BufferUsage::Index
      )},
      index_count{static_cast<uint32_t>(indices.size())},
      diffuse_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.diffuse_texture,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm_Srgb
      )},
      normal_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.normal_texture,
          create_flat_normal_texture
      )},
      metallic_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.metallic_texture,
          create_white_pixel_texture
      )},
      roughness_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.roughness_texture,
          create_white_pixel_texture
      )},
      ambient_occlusion_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.ambient_occlusion_texture,
          create_white_pixel_texture
      )},
      diffuse_sampler{make_sampler(device, texture_images.diffuse_texture_wrap)},
      normal_sampler{make_sampler(device, texture_images.normal_texture_wrap)},
      metallic_sampler{
          make_sampler(device, texture_images.metallic_texture_wrap)
      },
      roughness_sampler{
          make_sampler(device, texture_images.roughness_texture_wrap)
      },
      ambient_occlusion_sampler{make_sampler(
          device, texture_images.ambient_occlusion_texture_wrap
      )} {}

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
        TextureSamplerBinding{
            .texture = &diffuse_texture, .sampler = &diffuse_sampler
        },
        TextureSamplerBinding{
            .texture = &normal_texture, .sampler = &normal_sampler
        },
        TextureSamplerBinding{
            .texture = &metallic_texture, .sampler = &metallic_sampler
        },
        TextureSamplerBinding{
            .texture = &roughness_texture, .sampler = &roughness_sampler
        },
        TextureSamplerBinding{
            .texture = &ambient_occlusion_texture,
            .sampler = &ambient_occlusion_sampler
        },
    };
    sdl_gpu_pass.bind_fragment_samplers(0, sampler_bindings);

    sdl_gpu_pass.draw_indexed_primitives(
        index_count, static_cast<uint32_t>(instance_count)
    );
}

auto SDL_GPUMesh::draw_instanced_geometry_only(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
    const auto vertex_bindings = std::array{VertexBufferBinding{
        .buffer = &vertex_buffer,
        .offset = 0,
    }};
    sdl_gpu_pass.bind_vertex_buffers(0, vertex_bindings);
    sdl_gpu_pass.bind_index_buffer(index_buffer, IndexElementSize::Bits32, 0);

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

    auto command_buffer = device.create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();

        for (const auto& mesh_data : model_data.meshes) {
            Expects(
                mesh_data.vertices.size() ==
                mesh_data.texture_coordinates.size()
            );

            constexpr auto vertex_components = 11;

            auto mesh_vertices = std::vector<float>{};
            mesh_vertices.reserve(
                mesh_data.vertices.size() * vertex_components
            );

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

            const auto texture_images =
                to_texture_images(mesh_data, model_data.textures_map);

            meshes.emplace_back(
                device,
                copy_pass,
                mesh_vertices,
                mesh_data.indices,
                texture_images
            );
        }
    }
    command_buffer.submit();

    return meshes;
}

}  // namespace Luminol::Graphics::SDL_GPU
