add_library(Luminol.Graphics
    OpenGL/OpenGLRenderer.cpp
)

target_link_libraries(Luminol.Graphics PRIVATE
    glad
    glm
)

target_include_directories(Luminol.Graphics PUBLIC
    ${SRC_DIR}
)
