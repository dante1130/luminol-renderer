add_executable(Luminol.Tests.CullingUtilsSmokeTest)

target_compile_features(Luminol.Tests.CullingUtilsSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.CullingUtilsSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.CullingUtilsSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.CullingUtilsSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.CullingUtilsSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME CullingUtilsSmokeTest
    COMMAND Luminol.Tests.CullingUtilsSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
