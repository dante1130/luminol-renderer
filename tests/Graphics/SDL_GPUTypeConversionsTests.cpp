#include <stdexcept>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypeConversions.hpp>

#include <doctest/doctest.h>

using namespace Luminol::Graphics::SDL_GPU;

TEST_CASE("to_sdl_shader_stage maps Vertex and Fragment") {
    CHECK(to_sdl_shader_stage(ShaderStage::Vertex) == SDL_GPU_SHADERSTAGE_VERTEX);
    CHECK(
        to_sdl_shader_stage(ShaderStage::Fragment) ==
        SDL_GPU_SHADERSTAGE_FRAGMENT
    );
}

TEST_CASE("to_sdl_shader_stage throws for Compute") {
    CHECK_THROWS_AS(
        (void)to_sdl_shader_stage(ShaderStage::Compute), std::runtime_error
    );
}

TEST_CASE("to_shadercross_stage maps all stages including Compute") {
    CHECK(
        to_shadercross_stage(ShaderStage::Vertex) ==
        SDL_SHADERCROSS_SHADERSTAGE_VERTEX
    );
    CHECK(
        to_shadercross_stage(ShaderStage::Fragment) ==
        SDL_SHADERCROSS_SHADERSTAGE_FRAGMENT
    );
    CHECK(
        to_shadercross_stage(ShaderStage::Compute) ==
        SDL_SHADERCROSS_SHADERSTAGE_COMPUTE
    );
}

TEST_CASE("from_sdl_texture_format maps known formats and defaults unknown to Invalid"
) {
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_INVALID) ==
        TextureFormat::Invalid
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM) ==
        TextureFormat::B8G8R8A8_Unorm
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM) ==
        TextureFormat::R8G8B8A8_Unorm
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB) ==
        TextureFormat::R8G8B8A8_Unorm_Srgb
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_D24_UNORM) ==
        TextureFormat::D24_Unorm
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT) ==
        TextureFormat::R16G16B16A16_Float
    );
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_R32_FLOAT) ==
        TextureFormat::R32_Float
    );
    // A format this project doesn't otherwise use should fall back to
    // Invalid rather than throwing.
    CHECK(
        from_sdl_texture_format(SDL_GPU_TEXTUREFORMAT_R8_UNORM) ==
        TextureFormat::Invalid
    );
}

TEST_CASE("to_sdl_primitive_type maps all primitive types") {
    CHECK(
        to_sdl_primitive_type(PrimitiveType::TriangleList) ==
        SDL_GPU_PRIMITIVETYPE_TRIANGLELIST
    );
    CHECK(
        to_sdl_primitive_type(PrimitiveType::TriangleStrip) ==
        SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP
    );
    CHECK(
        to_sdl_primitive_type(PrimitiveType::LineList) ==
        SDL_GPU_PRIMITIVETYPE_LINELIST
    );
    CHECK(
        to_sdl_primitive_type(PrimitiveType::LineStrip) ==
        SDL_GPU_PRIMITIVETYPE_LINESTRIP
    );
    CHECK(
        to_sdl_primitive_type(PrimitiveType::PointList) ==
        SDL_GPU_PRIMITIVETYPE_POINTLIST
    );
}

TEST_CASE("to_sdl_cull_mode maps all cull modes") {
    CHECK(to_sdl_cull_mode(CullMode::None) == SDL_GPU_CULLMODE_NONE);
    CHECK(to_sdl_cull_mode(CullMode::Front) == SDL_GPU_CULLMODE_FRONT);
    CHECK(to_sdl_cull_mode(CullMode::Back) == SDL_GPU_CULLMODE_BACK);
}

TEST_CASE("to_sdl_front_face maps both winding orders") {
    CHECK(
        to_sdl_front_face(FrontFace::CounterClockwise) ==
        SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE
    );
    CHECK(
        to_sdl_front_face(FrontFace::Clockwise) == SDL_GPU_FRONTFACE_CLOCKWISE
    );
}

TEST_CASE("to_sdl_texture_format maps all texture formats") {
    CHECK(
        to_sdl_texture_format(TextureFormat::Invalid) ==
        SDL_GPU_TEXTUREFORMAT_INVALID
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::B8G8R8A8_Unorm) ==
        SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::R8G8B8A8_Unorm) ==
        SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::R8G8B8A8_Unorm_Srgb) ==
        SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::D24_Unorm) ==
        SDL_GPU_TEXTUREFORMAT_D24_UNORM
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::R16G16B16A16_Float) ==
        SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT
    );
    CHECK(
        to_sdl_texture_format(TextureFormat::R32_Float) ==
        SDL_GPU_TEXTUREFORMAT_R32_FLOAT
    );
}

TEST_CASE("to_sdl_texture_usage maps a single flag") {
    CHECK(
        to_sdl_texture_usage(TextureUsage::Sampler) ==
        SDL_GPU_TEXTUREUSAGE_SAMPLER
    );
    CHECK(
        to_sdl_texture_usage(TextureUsage::ComputeStorageSimultaneousReadWrite
        ) == SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE
    );
}

TEST_CASE("to_sdl_texture_usage ORs multiple flags together") {
    const auto usage = TextureUsage::Sampler | TextureUsage::ColorTarget |
        TextureUsage::ComputeStorageRead;
    const auto flags = to_sdl_texture_usage(usage);

    CHECK((flags & SDL_GPU_TEXTUREUSAGE_SAMPLER) != 0);
    CHECK((flags & SDL_GPU_TEXTUREUSAGE_COLOR_TARGET) != 0);
    CHECK((flags & SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_READ) != 0);
    CHECK((flags & SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET) == 0);
    CHECK((flags & SDL_GPU_TEXTUREUSAGE_COMPUTE_STORAGE_WRITE) == 0);
}

TEST_CASE("to_sdl_sample_count maps all sample counts") {
    CHECK(to_sdl_sample_count(SampleCount::x1) == SDL_GPU_SAMPLECOUNT_1);
    CHECK(to_sdl_sample_count(SampleCount::x2) == SDL_GPU_SAMPLECOUNT_2);
    CHECK(to_sdl_sample_count(SampleCount::x4) == SDL_GPU_SAMPLECOUNT_4);
    CHECK(to_sdl_sample_count(SampleCount::x8) == SDL_GPU_SAMPLECOUNT_8);
}

TEST_CASE("to_sdl_vertex_element_format maps all formats") {
    CHECK(
        to_sdl_vertex_element_format(VertexElementFormat::Float) ==
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT
    );
    CHECK(
        to_sdl_vertex_element_format(VertexElementFormat::Float2) ==
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2
    );
    CHECK(
        to_sdl_vertex_element_format(VertexElementFormat::Float3) ==
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3
    );
    CHECK(
        to_sdl_vertex_element_format(VertexElementFormat::Float4) ==
        SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4
    );
}

TEST_CASE("to_sdl_vertex_input_rate maps both rates") {
    CHECK(
        to_sdl_vertex_input_rate(VertexInputRate::Vertex) ==
        SDL_GPU_VERTEXINPUTRATE_VERTEX
    );
    CHECK(
        to_sdl_vertex_input_rate(VertexInputRate::Instance) ==
        SDL_GPU_VERTEXINPUTRATE_INSTANCE
    );
}

TEST_CASE("to_sdl_buffer_usage ORs multiple flags together") {
    const auto usage = BufferUsage::Vertex | BufferUsage::Index |
        BufferUsage::Indirect;
    const auto flags = to_sdl_buffer_usage(usage);

    CHECK((flags & SDL_GPU_BUFFERUSAGE_VERTEX) != 0);
    CHECK((flags & SDL_GPU_BUFFERUSAGE_INDEX) != 0);
    CHECK((flags & SDL_GPU_BUFFERUSAGE_INDIRECT) != 0);
    CHECK((flags & SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ) == 0);
    CHECK((flags & SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ) == 0);
}

TEST_CASE("to_sdl_transfer_buffer_usage maps both directions") {
    CHECK(
        to_sdl_transfer_buffer_usage(TransferBufferUsage::Upload) ==
        SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD
    );
    CHECK(
        to_sdl_transfer_buffer_usage(TransferBufferUsage::Download) ==
        SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD
    );
}

TEST_CASE("to_sdl_filter maps both filters") {
    CHECK(to_sdl_filter(SamplerFilter::Nearest) == SDL_GPU_FILTER_NEAREST);
    CHECK(to_sdl_filter(SamplerFilter::Linear) == SDL_GPU_FILTER_LINEAR);
}

TEST_CASE("to_sdl_sampler_address_mode maps all address modes") {
    CHECK(
        to_sdl_sampler_address_mode(SamplerAddressMode::Repeat) ==
        SDL_GPU_SAMPLERADDRESSMODE_REPEAT
    );
    CHECK(
        to_sdl_sampler_address_mode(SamplerAddressMode::ClampToEdge) ==
        SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    );
    CHECK(
        to_sdl_sampler_address_mode(SamplerAddressMode::MirroredRepeat) ==
        SDL_GPU_SAMPLERADDRESSMODE_MIRRORED_REPEAT
    );
}
