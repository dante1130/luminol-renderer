add_subdirectory(Graphics)
add_subdirectory(Utilities)
add_subdirectory(Window)

add_library(LuminolRenderEngine
    LuminolRenderEngine.cpp
)

target_link_libraries(LuminolRenderEngine PUBLIC
    glm
    Luminol.Window
    Luminol.Graphics
    Luminol.Utilities
)

target_include_directories(LuminolRenderEngine PUBLIC
    ${SRC_DIR}
)
