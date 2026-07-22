add_library(Luminol.Graphics.SDL_GPU
    SDL_GPURenderer.cpp
    SDL_GPUDevice.cpp
    SDL_GPUTypeConversions.cpp
    SDL_GPUShader.cpp
    SDL_GPUCommandBuffer.cpp
    RenderPasses/SDL_GPURenderPass.cpp
    RenderPasses/SDL_GPUComputePass.cpp
    SDL_GPUGraphicsPipeline.cpp
    SDL_GPUComputePipeline.cpp
    SDL_GPUBuffer.cpp
    SDL_GPUTransferBuffer.cpp
    SDL_GPUFactory.cpp
    SDL_GPUResourceBuilders.cpp
    SDL_GPUTexture.cpp
    RenderPasses/SDL_GPUCopyPass.cpp
    SDL_GPUMesh.cpp
    SDL_GPUInstanceBufferCache.cpp
    SDL_GPUMeshRenderPass.cpp
    SDL_GPUAmbientOcclusionPass.cpp
    SDL_GPUScreenSpaceReflectionPass.cpp
    SDL_GPUClusterPass.cpp
    Culling/SDL_GPUInstanceCullPass.cpp
    Culling/SDL_GPUMeshletCullPass.cpp
    Culling/SDL_GPUHiZPass.cpp
    Culling/SDL_GPUOcclusionDepthPass.cpp
    Shadows/SDL_GPUShadowPass.cpp
    Culling/SDL_GPUCullingUtils.cpp
    Shadows/SDL_GPUPointSpotShadowPass.cpp
    SDL_GPUTonemapPass.cpp
    SDL_GPUSkybox.cpp
    SDL_GPUSkyboxRenderPass.cpp
    SDL_GPUIBLRenderPass.cpp
    SDL_GPUFont.cpp
    SDL_GPUTextRenderPass.cpp
)

target_compile_features(Luminol.Graphics.SDL_GPU PRIVATE cxx_std_20)
set_target_properties(Luminol.Graphics.SDL_GPU PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Graphics.SDL_GPU PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Graphics.SDL_GPU PRIVATE
    GSL
    Luminol.Utilities
    SDL3_ttf::SDL3_ttf
    meshoptimizer
)

target_link_libraries(Luminol.Graphics.SDL_GPU PUBLIC
    SDL3::SDL3-static
    SDL3_shadercross-static
)

target_include_directories(Luminol.Graphics.SDL_GPU PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
