add_executable(Luminol.Tests.ComputeSmokeTest)

target_compile_features(Luminol.Tests.ComputeSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ComputeSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ComputeSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ComputeSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ComputeSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME ComputeSmokeTest
    COMMAND Luminol.Tests.ComputeSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
