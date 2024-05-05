option(LUMINOL_RENDER_ENGINE_BUILD_DEMO "Build the demo for the render engine" ON)

if(LUMINOL_RENDER_ENGINE_BUILD_DEMO)
    add_subdirectory(Demo)
endif()

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
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
