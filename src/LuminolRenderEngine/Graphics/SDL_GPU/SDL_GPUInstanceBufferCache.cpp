#include "SDL_GPUInstanceBufferCache.hpp"

#include <cstring>
#include <numeric>
#include <vector>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCopyPass.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUDevice.hpp>

namespace Luminol::Graphics::SDL_GPU {

namespace {

auto ensure_identity_indices_capacity(
    GPUDevice& device,
    CopyPass& copy_pass,
    std::optional<Buffer>& identity_indices_buffer,
    std::optional<TransferBuffer>& identity_indices_transfer_buffer,
    uint32_t required_count
) -> void {
    const auto required_size = required_count * static_cast<uint32_t>(sizeof(uint32_t));
    if (identity_indices_buffer.has_value() &&
        identity_indices_buffer->get_size() >= required_size) {
        return;
    }

    if (!identity_indices_transfer_buffer.has_value() ||
        identity_indices_transfer_buffer->get_size() < required_size) {
        identity_indices_transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Upload,
            .size = required_size,
        });
    }
    identity_indices_buffer = device.create_buffer(BufferInfo{
        .usage = BufferUsage::StorageRead,
        .size = required_size,
    });

    auto indices = std::vector<uint32_t>(required_count);
    std::iota(indices.begin(), indices.end(), 0U);

    const auto mapped = identity_indices_transfer_buffer->map(true);
    std::memcpy(mapped.data(), indices.data(), required_size);
    identity_indices_transfer_buffer->unmap();
    copy_pass.upload_to_buffer(
        *identity_indices_transfer_buffer, 0, *identity_indices_buffer, 0,
        required_size, true
    );
}

}  // namespace

auto SDL_GPUInstanceBufferCache::upload(
    GPUDevice& device,
    CopyPass& copy_pass,
    RenderableId renderable_id,
    gsl::span<const Maths::Matrix4x4f> model_matrices
) -> const Buffer& {
    const auto required_size = static_cast<uint32_t>(
        model_matrices.size() * sizeof(Maths::Matrix4x4f)
    );

    if (renderable_id >= instance_transfer_buffers.size()) {
        instance_transfer_buffers.resize(renderable_id + 1);
    }
    if (renderable_id >= instance_buffers.size()) {
        instance_buffers.resize(renderable_id + 1);
    }

    auto& transfer_buffer = instance_transfer_buffers[renderable_id];
    if (!transfer_buffer.has_value() ||
        transfer_buffer->get_size() < required_size) {
        transfer_buffer = device.create_transfer_buffer(TransferBufferInfo{
            .usage = TransferBufferUsage::Upload,
            .size = required_size,
        });
    }

    auto& instance_buffer = instance_buffers[renderable_id];
    if (!instance_buffer.has_value() ||
        instance_buffer->get_size() < required_size) {
        instance_buffer = device.create_buffer(BufferInfo{
            .usage = BufferUsage::StorageRead | BufferUsage::ComputeStorageRead,
            .size = required_size,
        });
    }

    const auto mapped = transfer_buffer->map(true);
    std::memcpy(mapped.data(), model_matrices.data(), required_size);
    transfer_buffer->unmap();

    copy_pass.upload_to_buffer(
        *transfer_buffer, 0, *instance_buffer, 0, required_size, true
    );

    ensure_identity_indices_capacity(
        device, copy_pass, identity_indices_buffer,
        identity_indices_transfer_buffer,
        static_cast<uint32_t>(model_matrices.size())
    );

    return *instance_buffer;
}

auto SDL_GPUInstanceBufferCache::get(RenderableId renderable_id) const
    -> const Buffer& {
    return gsl::at(instance_buffers, renderable_id).value();
}

auto SDL_GPUInstanceBufferCache::get_identity_indices_buffer() const
    -> const Buffer& {
    return identity_indices_buffer.value();
}

}  // namespace Luminol::Graphics::SDL_GPU
