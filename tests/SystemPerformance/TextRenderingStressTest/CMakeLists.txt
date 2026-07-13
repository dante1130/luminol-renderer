add_executable(Luminol.Tests.TextRenderingStressTest)

target_compile_features(Luminol.Tests.TextRenderingStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.TextRenderingStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.TextRenderingStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.TextRenderingStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.TextRenderingStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME TextRenderingStressTest
    COMMAND Luminol.Tests.TextRenderingStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(TextRenderingStressTest PROPERTIES LABELS "performance")
