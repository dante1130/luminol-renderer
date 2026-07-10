#include "SDL_GPUDevice.hpp"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <vector>

#include <gsl/gsl>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_log.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUCommandBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUComputePipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUGraphicsPipeline.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUShader.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTexture.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTransferBuffer.hpp>
#include <LuminolRenderEngine/Graphics/SDL_GPU/SDL_GPUTypeConversions.hpp>

namespace {

using namespace Luminol::Graphics::SDL_GPU;

auto create_sdl_gpu_device(SDL_Window* window) -> SDL_GPUDevice* {
    SDL_SetLogPriority(SDL_LOG_CATEGORY_GPU, SDL_LOG_PRIORITY_VERBOSE);

    auto* gpu_device =
        SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, nullptr);

    if (gpu_device == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUDevice: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_device, window)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to claim window for SDL_GPUDevice: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    if (!SDL_ShaderCross_Init()) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to initialize SDL_shadercross: %s",
            SDL_GetError()
        );
        return nullptr;
    }

    return gpu_device;
}

}  // namespace

namespace Luminol::Graphics::SDL_GPU {

// The deleter captures the SDL_Window* by value. The window must still be
// alive when this fires: the destruction order in RenderEngine (renderer →
// factory → device, then window) plus Window's explicit SDL_DestroyWindow in
// its dtor keeps that invariant. Do not reorder those members.
GPUDevice::GPUDevice(SDL_Window* window)
    : device{create_sdl_gpu_device(window), [window](SDL_GPUDevice* device) {
                 SDL_ShaderCross_Quit();
                 SDL_ReleaseWindowFromGPUDevice(device, window);
                 SDL_DestroyGPUDevice(device);
             }} {
    Expects(this->device != nullptr);

    const auto swapchain_format =
        SDL_GetGPUSwapchainTextureFormat(this->device.get(), window);
    if (swapchain_format == SDL_GPU_TEXTUREFORMAT_INVALID) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Window is not claimed by GPU device after claim call: %s",
            SDL_GetError()
        );
        Ensures(false);
    }
}

auto GPUDevice::native_handle() const -> SDL_GPUDevice* {
    return device.get();
}

auto GPUDevice::create_command_buffer() -> CommandBuffer {
    auto* command_buffer = SDL_AcquireGPUCommandBuffer(device.get());

    if (command_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUCommandBuffer: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    return CommandBuffer{command_buffer};
}

auto GPUDevice::create_shader(const ShaderInfo& info) -> Shader {
    auto shader_deleter = [device = shared_from_this()](SDL_GPUShader* shader) {
        SDL_ReleaseGPUShader(device->native_handle(), shader);
    };

    if (info.source_language == ShaderSourceLanguage::Hlsl) {
        auto shader_file = std::ifstream{info.path, std::ios::in};
        auto hlsl_source = std::string{
            std::istreambuf_iterator<char>{shader_file},
            std::istreambuf_iterator<char>{}
        };

        const auto hlsl_info = SDL_ShaderCross_HLSL_Info{
            .source = hlsl_source.c_str(),
            .entrypoint = info.entrypoint.c_str(),
            .include_dir = nullptr,
            .defines = nullptr,
            .shader_stage = to_shadercross_stage(info.stage),
            .props = 0,
        };

        size_t spirv_size = 0;
        auto* spirv_bytes = static_cast<uint8_t*>(
            SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &spirv_size)
        );
        if (spirv_bytes == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_ERROR,
                "Failed to compile HLSL to SPIRV (%s): %s",
                info.path.string().c_str(),
                SDL_GetError()
            );
            Ensures(false);
        }

        const auto spirv_info = SDL_ShaderCross_SPIRV_Info{
            .bytecode = spirv_bytes,
            .bytecode_size = spirv_size,
            .entrypoint = info.entrypoint.c_str(),
            .shader_stage = to_shadercross_stage(info.stage),
            .props = 0,
        };

        const auto resource_info = SDL_ShaderCross_GraphicsShaderResourceInfo{
            .num_samplers = info.sampler_count,
            .num_storage_textures = info.storage_texture_count,
            .num_storage_buffers = info.storage_buffer_count,
            .num_uniform_buffers = info.uniform_buffer_count,
        };

        auto* sdl_shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(
            this->device.get(), &spirv_info, &resource_info, 0
        );

        SDL_free(spirv_bytes);

        if (sdl_shader == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_ERROR,
                "Failed to compile graphics shader from SPIRV (%s): %s",
                info.path.string().c_str(),
                SDL_GetError()
            );
            Ensures(false);
        }

        return Shader{
            std::unique_ptr<SDL_GPUShader, Shader::SDL_GPUShaderDeleter>(
                sdl_shader, shader_deleter
            )
        };
    }

    auto shader_file = std::ifstream{
        info.path, std::ios::in | std::ios::binary | std::ios::ate
    };

    const auto code_size = shader_file.tellg();
    shader_file.seekg(0, std::ios::beg);

    auto code_vector = std::vector<uint8_t>{};
    code_vector.resize(code_size);
    shader_file.read(reinterpret_cast<char*>(code_vector.data()), code_size);

    const auto sdl_gpu_shader_create_info = SDL_GPUShaderCreateInfo{
        .code_size = static_cast<size_t>(code_size),
        .code = code_vector.data(),
        .entrypoint = info.entrypoint.c_str(),
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = to_sdl_shader_stage(info.stage),
        .num_samplers = info.sampler_count,
        .num_storage_textures = info.storage_texture_count,
        .num_storage_buffers = info.storage_buffer_count,
        .num_uniform_buffers = info.uniform_buffer_count,
        .props = 0,
    };

    return Shader{std::unique_ptr<SDL_GPUShader, Shader::SDL_GPUShaderDeleter>(
        SDL_CreateGPUShader(this->device.get(), &sdl_gpu_shader_create_info),
        shader_deleter
    )};
}

auto GPUDevice::create_graphics_pipeline(const GraphicsPipelineInfo& info)
    -> GraphicsPipeline {
    auto sdl_vertex_buffer_descriptions =
        std::vector<SDL_GPUVertexBufferDescription>{};
    sdl_vertex_buffer_descriptions.reserve(
        info.vertex_buffer_descriptions.size()
    );
    for (const auto& desc : info.vertex_buffer_descriptions) {
        sdl_vertex_buffer_descriptions.push_back(SDL_GPUVertexBufferDescription{
            .slot = desc.slot,
            .pitch = desc.pitch,
            .input_rate = to_sdl_vertex_input_rate(desc.input_rate),
            .instance_step_rate = desc.instance_step_rate,
        });
    }

    auto sdl_vertex_attributes = std::vector<SDL_GPUVertexAttribute>{};
    sdl_vertex_attributes.reserve(info.vertex_attributes.size());
    for (const auto& attr : info.vertex_attributes) {
        sdl_vertex_attributes.push_back(SDL_GPUVertexAttribute{
            .location = attr.location,
            .buffer_slot = attr.buffer_slot,
            .format = to_sdl_vertex_element_format(attr.format),
            .offset = attr.offset,
        });
    }

    const auto blend_state = info.enable_blend
        ? SDL_GPUColorTargetBlendState{
              .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
              .dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              .color_blend_op = SDL_GPU_BLENDOP_ADD,
              .src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
              .dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
              .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
              .enable_blend = true,
          }
        : SDL_GPUColorTargetBlendState{};

    const auto color_target_description = SDL_GPUColorTargetDescription{
        .format = to_sdl_texture_format(
            info.color_target_format.value_or(TextureFormat::Invalid)
        ),
        .blend_state = blend_state,
    };

    const auto create_info = SDL_GPUGraphicsPipelineCreateInfo{
        .vertex_shader = info.vertex_shader.native_handle(),
        .fragment_shader = info.fragment_shader.native_handle(),
        .vertex_input_state =
            SDL_GPUVertexInputState{
                .vertex_buffer_descriptions =
                    sdl_vertex_buffer_descriptions.data(),
                .num_vertex_buffers = static_cast<uint32_t>(
                    sdl_vertex_buffer_descriptions.size()
                ),
                .vertex_attributes = sdl_vertex_attributes.data(),
                .num_vertex_attributes =
                    static_cast<uint32_t>(sdl_vertex_attributes.size()),
            },
        .primitive_type = to_sdl_primitive_type(info.primitive_type),
        .rasterizer_state =
            SDL_GPURasterizerState{
                .fill_mode = SDL_GPU_FILLMODE_FILL,
                .cull_mode = to_sdl_cull_mode(info.cull_mode),
                .front_face = to_sdl_front_face(info.front_face),
                .depth_bias_constant_factor = 0.0F,
                .depth_bias_clamp = 0.0F,
                .depth_bias_slope_factor = 0.0F,
                .enable_depth_bias = false,
                .enable_depth_clip = false,
                .padding1 = 0,
                .padding2 = 0,
            },
        .multisample_state =
            SDL_GPUMultisampleState{
                .sample_count = to_sdl_sample_count(info.sample_count),
                .sample_mask = 0,
                .enable_mask = false,
                .enable_alpha_to_coverage = false,
                .padding2 = 0,
                .padding3 = 0,
            },
        .depth_stencil_state =
            SDL_GPUDepthStencilState{
                .compare_op = info.enable_depth_test
                    ? SDL_GPU_COMPAREOP_LESS_OR_EQUAL
                    : SDL_GPU_COMPAREOP_INVALID,
                .back_stencil_state = {},
                .front_stencil_state = {},
                .compare_mask = 0,
                .write_mask = 0,
                .enable_depth_test = info.enable_depth_test,
                .enable_depth_write = info.enable_depth_test && info.enable_depth_write,
                .enable_stencil_test = false,
                .padding1 = 0,
                .padding2 = 0,
                .padding3 = 0,
            },
        .target_info =
            SDL_GPUGraphicsPipelineTargetInfo{
                .color_target_descriptions = info.color_target_format.has_value()
                    ? &color_target_description
                    : nullptr,
                .num_color_targets =
                    info.color_target_format.has_value() ? 1U : 0U,
                .depth_stencil_format =
                    to_sdl_texture_format(info.depth_stencil_format),
                .has_depth_stencil_target = info.enable_depth_test,
                .padding1 = 0,
                .padding2 = 0,
                .padding3 = 0,
            },
        .props = 0,
    };

    auto* pipeline =
        SDL_CreateGPUGraphicsPipeline(this->device.get(), &create_info);

    if (pipeline == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUGraphicsPipeline: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    return GraphicsPipeline{std::unique_ptr<
        SDL_GPUGraphicsPipeline,
        GraphicsPipeline::SDL_GPUGraphicsPipelineDeleter>(
        pipeline,
        [device = shared_from_this()](
            SDL_GPUGraphicsPipeline* pipeline_to_release
        ) {
            SDL_ReleaseGPUGraphicsPipeline(
                device->native_handle(), pipeline_to_release
            );
        }
    )};
}

auto GPUDevice::create_compute_pipeline(const ComputePipelineInfo& info)
    -> ComputePipeline {
    auto pipeline_deleter =
        [device = shared_from_this()](SDL_GPUComputePipeline* pipeline) {
            SDL_ReleaseGPUComputePipeline(device->native_handle(), pipeline);
        };

    if (info.source_language == ShaderSourceLanguage::Hlsl) {
        auto shader_file = std::ifstream{info.path, std::ios::in};
        auto hlsl_source = std::string{
            std::istreambuf_iterator<char>{shader_file},
            std::istreambuf_iterator<char>{}
        };

        const auto hlsl_info = SDL_ShaderCross_HLSL_Info{
            .source = hlsl_source.c_str(),
            .entrypoint = info.entrypoint.c_str(),
            .include_dir = nullptr,
            .defines = nullptr,
            .shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,
            .props = 0,
        };

        size_t spirv_size = 0;
        auto* spirv_bytes = static_cast<uint8_t*>(
            SDL_ShaderCross_CompileSPIRVFromHLSL(&hlsl_info, &spirv_size)
        );
        if (spirv_bytes == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_ERROR,
                "Failed to compile HLSL to SPIRV (%s): %s",
                info.path.string().c_str(),
                SDL_GetError()
            );
            Ensures(false);
        }

        const auto spirv_info = SDL_ShaderCross_SPIRV_Info{
            .bytecode = spirv_bytes,
            .bytecode_size = spirv_size,
            .entrypoint = info.entrypoint.c_str(),
            .shader_stage = SDL_SHADERCROSS_SHADERSTAGE_COMPUTE,
            .props = 0,
        };

        const auto metadata = SDL_ShaderCross_ComputePipelineMetadata{
            .num_samplers = info.sampler_count,
            .num_readonly_storage_textures =
                info.readonly_storage_texture_count,
            .num_readonly_storage_buffers = info.readonly_storage_buffer_count,
            .num_readwrite_storage_textures =
                info.readwrite_storage_texture_count,
            .num_readwrite_storage_buffers =
                info.readwrite_storage_buffer_count,
            .num_uniform_buffers = info.uniform_buffer_count,
            .threadcount_x = info.threadcount_x,
            .threadcount_y = info.threadcount_y,
            .threadcount_z = info.threadcount_z,
        };

        auto* sdl_pipeline = SDL_ShaderCross_CompileComputePipelineFromSPIRV(
            this->device.get(), &spirv_info, &metadata, 0
        );

        SDL_free(spirv_bytes);

        if (sdl_pipeline == nullptr) {
            SDL_LogError(
                SDL_LOG_CATEGORY_ERROR,
                "Failed to compile compute pipeline from SPIRV (%s): %s",
                info.path.string().c_str(),
                SDL_GetError()
            );
            Ensures(false);
        }

        return ComputePipeline{std::unique_ptr<
            SDL_GPUComputePipeline,
            ComputePipeline::SDL_GPUComputePipelineDeleter>(
            sdl_pipeline, pipeline_deleter
        )};
    }

    auto shader_file = std::ifstream{
        info.path, std::ios::in | std::ios::binary | std::ios::ate
    };

    const auto code_size = shader_file.tellg();
    shader_file.seekg(0, std::ios::beg);

    auto code_vector = std::vector<uint8_t>{};
    code_vector.resize(code_size);
    shader_file.read(reinterpret_cast<char*>(code_vector.data()), code_size);

    const auto create_info = SDL_GPUComputePipelineCreateInfo{
        .code_size = static_cast<size_t>(code_size),
        .code = code_vector.data(),
        .entrypoint = info.entrypoint.c_str(),
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .num_samplers = info.sampler_count,
        .num_readonly_storage_textures = info.readonly_storage_texture_count,
        .num_readonly_storage_buffers = info.readonly_storage_buffer_count,
        .num_readwrite_storage_textures = info.readwrite_storage_texture_count,
        .num_readwrite_storage_buffers = info.readwrite_storage_buffer_count,
        .num_uniform_buffers = info.uniform_buffer_count,
        .threadcount_x = info.threadcount_x,
        .threadcount_y = info.threadcount_y,
        .threadcount_z = info.threadcount_z,
        .props = 0,
    };

    return ComputePipeline{std::unique_ptr<
        SDL_GPUComputePipeline,
        ComputePipeline::SDL_GPUComputePipelineDeleter>(
        SDL_CreateGPUComputePipeline(this->device.get(), &create_info),
        pipeline_deleter
    )};
}

auto GPUDevice::create_buffer(const BufferInfo& info) -> Buffer {
    const auto create_info = SDL_GPUBufferCreateInfo{
        .usage = to_sdl_buffer_usage(info.usage),
        .size = info.size,
        .props = 0,
    };

    auto* raw_buffer = SDL_CreateGPUBuffer(this->device.get(), &create_info);

    if (raw_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUBuffer: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    auto owned = std::unique_ptr<SDL_GPUBuffer, Buffer::SDL_GPUBufferDeleter>(
        raw_buffer,
        [device = shared_from_this()](SDL_GPUBuffer* buffer_to_release) {
            SDL_ReleaseGPUBuffer(device->native_handle(), buffer_to_release);
        }
    );

    return Buffer{std::move(owned), info.size};
}

auto GPUDevice::create_transfer_buffer(const TransferBufferInfo& info)
    -> TransferBuffer {
    const auto create_info = SDL_GPUTransferBufferCreateInfo{
        .usage = to_sdl_transfer_buffer_usage(info.usage),
        .size = info.size,
        .props = 0,
    };

    auto* raw_transfer_buffer =
        SDL_CreateGPUTransferBuffer(this->device.get(), &create_info);

    if (raw_transfer_buffer == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUTransferBuffer: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    auto owned = std::unique_ptr<
        SDL_GPUTransferBuffer,
        TransferBuffer::SDL_GPUTransferBufferDeleter>(
        raw_transfer_buffer,
        [device = shared_from_this()](
            SDL_GPUTransferBuffer* transfer_buffer_to_release
        ) {
            SDL_ReleaseGPUTransferBuffer(
                device->native_handle(), transfer_buffer_to_release
            );
        }
    );

    return TransferBuffer{
        std::move(owned), shared_from_this(), info.size
    };
}

auto GPUDevice::create_texture(const TextureInfo& info) -> Texture {
    const auto num_levels = info.generate_mipmaps
        ? 1U +
            static_cast<uint32_t>(std::floor(
                std::log2(static_cast<float>(std::max(info.width, info.height)))
            ))
        : info.mip_levels;

    auto usage_flags = to_sdl_texture_usage(info.usage);
    if (info.generate_mipmaps) {
        usage_flags |= SDL_GPU_TEXTUREUSAGE_COLOR_TARGET;
    }

    const auto sdl_texture_type = [&info] {
        switch (info.type) {
            case TextureType::TextureCube:
                return SDL_GPU_TEXTURETYPE_CUBE;
            case TextureType::Texture2DArray:
                return SDL_GPU_TEXTURETYPE_2D_ARRAY;
            case TextureType::TextureCubeArray:
                return SDL_GPU_TEXTURETYPE_CUBE_ARRAY;
            case TextureType::Texture2D:
            default:
                return SDL_GPU_TEXTURETYPE_2D;
        }
    }();

    const auto layer_count_or_depth = [&info] {
        switch (info.type) {
            case TextureType::TextureCube:
                return 6U;
            case TextureType::Texture2DArray:
                return info.layer_count;
            case TextureType::TextureCubeArray:
                return 6U * info.layer_count;
            case TextureType::Texture2D:
            default:
                return 1U;
        }
    }();

    const auto create_info = SDL_GPUTextureCreateInfo{
        .type = sdl_texture_type,
        .format = to_sdl_texture_format(info.format),
        .usage = usage_flags,
        .width = info.width,
        .height = info.height,
        .layer_count_or_depth = layer_count_or_depth,
        .num_levels = num_levels,
        .sample_count = to_sdl_sample_count(info.sample_count),
        .props = 0,
    };

    auto* raw_texture = SDL_CreateGPUTexture(this->device.get(), &create_info);

    if (raw_texture == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUTexture: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    auto owned = std::unique_ptr<SDL_GPUTexture, Texture::SDL_GPUTextureDeleter>(
        raw_texture,
        [device = shared_from_this()](SDL_GPUTexture* texture_to_release) {
            SDL_ReleaseGPUTexture(device->native_handle(), texture_to_release);
        }
    );

    return Texture{std::move(owned), info.width, info.height, num_levels};
}

auto GPUDevice::create_sampler(const SamplerInfo& info) -> Sampler {
    const auto create_info = SDL_GPUSamplerCreateInfo{
        .min_filter = to_sdl_filter(info.filter),
        .mag_filter = to_sdl_filter(info.filter),
        .mipmap_mode = info.enable_mipmap_filtering
            ? SDL_GPU_SAMPLERMIPMAPMODE_LINEAR
            : SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
        .address_mode_u = to_sdl_sampler_address_mode(info.address_mode_u),
        .address_mode_v = to_sdl_sampler_address_mode(info.address_mode_v),
        .address_mode_w = to_sdl_sampler_address_mode(info.address_mode_u),
        .mip_lod_bias = 0.0F,
        .max_anisotropy = 1.0F,
        .compare_op = info.enable_compare
            ? SDL_GPU_COMPAREOP_LESS_OR_EQUAL
            : SDL_GPU_COMPAREOP_INVALID,
        .min_lod = 0.0F,
        .max_lod = info.enable_mipmap_filtering ? FLT_MAX : 0.0F,
        .enable_anisotropy = false,
        .enable_compare = info.enable_compare,
        .padding1 = 0,
        .padding2 = 0,
        .props = 0,
    };

    auto* raw_sampler = SDL_CreateGPUSampler(this->device.get(), &create_info);

    if (raw_sampler == nullptr) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to create SDL_GPUSampler: %s",
            SDL_GetError()
        );
        Ensures(false);
    }

    auto owned = std::unique_ptr<SDL_GPUSampler, Sampler::SDL_GPUSamplerDeleter>(
        raw_sampler,
        [device = shared_from_this()](SDL_GPUSampler* sampler_to_release) {
            SDL_ReleaseGPUSampler(device->native_handle(), sampler_to_release);
        }
    );

    return Sampler{std::move(owned)};
}

auto GPUDevice::get_swapchain_texture_format(SDL_Window* window) const
    -> TextureFormat {
    const auto sdl_format =
        SDL_GetGPUSwapchainTextureFormat(this->device.get(), window);
    const auto format = from_sdl_texture_format(sdl_format);
    if (format == TextureFormat::Invalid) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Swapchain texture format %d has no Luminol mapping",
            static_cast<int>(sdl_format)
        );
        Ensures(false);
    }
    return format;
}

auto GPUDevice::supports_sample_count(
    TextureFormat format, SampleCount sample_count
) const -> bool {
    return SDL_GPUTextureSupportsSampleCount(
        this->device.get(),
        to_sdl_texture_format(format),
        to_sdl_sample_count(sample_count)
    );
}

auto GPUDevice::wait_for_idle() const -> void {
    if (!SDL_WaitForGPUIdle(this->device.get())) {
        SDL_LogError(
            SDL_LOG_CATEGORY_ERROR,
            "Failed to wait for GPU idle: %s",
            SDL_GetError()
        );
    }
}

}  // namespace Luminol::Graphics::SDL_GPU
