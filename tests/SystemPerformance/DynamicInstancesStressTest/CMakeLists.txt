add_executable(Luminol.Tests.DynamicInstancesStressTest)

target_compile_features(Luminol.Tests.DynamicInstancesStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.DynamicInstancesStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.DynamicInstancesStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.DynamicInstancesStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.DynamicInstancesStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME DynamicInstancesStressTest
    COMMAND Luminol.Tests.DynamicInstancesStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(DynamicInstancesStressTest PROPERTIES LABELS "performance")
