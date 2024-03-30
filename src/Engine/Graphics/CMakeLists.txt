add_library(Luminol.Graphics
    Camera.cpp
    GraphicsFactory.cpp
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLVertexBuffer.cpp
    OpenGL/OpenGLIndexBuffer.cpp
    OpenGL/OpenGLVertexArrayObject.cpp
    OpenGL/OpenGLTexture.cpp
    OpenGL/OpenGLMesh.cpp
    OpenGL/OpenGLError.cpp
)

target_link_libraries(Luminol.Graphics PUBLIC
    glad
    glm
    GSL
    Luminol.Utilities.ImageLoader
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
