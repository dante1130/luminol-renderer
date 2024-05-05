add_library(Luminol.Utilities
    ImageLoader.cpp
    ModelLoader.cpp
    Timer.cpp
)

target_link_libraries(Luminol.Utilities PRIVATE 
    stb
    glm
    GSL
    assimp
)

target_include_directories(Luminol.Utilities PUBLIC
    ${SRC_DIR}
)

