add_executable(LuminolRenderEngineDemo.ClusterAabbSmokeTest)

target_compile_features(LuminolRenderEngineDemo.ClusterAabbSmokeTest PRIVATE cxx_std_20)
set_target_properties(LuminolRenderEngineDemo.ClusterAabbSmokeTest PROPERTIES
    CXX_EXTENSIONS OFF
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_compile_options(LuminolRenderEngineDemo.ClusterAabbSmokeTest PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /WX /wd4458>
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -pedantic -Werror>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -pedantic -Werror>
)

target_sources(LuminolRenderEngineDemo.ClusterAabbSmokeTest PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo.ClusterAabbSmokeTest PRIVATE
    GSL
    LuminolMaths
    Luminol.Window
    Luminol.Graphics.SDL_GPU
)
