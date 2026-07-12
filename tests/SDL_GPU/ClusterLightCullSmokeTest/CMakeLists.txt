add_executable(Luminol.Tests.ClusterLightCullSmokeTest)

target_compile_features(Luminol.Tests.ClusterLightCullSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ClusterLightCullSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ClusterLightCullSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ClusterLightCullSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ClusterLightCullSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME ClusterLightCullSmokeTest
    COMMAND Luminol.Tests.ClusterLightCullSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
