add_library(Luminol.Graphics
    Shader.cpp
    OpenGL/OpenGLRenderer.cpp
    OpenGL/OpenGLFactory.cpp
    OpenGL/OpenGLShader.cpp
)

target_link_libraries(Luminol.Graphics PRIVATE
    glad
    glm
    GSL
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
