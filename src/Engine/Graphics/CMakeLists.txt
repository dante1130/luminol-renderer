add_library(Luminol.Graphics
    Shader.cpp
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLVertexBuffer.cpp
    OpenGL/OpenGLVertexArrayObject.cpp
)

target_link_libraries(Luminol.Graphics PUBLIC
    glad
    glm
    GSL
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
