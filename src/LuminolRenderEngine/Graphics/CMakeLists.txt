add_library(Luminol.Graphics
    Camera.cpp
    GraphicsFactory.cpp
    LightManager.cpp
    Renderer.cpp
    OpenGL/OpenGLBufferBit.cpp
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLVertexBuffer.cpp
    OpenGL/OpenGLIndexBuffer.cpp
    OpenGL/OpenGLVertexArrayObject.cpp
    OpenGL/OpenGLTexture.cpp
    OpenGL/OpenGLTextureManager.cpp
    OpenGL/OpenGLTextureFormat.cpp
    OpenGL/OpenGLSkybox.cpp
    OpenGL/OpenGLMesh.cpp
    OpenGL/OpenGLModel.cpp
    OpenGL/OpenGLError.cpp
    OpenGL/OpenGLFrameBuffer.cpp
    OpenGL/OpenGLGBufferRenderPass.cpp
    OpenGL/OpenGLLightingRenderPass.cpp
    OpenGL/OpenGLColorRenderPass.cpp
    OpenGL/OpenGLHDRRenderPass.cpp
)

target_compile_features(Luminol.Graphics PRIVATE cxx_std_20)
set_target_properties(Luminol.Graphics PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Graphics PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Graphics PUBLIC
    glad
    glm
    GSL
    Luminol.Utilities
)

target_include_directories(Luminol.Graphics PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
