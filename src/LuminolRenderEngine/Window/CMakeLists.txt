add_library(Luminol.Window
    Window.cpp
)

target_compile_features(Luminol.Window PRIVATE cxx_std_20)
set_target_properties(Luminol.Window PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Window PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)


target_link_libraries(Luminol.Window PRIVATE
    glfw
    GSL
)

target_compile_definitions(Luminol.Window PRIVATE
    GLFW_INCLUDE_NONE
)
