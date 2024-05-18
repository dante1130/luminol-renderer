add_library(Luminol.Graphics.OpenGL
    OpenGLBufferBit.cpp
    OpenGLRenderer.cpp
    OpenGLFactory.cpp
    OpenGLShader.cpp
    OpenGLVertexBuffer.cpp
    OpenGLIndexBuffer.cpp
    OpenGLVertexArrayObject.cpp
    OpenGLTexture.cpp
    OpenGLTextureManager.cpp
    OpenGLTextureFormat.cpp
    OpenGLSkybox.cpp
    OpenGLMesh.cpp
    OpenGLModel.cpp
    OpenGLError.cpp
    OpenGLFrameBuffer.cpp
    OpenGLGBufferRenderPass.cpp
    OpenGLLightingRenderPass.cpp
    OpenGLColorRenderPass.cpp
    OpenGLSkyboxRenderPass.cpp
    OpenGLHDRRenderPass.cpp
    OpenGLComputeRenderPass.cpp
)

target_compile_features(Luminol.Graphics.OpenGL PRIVATE cxx_std_20)
set_target_properties(Luminol.Graphics.OpenGL PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Graphics.OpenGL PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Graphics.OpenGL PRIVATE
    glm
    GSL
    glad
)

target_include_directories(Luminol.Graphics.OpenGL PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
