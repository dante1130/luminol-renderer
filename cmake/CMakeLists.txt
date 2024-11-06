include(get_cpm.cmake)

CPMAddPackage(
    NAME glfw
    GITHUB_REPOSITORY glfw/glfw
    GIT_TAG 3.4
    OPTIONS 
        "GLFW_BUILD_EXAMPLES OFF" 
        "GLFW_BUILD_TESTS OFF" 
        "GLFW_BUILD_DOCS OFF" 
        "GLFW_INSTALL OFF"
    EXCLUDE_FROM_ALL
    SYSTEM
)

CPMAddPackage(
    NAME glad
    GITHUB_REPOSITORY Dav1dde/glad
    GIT_TAG v2.0.8
    DOWNLOAD_ONLY TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)

add_subdirectory(${glad_SOURCE_DIR}/cmake ${LUMINOL_RENDER_ENGINE_ROOT_DIR} EXCLUDE_FROM_ALL SYSTEM)
glad_add_library(glad STATIC API gl:core=4.6)

CPMAddPackage(
    NAME glm
    GITHUB_REPOSITORY g-truc/glm
    GIT_TAG 1.0.1
    OPTIONS "GLM_ENABLE_CXX_20 ON"
    EXCLUDE_FROM_ALL
    SYSTEM
)

CPMAddPackage(
    NAME gsl
    GITHUB_REPOSITORY microsoft/GSL
    GIT_TAG v4.1.0
    EXCLUDE_FROM_ALL
    SYSTEM
)
