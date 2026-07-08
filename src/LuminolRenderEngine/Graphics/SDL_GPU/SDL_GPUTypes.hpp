#pragma once

#include <cstdint>

namespace Luminol::Graphics::SDL_GPU {

class Buffer;
class Texture;
class Sampler;

struct VertexBufferBinding {
    // Required; asserted non-null in bind_vertex_buffers. Pointer (not
    // reference) so callers can use designated-initializer syntax.
    const Buffer* buffer = nullptr;
    uint32_t offset = 0;
};

struct TextureSamplerBinding {
    // Required; asserted non-null in bind_fragment_samplers. Pointers (not
    // references) so callers can use designated-initializer syntax.
    const Texture* texture = nullptr;
    const Sampler* sampler = nullptr;
};

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

enum class CullMode : uint8_t {
    None,
    Front,
    Back,
};

enum class FrontFace : uint8_t {
    CounterClockwise,
    Clockwise,
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
    R8G8B8A8_Unorm_Srgb,
    D24_Unorm,
    R16G16B16A16_Float,
};

enum class TextureUsage : uint8_t {
    Sampler = 1U << 0U,
    ColorTarget = 1U << 1U,
    DepthStencilTarget = 1U << 2U,
};

constexpr auto operator|(TextureUsage lhs, TextureUsage rhs) -> TextureUsage {
    return static_cast<TextureUsage>(
        static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs)
    );
}

constexpr auto operator&(TextureUsage lhs, TextureUsage rhs) -> TextureUsage {
    return static_cast<TextureUsage>(
        static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs)
    );
}

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
    StorageRead,
};

enum class TransferBufferUsage : uint8_t {
    Upload,
    Download,
};

enum class IndexElementSize : uint8_t {
    Bits16,
    Bits32,
};

enum class SamplerFilter : uint8_t {
    Nearest,
    Linear,
};

enum class SamplerAddressMode : uint8_t {
    Repeat,
    ClampToEdge,
};

}  // namespace Luminol::Graphics::SDL_GPU
