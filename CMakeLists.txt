# Project definitions
cmake_minimum_required(VERSION 3.21)
project(executable C)

# Project options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Get all sources
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.c)

# Create executable and add dependencies
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Clang-tidy only for project
set_target_properties(${PROJECT_NAME} PROPERTIES
    C_CLANG_TIDY $<$<CONFIG:Debug>:clang-tidy>
)

# Compiler and build types
if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:/W4 /WX>
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-g3 -O0 -Wall -Wextra -Wpedantic -Wmisleading-indentation -Wunused -Wuninitialized -Wshadow -Wconversion -Wdeprecated -Wpointer-arith -Wstrict-aliasing -Werror -fsanitize=address,leak,undefined>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -DENABLE_PROFILER>
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,leak,undefined>
    )
endif()

# Symlink / copy assets
set(ASSETS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(ASSETS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
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

# CGLM
set(CGLM_DIR ${LIB_DIR}/cglm)
add_library(cglm INTERFACE)
target_include_directories(cglm INTERFACE ${CGLM_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE cglm)

# FreeType
set(FREETYPE_DIR ${LIB_DIR}/freetype)
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(FT_WITH_ZLIB OFF CACHE BOOL "" FORCE)
set(FT_WITH_BZIP2 OFF CACHE BOOL "" FORCE)
set(FT_WITH_PNG OFF CACHE BOOL "" FORCE)
set(FT_WITH_HARFBUZZ OFF CACHE BOOL "" FORCE)
set(FT_WITH_BROTLI OFF CACHE BOOL "" FORCE)
add_subdirectory(${FREETYPE_DIR} ${CMAKE_BINARY_DIR}/freetype_build)
target_link_libraries(${PROJECT_NAME} PRIVATE freetype)

# GLFW
set(GLFW_DIR ${LIB_DIR}/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory("${GLFW_DIR}")
target_link_libraries(${PROJECT_NAME} PRIVATE glfw)
target_compile_definitions(${PROJECT_NAME} PRIVATE GLFW_INCLUDE_NONE)

# Glad
set(GLAD_DIR ${LIB_DIR}/glad)
add_library(glad ${GLAD_DIR}/src/gl.c)
target_include_directories(glad SYSTEM PUBLIC ${GLAD_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE glad)

# STB
set(STB_DIR ${LIB_DIR}/stb)
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${STB_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE stb)
