add_executable(LuminolRenderEngineDemo.InstancedCube)

target_compile_features(LuminolRenderEngineDemo.InstancedCube PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.InstancedCube PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.InstancedCube PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.InstancedCube PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.InstancedCube PRIVATE
    LuminolRenderEngine
)
