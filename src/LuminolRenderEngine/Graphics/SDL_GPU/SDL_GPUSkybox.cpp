#include "SDL_GPUSkybox.hpp"

#include <array>
#include <cstring>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Utilities/ImageLoader.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

constexpr auto desired_rgba_channels = int32_t{4};
constexpr auto skybox_face_count = uint32_t{6};

// SDL_GPU cubemap layer order matches D3D/Vulkan: +X, -X, +Y, -Y, +Z, -Z.
auto load_faces_in_layer_order(const Luminol::Graphics::SkyboxPaths& paths)
    -> std::array<Luminol::Utilities::ImageLoader::Image, skybox_face_count> {
    return std::array{
        Luminol::Utilities::ImageLoader::load_image(
            paths.right, desired_rgba_channels
        ),
        Luminol::Utilities::ImageLoader::load_image(
            paths.left, desired_rgba_channels
        ),
        Luminol::Utilities::ImageLoader::load_image(
            paths.top, desired_rgba_channels
        ),
        Luminol::Utilities::ImageLoader::load_image(
            paths.bottom, desired_rgba_channels
        ),
        Luminol::Utilities::ImageLoader::load_image(
            paths.front, desired_rgba_channels
        ),
        Luminol::Utilities::ImageLoader::load_image(
            paths.back, desired_rgba_channels
        ),
    };
}

auto create_skybox_texture(
    GPUDevice& device,
    CopyPass& copy_pass,
    const std::array<Luminol::Utilities::ImageLoader::Image, skybox_face_count>&
        faces
) -> Texture {
    const auto width = static_cast<uint32_t>(faces[0].width);
    const auto height = static_cast<uint32_t>(faces[0].height);
    const auto face_size_bytes = width * height * 4U;

    auto texture = device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = TextureFormat::R8G8B8A8_Unorm_Srgb,
        .usage = TextureUsage::Sampler,
        .type = TextureType::TextureCube,
    });

    for (auto layer = uint32_t{0}; layer < skybox_face_count; ++layer) {
        auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Upload,
            .size = face_size_bytes,
        });

        const auto mapped = transfer_buffer.map(false);
        std::memcpy(
            mapped.data(), gsl::at(faces, layer).data.data(), face_size_bytes
        );
        transfer_buffer.unmap();

        copy_pass.upload_to_texture(
            transfer_buffer, 0, texture, width, height, false, layer
        );
    }

    return texture;
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUSkybox::SDL_GPUSkybox(
    GPUDevice& device, CopyPass& copy_pass, const SkyboxPaths& paths
)
    : texture{create_skybox_texture(
          device, copy_pass, load_faces_in_layer_order(paths)
      )},
      sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode_u = SamplerAddressMode::ClampToEdge,
          .address_mode_v = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPUSkybox::get_texture() const -> const Texture& { return texture; }

auto SDL_GPUSkybox::get_sampler() const -> const Sampler& { return sampler; }

}  // namespace Luminol::Graphics::SDL_GPU
