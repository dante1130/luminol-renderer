add_executable(LuminolRenderEngineDemo)

target_sources(LuminolRenderEngineDemo PRIVATE
    main.cpp
)

target_link_libraries(LuminolRenderEngineDemo PRIVATE
    LuminolRenderEngine
)
