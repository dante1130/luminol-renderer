add_executable(LuminolRenderEngineDemo.Triangle)

target_compile_features(LuminolRenderEngineDemo.Triangle PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.Triangle PROPERTIES 
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.Triangle PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.Triangle PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.Triangle PRIVATE
    LuminolRenderEngine
)
