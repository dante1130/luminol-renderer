add_executable(LuminolRenderEngineDemo)

target_compile_features(LuminolRenderEngineDemo PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo PROPERTIES CXX_EXTENSIONS OFF)

target_compile_options(LuminolRenderEngineDemo PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo PRIVATE
    LuminolRenderEngine
)
