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
