add_executable(Luminol.Utilities.Tests
    ImageLoaderTests.cpp
    ModelLoaderTests.cpp
)

target_compile_features(Luminol.Utilities.Tests PRIVATE cxx_std_20)
set_target_properties(Luminol.Utilities.Tests PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(Luminol.Utilities.Tests PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458 /wd5285>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_link_libraries(Luminol.Utilities.Tests PRIVATE
    Luminol.Utilities
    doctest::doctest
)

include(${doctest_SOURCE_DIR}/scripts/cmake/doctest.cmake)
doctest_discover_tests(Luminol.Utilities.Tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
