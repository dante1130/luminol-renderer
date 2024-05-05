add_executable(Application)

target_sources(Application PRIVATE
    main.cpp
)

target_link_libraries(Application PRIVATE
    LuminolRenderEngine
)
