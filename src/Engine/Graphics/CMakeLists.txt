add_library(Luminol.Graphics
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
    OpenGL/OpenGLVertexBuffer.cpp
    OpenGL/OpenGLIndexBuffer.cpp
    OpenGL/OpenGLVertexArrayObject.cpp
    OpenGL/OpenGLMesh.cpp
)

target_link_libraries(Luminol.Graphics PUBLIC
    glad
    glm
    GSL
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
