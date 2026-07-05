#pragma once

#include <cstdint>

namespace Luminol::Graphics::SDL_GPU {

enum class LoadOp : uint8_t {
    Load,
    Clear,
    DontCare,
};

enum class StoreOp : uint8_t {
    Store,
    DontCare,
    Resolve,
    ResolveAndStore,
};

enum class PrimitiveType : uint8_t {
    TriangleList,
    TriangleStrip,
    LineList,
    LineStrip,
    PointList,
};

enum class TextureFormat : uint8_t {
    Invalid,
    B8G8R8A8_Unorm,
    R8G8B8A8_Unorm,
};

enum class VertexElementFormat : uint8_t {
    Float,
    Float2,
    Float3,
    Float4,
};

enum class VertexInputRate : uint8_t {
    Vertex,
    Instance,
};

struct VertexAttribute {
    uint32_t location;
    uint32_t buffer_slot;
    VertexElementFormat format;
    uint32_t offset;
};

struct VertexBufferDescription {
    uint32_t slot = 0;
    uint32_t pitch = 0;
    VertexInputRate input_rate = VertexInputRate::Vertex;
    uint32_t instance_step_rate = 0;
};

enum class ShaderStage : uint8_t {
    Vertex,
    Fragment,
};

enum class ShaderSourceLanguage : uint8_t {
    SpirvBinary,
    Hlsl,
};

enum class BufferUsage : uint8_t {
    Vertex,
    Index,
};

enum class TransferBufferUsage : uint8_t {
    Upload,
    Download,
};

}  // namespace Luminol::Graphics::SDL_GPU
