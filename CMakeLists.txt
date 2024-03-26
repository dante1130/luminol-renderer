cmake_minimum_required(VERSION 3.25.1)

project(glfw-opengl-cmake-starter)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(RES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/res)
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(${LIB_DIR})

if(MSVC)
	set(CMAKE_WARNING_FLAGS 
        # Enable all warnings.
        /W4
        # Treat warnings as errors.
        /WX

        # Disable warnings about name shadowing.
        # this-> naming convention is used for members 
        # without any prefix so this warning generates 
        # a lot of noise.
        /wd4458
	)
else()
    set(CMAKE_WARNING_FLAGS -Wall -Wextra -Wpedantic -Werror)
endif()

add_compile_options(${CMAKE_WARNING_FLAGS})

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

