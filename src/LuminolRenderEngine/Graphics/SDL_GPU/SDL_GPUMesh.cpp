#include "SDL_GPUMesh.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <optional>
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
        .alpha_mode = mesh_data.alpha_mode,
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
        .enable_mipmap_filtering = true,
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
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm,
    bool generate_mipmaps = false
) -> Texture {
    const auto size_bytes = width * height * 4U;

    auto texture = device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = format,
        .generate_mipmaps = generate_mipmaps,
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
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm,
    bool generate_mipmaps = false
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
        device,
        copy_pass,
        width,
        height,
        image.data.data(),
        format,
        generate_mipmaps
    );
}

auto create_texture_from_image(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::optional<Luminol::Utilities::ImageLoader::Image>& texture_image,
    Texture (*default_texture)(GPUDevice&, CopyPass&),
    TextureFormat format = TextureFormat::R8G8B8A8_Unorm,
    bool generate_mipmaps = false
) -> Texture {
    if (!texture_image.has_value()) {
        return default_texture(device, copy_pass);
    }

    const auto& image = texture_image.value();

    const auto width = static_cast<uint32_t>(image.width);
    const auto height = static_cast<uint32_t>(image.height);

    return create_uploaded_texture(
        device,
        copy_pass,
        width,
        height,
        image.data.data(),
        format,
        generate_mipmaps
    );
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

constexpr auto vertex_stride_in_floats = 11U;

auto compute_mesh_local_bounds(gsl::span<const float> vertices) -> BoundingBox {
    const auto vertex_count = vertices.size() / vertex_stride_in_floats;
    if (vertex_count == 0) {
        return BoundingBox{};
    }

    const auto first_position = Luminol::Maths::Vector3f{
        vertices[0], vertices[1], vertices[2]
    };
    auto min = first_position;
    auto max = first_position;

    for (auto i = size_t{1}; i < vertex_count; ++i) {
        const auto offset = i * vertex_stride_in_floats;
        const auto position = Luminol::Maths::Vector3f{
            vertices[offset], vertices[offset + 1], vertices[offset + 2]
        };

        min = Luminol::Maths::Vector3f{
            std::min(min.x(), position.x()),
            std::min(min.y(), position.y()),
            std::min(min.z(), position.z()),
        };
        max = Luminol::Maths::Vector3f{
            std::max(max.x(), position.x()),
            std::max(max.y(), position.y()),
            std::max(max.z(), position.z()),
        };
    }

    return BoundingBox{.min = min, .max = max};
}

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    CopyPass& copy_pass,
    uint32_t first_index,
    uint32_t index_count,
    int32_t vertex_offset,
    const BoundingBox& local_bounds,
    const TexturePaths& texture_paths
)
    : first_index{first_index},
      index_count{index_count},
      vertex_offset{vertex_offset},
      local_bounds{local_bounds},
      diffuse_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.diffuse_texture_path,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm_Srgb,
          true
      )},
      normal_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.normal_texture_path,
          create_flat_normal_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      metallic_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.metallic_texture_path,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      roughness_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.roughness_texture_path,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      ambient_occlusion_texture{create_texture_from_path(
          device,
          copy_pass,
          texture_paths.ambient_occlusion_texture_path,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      diffuse_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
          .enable_mipmap_filtering = true,
      })},
      normal_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
          .enable_mipmap_filtering = true,
      })},
      metallic_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
          .enable_mipmap_filtering = true,
      })},
      roughness_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
          .enable_mipmap_filtering = true,
      })},
      ambient_occlusion_sampler{device.create_sampler(SamplerInfo{
          .address_mode_u = SamplerAddressMode::Repeat,
          .address_mode_v = SamplerAddressMode::Repeat,
          .enable_mipmap_filtering = true,
      })},
      mesh_alpha_mode{
          texture_paths.is_transparent
              ? Utilities::ModelLoader::AlphaMode::Blend
              : Utilities::ModelLoader::AlphaMode::Opaque
      } {}

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    CopyPass& copy_pass,
    uint32_t first_index,
    uint32_t index_count,
    int32_t vertex_offset,
    const BoundingBox& local_bounds,
    const TextureImages& texture_images
)
    : first_index{first_index},
      index_count{index_count},
      vertex_offset{vertex_offset},
      local_bounds{local_bounds},
      diffuse_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.diffuse_texture,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm_Srgb,
          true
      )},
      normal_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.normal_texture,
          create_flat_normal_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      metallic_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.metallic_texture,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      roughness_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.roughness_texture,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
      )},
      ambient_occlusion_texture{create_texture_from_image(
          device,
          copy_pass,
          texture_images.ambient_occlusion_texture,
          create_white_pixel_texture,
          TextureFormat::R8G8B8A8_Unorm,
          true
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
      )},
      mesh_alpha_mode{texture_images.alpha_mode} {}

auto SDL_GPUMesh::get_local_bounds() const -> const Luminol::Graphics::BoundingBox& {
    return local_bounds;
}

auto SDL_GPUMesh::draw(RenderPass& sdl_gpu_pass) const -> void {
    draw_instanced(1, sdl_gpu_pass);
}

auto SDL_GPUMesh::draw_instanced(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
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
        index_count, static_cast<uint32_t>(instance_count), first_index,
        vertex_offset
    );
}

auto SDL_GPUMesh::draw_instanced_geometry_only(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
    sdl_gpu_pass.draw_indexed_primitives(
        index_count, static_cast<uint32_t>(instance_count), first_index,
        vertex_offset
    );
}

auto SDL_GPUMesh::draw_indirect(
    RenderPass& sdl_gpu_pass,
    const Buffer& indirect_buffer,
    uint32_t byte_offset
) const -> void {
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

    sdl_gpu_pass.draw_indexed_primitives_indirect(indirect_buffer, byte_offset, 1);
}

auto SDL_GPUMesh::draw_indirect_geometry_only(
    RenderPass& sdl_gpu_pass,
    const Buffer& indirect_buffer,
    uint32_t byte_offset
) const -> void {
    sdl_gpu_pass.draw_indexed_primitives_indirect(indirect_buffer, byte_offset, 1);
}

auto SDL_GPUMesh::alpha_mode() const -> Utilities::ModelLoader::AlphaMode {
    return mesh_alpha_mode;
}

auto SDL_GPUMesh::get_first_index() const -> uint32_t { return first_index; }

auto SDL_GPUMesh::get_index_count() const -> uint32_t { return index_count; }

auto SDL_GPUMesh::get_vertex_offset() const -> int32_t {
    return vertex_offset;
}

auto SDL_GPUMesh::generate_mipmaps(CommandBuffer& command_buffer) const
    -> void {
    const auto textures = std::array<const Texture*, 5>{
        &diffuse_texture,
        &normal_texture,
        &metallic_texture,
        &roughness_texture,
        &ambient_occlusion_texture,
    };

    for (const auto* texture : textures) {
        // Default 1x1 fallback textures (used when a material has no path
        // for this slot) only have 1 mip level; SDL asserts if asked to
        // generate mipmaps for those.
        if (texture->get_mip_levels() > 1) {
            command_buffer.generate_mipmaps(*texture);
        }
    }
}

auto load_meshes_from_model(
    GPUDevice& device, const std::filesystem::path& model_path
) -> RenderableMeshes {
    const auto model_data_opt =
        Luminol::Utilities::ModelLoader::load_model(model_path);

    Expects(model_data_opt.has_value());

    const auto& model_data = model_data_opt.value();

    constexpr auto vertex_components = 11;

    // Build one shared vertex/index buffer covering every submesh, so all of
    // a renderable's submeshes can be drawn against a single bound
    // vertex/index buffer (required for indirect multi-draw batching, see
    // SDL_GPUPointSpotShadowPass). Each submesh's indices in ModelLoader
    // output are already local/0-based, so they're concatenated as-is; only
    // first_index/vertex_offset need to track the running totals.
    struct SubmeshInfo {
        uint32_t first_index;
        uint32_t index_count;
        int32_t vertex_offset;
        BoundingBox local_bounds;
        Utilities::ModelLoader::MeshData const* mesh_data;
    };

    auto combined_vertices = std::vector<float>{};
    auto combined_indices = std::vector<uint32_t>{};
    auto submesh_infos = std::vector<SubmeshInfo>{};
    submesh_infos.reserve(model_data.meshes.size());

    auto running_vertex_offset = int32_t{0};
    auto running_first_index = uint32_t{0};

    for (const auto& mesh_data : model_data.meshes) {
        Expects(
            mesh_data.vertices.size() == mesh_data.texture_coordinates.size()
        );

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

        submesh_infos.push_back(SubmeshInfo{
            .first_index = running_first_index,
            .index_count = static_cast<uint32_t>(mesh_data.indices.size()),
            .vertex_offset = running_vertex_offset,
            .local_bounds = compute_mesh_local_bounds(mesh_vertices),
            .mesh_data = &mesh_data,
        });

        combined_vertices.insert(
            combined_vertices.end(), mesh_vertices.begin(), mesh_vertices.end()
        );
        combined_indices.insert(
            combined_indices.end(), mesh_data.indices.begin(),
            mesh_data.indices.end()
        );

        running_vertex_offset += static_cast<int32_t>(mesh_data.vertices.size());
        running_first_index += static_cast<uint32_t>(mesh_data.indices.size());
    }

    auto meshes = std::vector<SDL_GPUMesh>{};
    meshes.reserve(submesh_infos.size());

    auto command_buffer = device.create_command_buffer();
    auto vertex_buffer = std::optional<Buffer>{};
    auto index_buffer = std::optional<Buffer>{};
    {
        auto copy_pass = command_buffer.begin_copy_pass();

        vertex_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_vertices.data(),
            static_cast<uint32_t>(
                combined_vertices.size() * sizeof(float)
            ),
            BufferUsage::Vertex
        );
        index_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_indices.data(),
            static_cast<uint32_t>(
                combined_indices.size() * sizeof(uint32_t)
            ),
            BufferUsage::Index
        );

        for (const auto& info : submesh_infos) {
            const auto texture_images =
                to_texture_images(*info.mesh_data, model_data.textures_map);

            meshes.emplace_back(
                device,
                copy_pass,
                info.first_index,
                info.index_count,
                info.vertex_offset,
                info.local_bounds,
                texture_images
            );
        }
    }

    for (const auto& mesh : meshes) {
        mesh.generate_mipmaps(command_buffer);
    }

    command_buffer.submit();

    return RenderableMeshes{
        .vertex_buffer = std::move(vertex_buffer).value(),
        .index_buffer = std::move(index_buffer).value(),
        .meshes = std::move(meshes),
    };
}

}  // namespace Luminol::Graphics::SDL_GPU
