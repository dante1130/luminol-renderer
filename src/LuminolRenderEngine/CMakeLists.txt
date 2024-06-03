if(LUMINOL_RENDER_ENGINE_BUILD_DEMO)
    add_subdirectory(Demo)
endif()

add_subdirectory(Graphics)
add_subdirectory(Utilities)
add_subdirectory(Window)

add_library(LuminolRenderEngine
    LuminolRenderEngine.cpp
)

target_compile_features(LuminolRenderEngine PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngine PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(LuminolRenderEngine PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(LuminolRenderEngine PUBLIC
    Luminol.Window
    Luminol.Graphics
    Luminol.Utilities
)

target_include_directories(LuminolRenderEngine PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
