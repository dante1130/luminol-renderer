add_library(Luminol.Graphics.SDL_GPU
    SDL_GPURenderer.cpp
    SDL_GPUDevice.cpp
    SDL_GPUShader.cpp
    SDL_GPUCommandBuffer.cpp
    SDL_GPURenderPass.cpp
    SDL_GPUGraphicsPipeline.cpp
    SDL_GPUBuffer.cpp
    SDL_GPUTransferBuffer.cpp
    SDL_GPUFactory.cpp
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
)

target_link_libraries(Luminol.Graphics.SDL_GPU PUBLIC
    SDL3::SDL3-static
    SDL3_shadercross-static
)

target_include_directories(Luminol.Graphics.SDL_GPU PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
