#include "SDL_GPUMesh.hpp"

#include <array>
#include <cstring>

#include <gsl/gsl>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPURenderPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

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

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

SDL_GPUMesh::SDL_GPUMesh(
    GPUDevice& device,
    gsl::span<const float> vertices,
    gsl::span<const uint32_t> indices
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
      index_count{static_cast<uint32_t>(indices.size())} {}

auto SDL_GPUMesh::draw() const -> void {}

auto SDL_GPUMesh::draw_instanced(int32_t /*instance_count*/) const -> void {}

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
    sdl_gpu_pass.draw_indexed_primitives(
        index_count, static_cast<uint32_t>(instance_count)
    );
}

}  // namespace Luminol::Graphics::SDL_GPU
