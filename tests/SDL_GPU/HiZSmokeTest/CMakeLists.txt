add_executable(Luminol.Tests.HiZSmokeTest)

target_compile_features(Luminol.Tests.HiZSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.HiZSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.HiZSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.HiZSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.HiZSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME HiZSmokeTest
    COMMAND Luminol.Tests.HiZSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
