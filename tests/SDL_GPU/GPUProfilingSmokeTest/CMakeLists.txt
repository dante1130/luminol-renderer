add_executable(Luminol.Tests.GPUProfilingSmokeTest)

target_compile_features(Luminol.Tests.GPUProfilingSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.GPUProfilingSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.GPUProfilingSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.GPUProfilingSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.GPUProfilingSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME GPUProfilingSmokeTest
    COMMAND Luminol.Tests.GPUProfilingSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
