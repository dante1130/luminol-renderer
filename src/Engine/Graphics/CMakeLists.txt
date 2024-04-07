add_library(Luminol.Graphics
    Camera.cpp
    GraphicsFactory.cpp
    LightManager.cpp
    Renderer.cpp
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLVertexBuffer.cpp
    OpenGL/OpenGLIndexBuffer.cpp
    OpenGL/OpenGLVertexArrayObject.cpp
    OpenGL/OpenGLTexture.cpp
    OpenGL/OpenGLTextureManager.cpp
    OpenGL/OpenGLSkybox.cpp
    OpenGL/OpenGLMesh.cpp
    OpenGL/OpenGLModel.cpp
    OpenGL/OpenGLError.cpp
    OpenGL/OpenGLFrameBuffer.cpp
)

target_link_libraries(Luminol.Graphics PUBLIC
    glad
    glm
    GSL
    Luminol.Utilities
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
