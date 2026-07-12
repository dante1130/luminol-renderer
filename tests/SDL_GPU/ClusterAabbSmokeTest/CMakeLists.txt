add_executable(Luminol.Tests.ClusterAabbSmokeTest)

target_compile_features(Luminol.Tests.ClusterAabbSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ClusterAabbSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ClusterAabbSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ClusterAabbSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ClusterAabbSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME ClusterAabbSmokeTest
    COMMAND Luminol.Tests.ClusterAabbSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
