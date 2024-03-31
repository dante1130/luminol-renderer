add_library(Luminol.Utilities
    ImageLoader.cpp
    Timer.cpp
)

target_link_libraries(Luminol.Utilities PRIVATE stb)

