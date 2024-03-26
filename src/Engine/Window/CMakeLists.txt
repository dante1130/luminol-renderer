add_library(Luminol.Window
    Window.cpp
)

target_link_libraries(Luminol.Window PRIVATE
    glfw
)
