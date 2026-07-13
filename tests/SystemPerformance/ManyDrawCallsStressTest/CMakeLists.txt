add_executable(Luminol.Tests.ManyDrawCallsStressTest)

target_compile_features(Luminol.Tests.ManyDrawCallsStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.ManyDrawCallsStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.ManyDrawCallsStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.ManyDrawCallsStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.ManyDrawCallsStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME ManyDrawCallsStressTest
    COMMAND Luminol.Tests.ManyDrawCallsStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(ManyDrawCallsStressTest PROPERTIES LABELS "performance")
