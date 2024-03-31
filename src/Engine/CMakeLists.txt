add_subdirectory(Window)
add_subdirectory(Graphics)
add_subdirectory(Utilities)

add_library(LuminolEngine
    LuminolEngine.cpp
)

target_link_libraries(LuminolEngine PUBLIC
    glm
    Luminol.Window
    Luminol.Graphics
    Luminol.Utilities
)

target_include_directories(LuminolEngine PRIVATE
    ${SRC_DIR}
)
