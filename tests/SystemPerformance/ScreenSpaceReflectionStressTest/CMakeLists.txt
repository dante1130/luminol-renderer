add_executable(Luminol.Tests.ScreenSpaceReflectionStressTest)

target_compile_features(Luminol.Tests.ScreenSpaceReflectionStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ScreenSpaceReflectionStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ScreenSpaceReflectionStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ScreenSpaceReflectionStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ScreenSpaceReflectionStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME ScreenSpaceReflectionStressTest
    COMMAND Luminol.Tests.ScreenSpaceReflectionStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(ScreenSpaceReflectionStressTest PROPERTIES LABELS "performance")
