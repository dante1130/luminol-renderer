add_library(Luminol.Utilities
    ImageLoader.cpp
    ModelLoader.cpp
    Timer.cpp
)

target_compile_features(Luminol.Utilities PRIVATE cxx_std_20)
set_target_properties(Luminol.Utilities PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Utilities PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Utilities PUBLIC
    LuminolMaths
)

target_link_libraries(Luminol.Utilities PRIVATE
    stb
    GSL
    assimp
)

target_include_directories(Luminol.Utilities PUBLIC
    ${LUMINOL_RENDER_ENGINE_SRC_DIR}
)
