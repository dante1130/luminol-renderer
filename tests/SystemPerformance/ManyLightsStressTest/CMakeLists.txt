add_executable(Luminol.Tests.ManyLightsStressTest)

target_compile_features(Luminol.Tests.ManyLightsStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ManyLightsStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ManyLightsStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ManyLightsStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ManyLightsStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME ManyLightsStressTest
    COMMAND Luminol.Tests.ManyLightsStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(ManyLightsStressTest PROPERTIES LABELS "performance")
