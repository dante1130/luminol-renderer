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
    NAME gsl
    GITHUB_REPOSITORY microsoft/GSL
    GIT_TAG v4.1.0
    EXCLUDE_FROM_ALL
    SYSTEM
)

CPMAddPackage(
    NAME stb
    GITHUB_REPOSITORY nothings/stb
    GIT_TAG master
    DOWNLOAD_ONLY TRUE
    EXCLUDE_FROM_ALL
    SYSTEM
)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

CPMAddPackage(
    NAME assimp
    GITHUB_REPOSITORY assimp/assimp
    GIT_TAG v5.4.3
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "ASSIMP_BUILD_TESTS OFF"
        "ASSIMP_NO_EXPORT ON"
        "ASSIMP_INSTALL OFF"
        "ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF"
        "ASSIMP_BUILD_FBX_IMPORTER ON"
        "ASSIMP_BUILD_OBJ_IMPORTER ON"
        "ASSIMP_BUILD_GLTF_IMPORTER ON"
    EXCLUDE_FROM_ALL
    SYSTEM
)

CPMAddPackage(
    NAME luminol-maths
    GITHUB_REPOSITORY dante1130/luminol-maths
    GIT_TAG main
    OPTIONS 
        "LUMINOL_MATHS_EXPORT_COMPILE_COMMANDS OFF" 
        "LUMINOL_MATHS_BUILD_DEMO OFF" 
        "LUMINOL_MATHS_BUILD_TESTS OFF"
    EXCLUDE_FROM_ALL
    SYSTEM
)
