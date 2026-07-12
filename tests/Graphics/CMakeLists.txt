add_executable(Luminol.Graphics.Tests
    FrustumTests.cpp
)

target_compile_features(Luminol.Graphics.Tests PRIVATE cxx_std_20)
set_target_properties(Luminol.Graphics.Tests PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Graphics.Tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458 /wd5285>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Graphics.Tests PRIVATE
    Luminol.Graphics
    doctest::doctest
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(Luminol.Graphics.Tests)
