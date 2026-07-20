add_executable(Luminol.Tests.LodSelectionSmokeTest)

target_compile_features(Luminol.Tests.LodSelectionSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.LodSelectionSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.LodSelectionSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.LodSelectionSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.LodSelectionSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME LodSelectionSmokeTest
    COMMAND Luminol.Tests.LodSelectionSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
