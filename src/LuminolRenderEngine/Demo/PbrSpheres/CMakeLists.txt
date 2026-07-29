add_executable(LuminolRenderEngineDemo.PbrSpheres)

target_compile_features(LuminolRenderEngineDemo.PbrSpheres PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.PbrSpheres PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.PbrSpheres PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.PbrSpheres PRIVATE
    main.cpp
    SphereMesh.cpp
)

target_link_libraries(LuminolRenderEngineDemo.PbrSpheres PRIVATE
    LuminolRenderEngine
)
