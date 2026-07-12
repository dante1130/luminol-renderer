add_executable(Luminol.Tests.InstanceCullSmokeTest)

target_compile_features(Luminol.Tests.InstanceCullSmokeTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.InstanceCullSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.InstanceCullSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.InstanceCullSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.InstanceCullSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics
    Luminol.Graphics.SDL_GPU
)

add_test(
    NAME InstanceCullSmokeTest
    COMMAND Luminol.Tests.InstanceCullSmokeTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
