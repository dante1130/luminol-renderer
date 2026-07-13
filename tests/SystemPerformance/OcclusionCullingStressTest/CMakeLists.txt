add_executable(Luminol.Tests.OcclusionCullingStressTest)

target_compile_features(Luminol.Tests.OcclusionCullingStressTest PRIVATE cxx_std_20)
set_target_properties(Luminol.Tests.OcclusionCullingStressTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(Luminol.Tests.OcclusionCullingStressTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(Luminol.Tests.OcclusionCullingStressTest PRIVATE
    main.cpp
)

target_link_libraries(Luminol.Tests.OcclusionCullingStressTest PRIVATE
    LuminolRenderEngine
)

add_test(
    NAME OcclusionCullingStressTest
    COMMAND Luminol.Tests.OcclusionCullingStressTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
set_tests_properties(OcclusionCullingStressTest PROPERTIES LABELS "performance")
