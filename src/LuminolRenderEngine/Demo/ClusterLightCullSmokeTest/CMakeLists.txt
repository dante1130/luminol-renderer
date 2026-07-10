add_executable(LuminolRenderEngineDemo.ClusterLightCullSmokeTest)

target_compile_features(LuminolRenderEngineDemo.ClusterLightCullSmokeTest PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.ClusterLightCullSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.ClusterLightCullSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.ClusterLightCullSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.ClusterLightCullSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)
