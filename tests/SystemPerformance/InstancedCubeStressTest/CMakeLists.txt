add_executable(Luminol.Tests.InstancedCubeStressTest)

target_compile_features(Luminol.Tests.InstancedCubeStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.InstancedCubeStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.InstancedCubeStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.InstancedCubeStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.InstancedCubeStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME InstancedCubeStressTest
    COMMAND Luminol.Tests.InstancedCubeStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(InstancedCubeStressTest PROPERTIES LABELS "performance")
