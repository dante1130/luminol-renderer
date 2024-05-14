add_library(Luminol.Graphics
    Camera.cpp
    GraphicsFactory.cpp
    LightManager.cpp
    Renderer.cpp
    RenderableManager.cpp
)

target_compile_features(Luminol.Graphics PRIVATE cxx_std_20)
set_target_properties(Luminol.Graphics PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Graphics PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Graphics PUBLIC
    glm
    GSL
    Luminol.Utilities
)

target_link_libraries(Luminol.Graphics PRIVATE
    Luminol.Graphics.OpenGL
)

target_include_directories(Luminol.Graphics PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)

add_subdirectory(OpenGL)
