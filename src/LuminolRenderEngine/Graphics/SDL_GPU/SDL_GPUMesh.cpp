#include "SDL_GPUMesh.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <optional>
#include <stdexcept>

#include <gsl/gsl>
#include <meshoptimizer.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/RenderPasses/SDL_GPURenderPass.hpp>
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

// True when both paths lists are non-empty and their first entry is the same
// file - i.e. this material slot shares a source image with another slot
// (e.g. glTF's packed occlusion/roughness/metallic convention).
auto shares_first_path(
    const std::vector<std::filesystem::path>& lhs_paths,
    const std::vector<std::filesystem::path>& rhs_paths
) -> bool {
    return !lhs_paths.empty() && !rhs_paths.empty() && lhs_paths[0] == rhs_paths[0];
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
        .roughness_shares_metallic_source = shares_first_path(
            mesh_data.roughness_texture_paths, mesh_data.metallic_texture_paths
        ),
        .ambient_occlusion_shares_metallic_source = shares_first_path(
            mesh_data.ambient_occlusion_texture_paths,
            mesh_data.metallic_texture_paths
        ),
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

// ~64 triangles/vertices per meshlet: within meshoptimizer's implementation
// limits (<=256 vertices, <=512 triangles) and matches the engine's
// existing compute thread-group size (see threads_per_group in
// SDL_GPUInstanceCullPass.cpp / SDL_GPUMeshletCullPass), which is also why
// it's the conventional choice for GPU-driven meshlet renderers.
constexpr auto meshlet_max_vertices = std::size_t{64};
constexpr auto meshlet_max_triangles = std::size_t{64};
// 0 = no cone-based backface culling in this first cut (see
// meshlet_cull.hlsl's bounding-sphere-only test) - cone data is still
// computed and stored (GpuMeshletMetadata has room via bounds_center /
// bounds_radius only, cone fields are not persisted) since
// meshopt_computeMeshletBounds always returns them; simply unused for now.
constexpr auto meshlet_cone_weight = 0.0F;

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

constexpr auto vertex_stride_in_floats = 11U;

auto build_meshlets(
    gsl::span<const uint32_t> indices,
    gsl::span<const float> vertex_positions,
    std::size_t vertex_count,
    std::size_t vertex_positions_stride,
    uint32_t submesh_vertex_offset,
    std::vector<GpuMeshletMetadata>& combined_meshlets,
    std::vector<uint32_t>& combined_meshlet_vertices,
    std::vector<uint32_t>& combined_meshlet_triangles
) -> MeshletRange {
    const auto range_start = static_cast<uint32_t>(combined_meshlets.size());

    if (indices.empty()) {
        return MeshletRange{.first_meshlet = range_start, .meshlet_count = 0U};
    }

    const auto max_meshlets = meshopt_buildMeshletsBound(
        indices.size(), meshlet_max_vertices, meshlet_max_triangles
    );

    auto raw_meshlets = std::vector<meshopt_Meshlet>(max_meshlets);
    auto raw_meshlet_vertices =
        std::vector<uint32_t>(max_meshlets * meshlet_max_vertices);
    auto raw_meshlet_triangles =
        std::vector<uint8_t>(max_meshlets * meshlet_max_triangles * 3U);

    const auto meshlet_count = meshopt_buildMeshlets(
        raw_meshlets.data(),
        raw_meshlet_vertices.data(),
        raw_meshlet_triangles.data(),
        indices.data(),
        indices.size(),
        vertex_positions.data(),
        vertex_count,
        vertex_positions_stride,
        meshlet_max_vertices,
        meshlet_max_triangles,
        meshlet_cone_weight
    );
    raw_meshlets.resize(meshlet_count);

    for (const auto& meshlet : raw_meshlets) {
        const auto bounds = meshopt_computeMeshletBounds(
            &raw_meshlet_vertices[meshlet.vertex_offset],
            &raw_meshlet_triangles[meshlet.triangle_offset],
            meshlet.triangle_count,
            vertex_positions.data(),
            vertex_count,
            vertex_positions_stride
        );

        auto local_min = Luminol::Maths::Vector3f{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max(),
        };
        auto local_max = Luminol::Maths::Vector3f{
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest(),
        };

        for (uint32_t vertex_slot = 0; vertex_slot < meshlet.vertex_count;
             ++vertex_slot) {
            const auto vertex_index =
                raw_meshlet_vertices[meshlet.vertex_offset + vertex_slot];
            const auto float_offset =
                (static_cast<size_t>(vertex_index) * vertex_positions_stride)
                / sizeof(float);
            const auto position = Luminol::Maths::Vector3f{
                vertex_positions[float_offset],
                vertex_positions[float_offset + 1],
                vertex_positions[float_offset + 2],
            };

            local_min = Luminol::Maths::Vector3f{
                std::min(local_min.x(), position.x()),
                std::min(local_min.y(), position.y()),
                std::min(local_min.z(), position.z()),
            };
            local_max = Luminol::Maths::Vector3f{
                std::max(local_max.x(), position.x()),
                std::max(local_max.y(), position.y()),
                std::max(local_max.z(), position.z()),
            };
        }

        combined_meshlets.push_back(GpuMeshletMetadata{
            .vertex_offset =
                static_cast<uint32_t>(combined_meshlet_vertices.size()),
            .triangle_offset =
                static_cast<uint32_t>(combined_meshlet_triangles.size()),
            .vertex_count = meshlet.vertex_count,
            .triangle_count = meshlet.triangle_count,
            .bounds_center =
                {bounds.center[0], bounds.center[1], bounds.center[2]},
            .bounds_radius = bounds.radius,
            .local_bounds_min =
                {local_min.x(), local_min.y(), local_min.z(), 0.0F},
            .local_bounds_max =
                {local_max.x(), local_max.y(), local_max.z(), 0.0F},
        });

        for (auto local_vertex = uint32_t{0}; local_vertex < meshlet.vertex_count;
             ++local_vertex) {
            combined_meshlet_vertices.push_back(
                raw_meshlet_vertices[meshlet.vertex_offset + local_vertex] +
                submesh_vertex_offset
            );
        }

        const auto triangle_index_count = meshlet.triangle_count * 3U;
        for (auto i = uint32_t{0}; i < triangle_index_count; ++i) {
            combined_meshlet_triangles.push_back(static_cast<uint32_t>(
                raw_meshlet_triangles[meshlet.triangle_offset + i]
            ));
        }
    }

    return MeshletRange{
        .first_meshlet = range_start,
        .meshlet_count = static_cast<uint32_t>(meshlet_count),
    };
}

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
    const std::array<LodRange, max_lod_levels>& lod_ranges,
    const std::array<MeshletRange, max_lod_levels>& meshlet_ranges,
    int32_t vertex_offset,
    const BoundingBox& local_bounds,
    const TexturePaths& texture_paths
)
    : lod_ranges{lod_ranges},
      meshlet_ranges{meshlet_ranges},
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
      // Packed ORM (occlusion/roughness/metallic) textures share one source
      // file across these three material slots - reuse the already-uploaded
      // metallic_texture instead of uploading the same pixel data again.
      roughness_texture{
          texture_paths.metallic_texture_path.has_value() &&
                  texture_paths.roughness_texture_path ==
                      texture_paths.metallic_texture_path
              ? metallic_texture
              : create_texture_from_path(
                    device,
                    copy_pass,
                    texture_paths.roughness_texture_path,
                    create_white_pixel_texture,
                    TextureFormat::R8G8B8A8_Unorm,
                    true
                )
      },
      ambient_occlusion_texture{
          texture_paths.metallic_texture_path.has_value() &&
                  texture_paths.ambient_occlusion_texture_path ==
                      texture_paths.metallic_texture_path
              ? metallic_texture
              : create_texture_from_path(
                    device,
                    copy_pass,
                    texture_paths.ambient_occlusion_texture_path,
                    create_white_pixel_texture,
                    TextureFormat::R8G8B8A8_Unorm,
                    true
                )
      },
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
    const std::array<LodRange, max_lod_levels>& lod_ranges,
    const std::array<MeshletRange, max_lod_levels>& meshlet_ranges,
    int32_t vertex_offset,
    const BoundingBox& local_bounds,
    const TextureImages& texture_images
)
    : lod_ranges{lod_ranges},
      meshlet_ranges{meshlet_ranges},
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
      // Packed ORM (occlusion/roughness/metallic) textures share one source
      // file across these three material slots - reuse the already-uploaded
      // metallic_texture instead of uploading the same pixel data again.
      roughness_texture{
          texture_images.roughness_shares_metallic_source
              ? metallic_texture
              : create_texture_from_image(
                    device,
                    copy_pass,
                    texture_images.roughness_texture,
                    create_white_pixel_texture,
                    TextureFormat::R8G8B8A8_Unorm,
                    true
                )
      },
      ambient_occlusion_texture{
          texture_images.ambient_occlusion_shares_metallic_source
              ? metallic_texture
              : create_texture_from_image(
                    device,
                    copy_pass,
                    texture_images.ambient_occlusion_texture,
                    create_white_pixel_texture,
                    TextureFormat::R8G8B8A8_Unorm,
                    true
                )
      },
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
        lod_ranges[0].index_count, static_cast<uint32_t>(instance_count),
        lod_ranges[0].first_index, vertex_offset
    );
}

auto SDL_GPUMesh::draw_instanced_geometry_only(
    int32_t instance_count, RenderPass& sdl_gpu_pass
) const -> void {
    sdl_gpu_pass.draw_indexed_primitives(
        lod_ranges[0].index_count, static_cast<uint32_t>(instance_count),
        lod_ranges[0].first_index, vertex_offset
    );
}

auto SDL_GPUMesh::draw_indirect_geometry_only(
    RenderPass& sdl_gpu_pass,
    const Buffer& indirect_buffer,
    uint32_t byte_offset
) const -> void {
    sdl_gpu_pass.draw_indexed_primitives_indirect(indirect_buffer, byte_offset, 1);
}

auto SDL_GPUMesh::draw_meshlet_indirect(
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

    sdl_gpu_pass.draw_primitives_indirect(indirect_buffer, byte_offset, 1);
}

auto SDL_GPUMesh::alpha_mode() const -> Utilities::ModelLoader::AlphaMode {
    return mesh_alpha_mode;
}

auto SDL_GPUMesh::get_first_index() const -> uint32_t {
    return lod_ranges[0].first_index;
}

auto SDL_GPUMesh::get_index_count() const -> uint32_t {
    return lod_ranges[0].index_count;
}

auto SDL_GPUMesh::get_vertex_offset() const -> int32_t {
    return vertex_offset;
}

auto SDL_GPUMesh::get_lod_range(std::size_t lod_index) const
    -> const LodRange& {
    return lod_ranges.at(lod_index);
}

auto SDL_GPUMesh::get_meshlet_range(std::size_t lod_index) const
    -> const MeshletRange& {
    return meshlet_ranges.at(lod_index);
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

    // Roughness/ambient-occlusion may share metallic_texture's native handle
    // (see TextureImages::roughness_shares_metallic_source) - skip a handle
    // already processed this call so its mip chain isn't generated twice.
    auto processed_handles = std::array<SDL_GPUTexture*, 5>{};
    auto processed_count = std::size_t{0};

    for (const auto* texture : textures) {
        // Default 1x1 fallback textures (used when a material has no path
        // for this slot) only have 1 mip level; SDL asserts if asked to
        // generate mipmaps for those.
        if (texture->get_mip_levels() <= 1) {
            continue;
        }

        auto* const handle = texture->native_handle();
        auto already_processed = false;
        for (auto i = std::size_t{0}; i < processed_count; ++i) {
            if (processed_handles.at(i) == handle) {
                already_processed = true;
                break;
            }
        }
        if (already_processed) {
            continue;
        }

        command_buffer.generate_mipmaps(*texture);
        processed_handles.at(processed_count) = handle;
        ++processed_count;
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
    // first_index/vertex_offset need to track the running totals. Each
    // submesh also gets max_lod_levels-1 additional simplified index ranges
    // appended after its LOD0 range, all referencing the same vertex range
    // (see LodRange).
    struct SubmeshInfo {
        std::array<LodRange, max_lod_levels> lod_ranges;
        std::array<MeshletRange, max_lod_levels> meshlet_ranges;
        int32_t vertex_offset;
        BoundingBox local_bounds;
        Utilities::ModelLoader::MeshData const* mesh_data;
    };

    // Target index-count ratios (relative to LOD0) for LOD1.. onward, and the
    // meshopt_simplify target error tolerated to reach them. Global defaults
    // for now - not configurable per model.
    constexpr auto lod_index_ratios =
        std::array<float, max_lod_levels - 1>{0.5F, 0.25F, 0.1F};
    constexpr auto lod_target_error = 0.02F;
    constexpr auto min_lod_index_count = std::size_t{96};

    auto combined_vertices = std::vector<float>{};
    auto combined_indices = std::vector<uint32_t>{};
    auto combined_meshlets = std::vector<GpuMeshletMetadata>{};
    auto combined_meshlet_vertices = std::vector<uint32_t>{};
    auto combined_meshlet_triangles = std::vector<uint32_t>{};
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

        auto mesh_indices = mesh_data.indices;
        const auto vertex_stride = vertex_components * sizeof(float);

        meshopt_optimizeVertexCache(
            mesh_indices.data(), mesh_indices.data(), mesh_indices.size(),
            mesh_data.vertices.size()
        );
        meshopt_optimizeOverdraw(
            mesh_indices.data(), mesh_indices.data(), mesh_indices.size(),
            mesh_vertices.data(), mesh_data.vertices.size(), vertex_stride,
            1.05F
        );
        const auto new_vertex_count = meshopt_optimizeVertexFetch(
            mesh_vertices.data(), mesh_indices.data(), mesh_indices.size(),
            mesh_vertices.data(), mesh_data.vertices.size(), vertex_stride
        );
        mesh_vertices.resize(new_vertex_count * vertex_components);

        auto lod_ranges = std::array<LodRange, max_lod_levels>{};
        lod_ranges[0] = LodRange{
            .first_index = running_first_index,
            .index_count = static_cast<uint32_t>(mesh_indices.size()),
        };

        auto meshlet_ranges = std::array<MeshletRange, max_lod_levels>{};
        meshlet_ranges[0] = build_meshlets(
            mesh_indices,
            mesh_vertices,
            new_vertex_count,
            vertex_stride,
            static_cast<uint32_t>(running_vertex_offset),
            combined_meshlets,
            combined_meshlet_vertices,
            combined_meshlet_triangles
        );

        combined_indices.insert(
            combined_indices.end(), mesh_indices.begin(), mesh_indices.end()
        );
        running_first_index += static_cast<uint32_t>(mesh_indices.size());

        // Generate coarser LODs by repeatedly simplifying the previous LOD's
        // index buffer. meshopt_simplify only selects a subset of the
        // existing vertices (it never introduces new ones), so every LOD
        // level can keep referencing this submesh's single vertex range.
        auto previous_lod_indices = mesh_indices;
        for (auto lod = std::size_t{1}; lod < max_lod_levels; ++lod) {
            auto target_index_count = std::max(
                min_lod_index_count,
                static_cast<std::size_t>(
                    static_cast<float>(mesh_indices.size()) *
                    lod_index_ratios.at(lod - 1)
                )
            );
            target_index_count -= target_index_count % 3;

            if (target_index_count >= previous_lod_indices.size()) {
                lod_ranges.at(lod) = lod_ranges.at(lod - 1);
                meshlet_ranges.at(lod) = meshlet_ranges.at(lod - 1);
                continue;
            }

            auto simplified_indices =
                std::vector<uint32_t>(previous_lod_indices.size());
            auto result_error = 0.0F;
            const auto simplified_count = meshopt_simplify(
                simplified_indices.data(),
                previous_lod_indices.data(),
                previous_lod_indices.size(),
                mesh_vertices.data(),
                new_vertex_count,
                vertex_stride,
                target_index_count,
                lod_target_error,
                meshopt_SimplifyLockBorder,
                &result_error
            );
            simplified_indices.resize(simplified_count);

            // meshopt_simplify can stop early once its error bound is hit,
            // producing few or no fewer triangles than the previous LOD -
            // reuse the previous (finer) LOD's range rather than storing a
            // degenerate or non-simplified duplicate.
            if (simplified_count < 3 ||
                simplified_count >= previous_lod_indices.size()) {
                lod_ranges.at(lod) = lod_ranges.at(lod - 1);
                meshlet_ranges.at(lod) = meshlet_ranges.at(lod - 1);
                continue;
            }

            meshopt_optimizeVertexCache(
                simplified_indices.data(), simplified_indices.data(),
                simplified_indices.size(), new_vertex_count
            );

            lod_ranges.at(lod) = LodRange{
                .first_index = running_first_index,
                .index_count = static_cast<uint32_t>(simplified_indices.size()),
            };
            meshlet_ranges.at(lod) = build_meshlets(
                simplified_indices,
                mesh_vertices,
                new_vertex_count,
                vertex_stride,
                static_cast<uint32_t>(running_vertex_offset),
                combined_meshlets,
                combined_meshlet_vertices,
                combined_meshlet_triangles
            );
            combined_indices.insert(
                combined_indices.end(), simplified_indices.begin(),
                simplified_indices.end()
            );
            running_first_index +=
                static_cast<uint32_t>(simplified_indices.size());

            previous_lod_indices = std::move(simplified_indices);
        }

        submesh_infos.push_back(SubmeshInfo{
            .lod_ranges = lod_ranges,
            .meshlet_ranges = meshlet_ranges,
            .vertex_offset = running_vertex_offset,
            .local_bounds = compute_mesh_local_bounds(mesh_vertices),
            .mesh_data = &mesh_data,
        });

        combined_vertices.insert(
            combined_vertices.end(), mesh_vertices.begin(), mesh_vertices.end()
        );

        running_vertex_offset += static_cast<int32_t>(new_vertex_count);
    }

    auto meshes = std::vector<SDL_GPUMesh>{};
    meshes.reserve(submesh_infos.size());

    auto command_buffer = device.create_command_buffer();
    auto vertex_buffer = std::optional<Buffer>{};
    auto index_buffer = std::optional<Buffer>{};
    auto meshlet_metadata_buffer = std::optional<Buffer>{};
    auto meshlet_vertices_buffer = std::optional<Buffer>{};
    auto meshlet_triangles_buffer = std::optional<Buffer>{};
    {
        auto copy_pass = command_buffer.begin_copy_pass();

        vertex_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_vertices.data(),
            static_cast<uint32_t>(
                combined_vertices.size() * sizeof(float)
            ),
            // StorageRead (in addition to Vertex): also bindable as a
            // StructuredBuffer<float> for the main pass's meshlet
            // vertex-pull path (pbr_vert_meshlet.hlsl) - purely additive,
            // the existing fixed-function vertex buffer path is unchanged.
            BufferUsage::Vertex | BufferUsage::StorageRead
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
        // StorageRead (not Vertex/Index): these are only ever read via
        // manual StructuredBuffer fetch in meshlet_cull.hlsl /
        // pbr_vert_meshlet.hlsl, never bound as a fixed-function
        // vertex/index buffer - see SDL_GPUMeshletCullPass. meshlet_metadata
        // additionally needs ComputeStorageRead: unlike the vertices/
        // triangles buffers (vertex-stage-only), it's also read by
        // meshlet_cull.hlsl's compute pass for per-meshlet bounds.
        meshlet_metadata_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_meshlets.data(),
            static_cast<uint32_t>(
                combined_meshlets.size() * sizeof(GpuMeshletMetadata)
            ),
            BufferUsage::StorageRead | BufferUsage::ComputeStorageRead
        );
        meshlet_vertices_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_meshlet_vertices.data(),
            static_cast<uint32_t>(
                combined_meshlet_vertices.size() * sizeof(uint32_t)
            ),
            BufferUsage::StorageRead
        );
        meshlet_triangles_buffer = create_uploaded_buffer(
            device,
            copy_pass,
            combined_meshlet_triangles.data(),
            static_cast<uint32_t>(
                combined_meshlet_triangles.size() * sizeof(uint32_t)
            ),
            BufferUsage::StorageRead
        );

        for (const auto& info : submesh_infos) {
            const auto texture_images =
                to_texture_images(*info.mesh_data, model_data.textures_map);

            meshes.emplace_back(
                device,
                copy_pass,
                info.lod_ranges,
                info.meshlet_ranges,
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
        .meshlet_metadata_buffer = std::move(meshlet_metadata_buffer).value(),
        .meshlet_vertices_buffer = std::move(meshlet_vertices_buffer).value(),
        .meshlet_triangles_buffer = std::move(meshlet_triangles_buffer).value(),
        .meshes = std::move(meshes),
    };
}

}  // namespace Luminol::Graphics::SDL_GPU
