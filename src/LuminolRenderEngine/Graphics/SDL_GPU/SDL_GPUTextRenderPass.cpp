#include "SDL_GPUTextRenderPass.hpp"

#include <array>
#include <cstring>
#include <filesystem>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;
using namespace Luminol::Maths;

struct TextVertexUniforms {
    Vector2f screen_size;
    Vector2f padding;
};

constexpr auto text_vertex_stride =
    static_cast<uint32_t>(sizeof(float) * 8U);  // position(2) + uv(2) + color(4)

constexpr auto text_vertex_buffer_descriptions = std::array{
    VertexBufferDescription{.slot = 0, .pitch = text_vertex_stride},
};

constexpr auto text_vertex_attributes = std::array{
    VertexAttribute{
        .location = 0,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float2,
        .offset = 0,
    },
    VertexAttribute{
        .location = 1,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float2,
        .offset = sizeof(float) * 2,
    },
    VertexAttribute{
        .location = 2,
        .buffer_slot = 0,
        .format = VertexElementFormat::Float4,
        .offset = sizeof(float) * 4,
    },
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
        .primitive_type = PrimitiveType::TriangleList,
        .vertex_buffer_descriptions = text_vertex_buffer_descriptions,
        .vertex_attributes = text_vertex_attributes,
        .enable_depth_test = false,
        .cull_mode = CullMode::None,
        .enable_blend = true,
    });
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
          0U
      )},
      text_pipeline{make_text_pipeline(
          device, window, text_vertex_shader, text_fragment_shader
      )} {}

auto SDL_GPUTextRenderPass::queue_draw(
    FontId font_id,
    const SDL_GPUFont& font,
    std::string_view text,
    const Maths::Vector2f& position,
    const Maths::Vector4f& color
) -> void {
    if (text.empty()) {
        return;
    }

    pending_fonts.emplace(font_id, &font);
    auto& vertices = pending_glyphs[font_id];

    auto pen_x = position.x();
    const auto baseline_y = position.y() + font.get_ascent();

    for (const char character : text) {
        const auto codepoint =
            static_cast<char32_t>(static_cast<unsigned char>(character));
        const auto* glyph = font.get_glyph(codepoint);
        if (glyph == nullptr) {
            continue;
        }

        if (glyph->width > 0.0F && glyph->height > 0.0F) {
            const auto left = pen_x + glyph->bearing_x;
            const auto top = baseline_y + glyph->bearing_y;
            const auto right = left + glyph->width;
            const auto bottom = top + glyph->height;

            const auto top_left = SDL_GPUTextRenderPass::TextVertex{
                .position = Vector2f{left, top},
                .uv = Vector2f{glyph->u0, glyph->v0},
                .color = color,
            };
            const auto top_right = SDL_GPUTextRenderPass::TextVertex{
                .position = Vector2f{right, top},
                .uv = Vector2f{glyph->u1, glyph->v0},
                .color = color,
            };
            const auto bottom_left = SDL_GPUTextRenderPass::TextVertex{
                .position = Vector2f{left, bottom},
                .uv = Vector2f{glyph->u0, glyph->v1},
                .color = color,
            };
            const auto bottom_right = SDL_GPUTextRenderPass::TextVertex{
                .position = Vector2f{right, bottom},
                .uv = Vector2f{glyph->u1, glyph->v1},
                .color = color,
            };

            vertices.push_back(top_left);
            vertices.push_back(top_right);
            vertices.push_back(bottom_right);
            vertices.push_back(top_left);
            vertices.push_back(bottom_right);
            vertices.push_back(bottom_left);
        }

        pen_x += glyph->advance;
    }
}

auto SDL_GPUTextRenderPass::flush_frame_geometry(
    GPUDevice& device, CommandBuffer& command_buffer
) -> void {
    draw_batches.clear();

    if (!pending_glyphs.empty()) {
        auto copy_pass = command_buffer.begin_copy_pass();
        for (auto& [font_id, vertices] : pending_glyphs) {
            if (vertices.empty()) {
                continue;
            }

            const auto size_bytes =
                static_cast<uint32_t>(vertices.size() * sizeof(TextVertex));

            auto vertex_buffer = device.create_buffer(BufferInfo{
                .usage = BufferUsage::Vertex,
                .size = size_bytes,
            });

            auto transfer_buffer =
                device.create_transfer_buffer(TransferBufferInfo{
                    .usage = TransferBufferUsage::Upload,
                    .size = size_bytes,
                });
            {
                const auto mapped = transfer_buffer.map(false);
                std::memcpy(mapped.data(), vertices.data(), size_bytes);
                transfer_buffer.unmap();
            }

            copy_pass.upload_to_buffer(
                transfer_buffer, 0, vertex_buffer, 0, size_bytes, false
            );

            draw_batches.push_back(DrawBatch{
                .font = pending_fonts.at(font_id),
                .vertex_buffer = std::move(vertex_buffer),
                .vertex_count = static_cast<uint32_t>(vertices.size()),
            });
        }
    }

    pending_glyphs.clear();
    pending_fonts.clear();
}

auto SDL_GPUTextRenderPass::draw(
    CommandBuffer& command_buffer,
    RenderPass& render_pass,
    uint32_t screen_width,
    uint32_t screen_height
) -> void {
    if (!draw_batches.empty()) {
        render_pass.bind_graphics_pipeline(text_pipeline);

        const auto vertex_uniforms = TextVertexUniforms{
            .screen_size = Maths::Vector2f{
                static_cast<float>(screen_width),
                static_cast<float>(screen_height)
            },
            .padding = Maths::Vector2f{0.0F, 0.0F},
        };
        command_buffer.push_vertex_uniform_data(
            0,
            gsl::span{
                reinterpret_cast<const std::byte*>(&vertex_uniforms),
                sizeof(vertex_uniforms)
            }
        );

        for (const auto& batch : draw_batches) {
            const auto vertex_bindings = std::array{VertexBufferBinding{
                .buffer = &batch.vertex_buffer, .offset = 0
            }};
            render_pass.bind_vertex_buffers(0, vertex_bindings);

            const auto sampler_bindings = std::array{TextureSamplerBinding{
                .texture = &batch.font->get_atlas_texture(),
                .sampler = &batch.font->get_atlas_sampler()
            }};
            render_pass.bind_fragment_samplers(0, sampler_bindings);

            render_pass.draw_primitives(batch.vertex_count, 1, 0, 0);
        }
    }

    draw_batches.clear();
}

}  // namespace Luminol::Graphics::SDL_GPU
