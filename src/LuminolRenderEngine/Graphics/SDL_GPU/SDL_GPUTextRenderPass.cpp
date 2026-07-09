#include "SDL_GPUTextRenderPass.hpp"

#include <array>
#include <cstring>
#include <filesystem>
#include <optional>
#include <tuple>

#include <gsl/gsl>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

struct TextVertexUniforms {
    Vector4f rect_pixels;
    Vector2f screen_size;
    Vector2f padding;
};

struct TextFragmentUniforms {
    Vector4f tint;
};

auto make_shader(
    GPUDevice& device,
    const std::filesystem::path& path,
    ShaderStage stage,
    uint32_t sampler_count,
    uint32_t uniform_buffer_count
) -> Shader {
    return device.create_shader(ShaderInfo{
        .path = path,
        .stage = stage,
        .source_language = ShaderSourceLanguage::Hlsl,
        .sampler_count = sampler_count,
        .uniform_buffer_count = uniform_buffer_count,
        .storage_buffer_count = 0U,
    });
}

auto make_text_pipeline(
    GPUDevice& device,
    SDL_Window* window,
    const Shader& vertex_shader,
    const Shader& fragment_shader
) -> GraphicsPipeline {
    return device.create_graphics_pipeline(GraphicsPipelineInfo{
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .color_target_format = device.get_swapchain_texture_format(window),
        .primitive_type = PrimitiveType::TriangleStrip,
        .vertex_buffer_descriptions = {},
        .vertex_attributes = {},
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
        .enable_blend = true,
    });
}

auto make_cache_key(
    FontId font_id, std::string_view text, const Vector4f& color
) -> std::string {
    const auto color_components =
        std::array{color.x(), color.y(), color.z(), color.w()};

    auto key = std::string{};
    key.resize(sizeof(FontId) + sizeof(color_components));
    std::memcpy(key.data(), &font_id, sizeof(FontId));
    std::memcpy(
        key.data() + sizeof(FontId),
        color_components.data(),
        sizeof(color_components)
    );
    key += text;
    return key;
}

// Rasterizes text to a tightly-packed RGBA8 buffer, uploading it as a GPU
// texture. Mirrors SDL_GPUMesh.cpp's create_uploaded_texture helper, but
// starts from an SDL_ttf-rendered SDL_Surface (which may have row padding)
// rather than a stb_image buffer.
auto create_text_texture(
    GPUDevice& device, const SDL_GPUFont& font, std::string_view text,
    const Vector4f& color
) -> std::optional<std::tuple<Texture, uint32_t, uint32_t>> {
    const auto sdl_color = SDL_Color{
        .r = static_cast<uint8_t>(color.x() * 255.0F),
        .g = static_cast<uint8_t>(color.y() * 255.0F),
        .b = static_cast<uint8_t>(color.z() * 255.0F),
        .a = static_cast<uint8_t>(color.w() * 255.0F),
    };

    auto* rendered_surface = TTF_RenderText_Blended(
        font.native_handle(), text.data(), text.size(), sdl_color
    );
    if (rendered_surface == nullptr) {
        return std::nullopt;
    }

    auto* rgba_surface =
        SDL_ConvertSurface(rendered_surface, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(rendered_surface);
    if (rgba_surface == nullptr) {
        return std::nullopt;
    }

    const auto width = static_cast<uint32_t>(rgba_surface->w);
    const auto height = static_cast<uint32_t>(rgba_surface->h);
    const auto row_bytes = width * 4U;
    const auto size_bytes = row_bytes * height;

    auto transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
        .usage = TransferBufferUsage::Upload,
        .size = size_bytes,
    });

    {
        const auto mapped = transfer_buffer.map(false);
        const auto* src_pixels = static_cast<const uint8_t*>(rgba_surface->pixels
        );
        for (uint32_t row = 0; row < height; ++row) {
            std::memcpy(
                mapped.data() + (row * row_bytes),
                src_pixels + (row * static_cast<uint32_t>(rgba_surface->pitch)),
                row_bytes
            );
        }
        transfer_buffer.unmap();
    }

    SDL_DestroySurface(rgba_surface);

    auto texture = device.create_texture(TextureInfo{
        .width = width,
        .height = height,
        .format = TextureFormat::R8G8B8A8_Unorm,
    });

    auto command_buffer = device.create_command_buffer();
    {
        auto copy_pass = command_buffer.begin_copy_pass();
        copy_pass.upload_to_texture(
            transfer_buffer, 0, texture, width, height, false
        );
    }
    command_buffer.submit();

    return std::make_tuple(std::move(texture), width, height);
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUTextRenderPass::SDL_GPUTextRenderPass(GPUDevice& device, SDL_Window* window)
    : text_vertex_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/text_vert.hlsl",
          ShaderStage::Vertex,
          0U,
          1U
      )},
      text_fragment_shader{make_shader(
          device,
          "res/shaders/sdl_gpu/text_frag.hlsl",
          ShaderStage::Fragment,
          1U,
          1U
      )},
      text_pipeline{make_text_pipeline(
          device, window, text_vertex_shader, text_fragment_shader
      )},
      clamp_sampler{device.create_sampler(SamplerInfo{
          .filter = SamplerFilter::Linear,
          .address_mode = SamplerAddressMode::ClampToEdge,
      })} {}

auto SDL_GPUTextRenderPass::queue_draw(
    GPUDevice& device,
    FontId font_id,
    const SDL_GPUFont& font,
    std::string_view text,
    const Maths::Vector2f& position,
    const Maths::Vector4f& color
) -> void {
    if (text.empty()) {
        return;
    }

    const auto key = make_cache_key(font_id, text, color);

    auto cache_iter = text_cache.find(key);
    if (cache_iter == text_cache.end()) {
        auto created = create_text_texture(device, font, text, color);
        if (!created.has_value()) {
            return;
        }

        auto [texture, width, height] = std::move(created.value());
        cache_iter = text_cache
                         .emplace(
                             key,
                             TextCacheEntry{
                                 .texture = std::move(texture),
                                 .width = width,
                                 .height = height,
                             }
                         )
                         .first;
    }

    cache_iter->second.used_this_frame = true;
    queued_draws.push_back(QueuedText{
        .entry = &cache_iter->second,
        .position = position,
    });
}

auto SDL_GPUTextRenderPass::draw(
    CommandBuffer& command_buffer,
    RenderPass& render_pass,
    uint32_t screen_width,
    uint32_t screen_height
) -> void {
    if (!queued_draws.empty()) {
        render_pass.bind_graphics_pipeline(text_pipeline);

        const auto screen_size = Maths::Vector2f{
            static_cast<float>(screen_width), static_cast<float>(screen_height)
        };

        for (const auto& queued : queued_draws) {
            const auto vertex_uniforms = TextVertexUniforms{
                .rect_pixels = Maths::Vector4f{
                    queued.position.x(),
                    queued.position.y(),
                    static_cast<float>(queued.entry->width),
                    static_cast<float>(queued.entry->height),
                },
                .screen_size = screen_size,
                .padding = Maths::Vector2f{0.0F, 0.0F},
            };
            command_buffer.push_vertex_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&vertex_uniforms),
                    sizeof(vertex_uniforms)
                }
            );

            const auto fragment_uniforms = TextFragmentUniforms{
                .tint = Maths::Vector4f{1.0F, 1.0F, 1.0F, 1.0F},
            };
            command_buffer.push_fragment_uniform_data(
                0,
                gsl::span{
                    reinterpret_cast<const std::byte*>(&fragment_uniforms),
                    sizeof(fragment_uniforms)
                }
            );

            const auto sampler_bindings = std::array{TextureSamplerBinding{
                .texture = &queued.entry->texture, .sampler = &clamp_sampler
            }};
            render_pass.bind_fragment_samplers(0, sampler_bindings);

            render_pass.draw_primitives(4, 1, 0, 0);
        }
    }

    queued_draws.clear();

    for (auto iter = text_cache.begin(); iter != text_cache.end();) {
        if (iter->second.used_this_frame) {
            iter->second.used_this_frame = false;
            ++iter;
        } else {
            iter = text_cache.erase(iter);
        }
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
