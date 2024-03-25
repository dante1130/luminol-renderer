cmake_minimum_required(VERSION 3.25.1)

project(glfw-opengl-cmake-starter)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(${LIB_DIR})
add_subdirectory(${SRC_DIR})

add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_CURRENT_LIST_DIR}
)

add_custom_target(
    copy-resources ALL
    ${CMAKE_COMMAND} -E copy_directory
        ${RES_DIR}
        ${CMAKE_BINARY_DIR}/res
)

