add_executable(Application)

target_sources(Application PRIVATE
    main.cpp
)

target_link_libraries(Application PRIVATE
    glad
    glm
    Luminol.Window
)

target_include_directories(Application PRIVATE
    ${SRC_DIR}
)

