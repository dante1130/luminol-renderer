add_executable(LuminolRenderEngineDemo.TexturedModel)

target_compile_features(LuminolRenderEngineDemo.TexturedModel PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.TexturedModel PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.TexturedModel PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.TexturedModel PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.TexturedModel PRIVATE
    LuminolRenderEngine
)
