add_executable(LuminolRenderEngineDemo.TexturedQuad)

target_compile_features(LuminolRenderEngineDemo.TexturedQuad PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.TexturedQuad PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.TexturedQuad PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.TexturedQuad PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.TexturedQuad PRIVATE
    LuminolRenderEngine
)
