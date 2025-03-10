# Project definitions
cmake_minimum_required(VERSION 3.21)
project(executable C)

# Project options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Get all sources
file(GLOB_RECURSE HEADER_FILES src/*.h)
file(GLOB_RECURSE SOURCE_FILES src/*.c)

# Create executable and add dependencies
add_executable(${PROJECT_NAME} ${HEADER_FILES} ${SOURCE_FILES})

# Compiler and build types
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/W4 /WX>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g3 -O0 -Wall -Wextra -Wdeprecated -Wpointer-arith -Wstrict-aliasing -Wpedantic -Wstrict-aliasing -Werror>
        $<$<CONFIG:Release>:-O2 -DNDEBUG>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )
endif()

# Symlink / copy assets
set(ASSETS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(ASSETS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

if (CMAKE_BUILD_TYPE STREQUAL debug)
    add_custom_target(ASSETS
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${ASSETS_SOURCE_DIR} ${ASSETS_DEST_DIR}
        COMMENT "Creating symbolic link for assets in debug mode"
    )
else()
    add_custom_target(ASSETS
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SOURCE_DIR} ${ASSETS_DEST_DIR}
        COMMENT "Copying assets in release mode"
    )
endif()

add_dependencies(${PROJECT_NAME} ASSETS)

# Library directory
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# GLFW
set(GLFW_DIR ${LIB_DIR}/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLFW_DIR}/include)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)

# Glad
set(GLAD_DIR ${LIB_DIR}/glad)
add_library(glad ${GLAD_DIR}/src/gl.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${GLAD_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

# Reservoir
set(RSV_DIR ${LIB_DIR}/rsv)
add_library(rsv INTERFACE)
target_include_directories(rsv INTERFACE ${RSV_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE rsv)

# STB
set(STB_DIR ${LIB_DIR}/stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${STB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE stb)
