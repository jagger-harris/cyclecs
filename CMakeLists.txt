# Project definitions
cmake_minimum_required(VERSION 3.21)
project(cls C)

# Project options
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Library options
option(CLS_BUILD_EXAMPLES "Build examples" ON)

# Library directory
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

# CGLM
set(CGLM_DIR ${LIB_DIR}/cglm)
add_library(cglm INTERFACE)
target_include_directories(cglm INTERFACE ${CGLM_DIR}/include)

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

# GLFW
set(GLFW_DIR ${LIB_DIR}/glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${GLFW_DIR})

# Glad
set(GLAD_DIR ${LIB_DIR}/glad)
add_library(glad ${GLAD_DIR}/src/gl.c)
target_include_directories(glad SYSTEM PUBLIC ${GLAD_DIR}/include)

# STB
set(STB_DIR ${LIB_DIR}/stb)
add_library(stb INTERFACE)
target_include_directories(stb SYSTEM INTERFACE ${STB_DIR})

# Cyclecs
file(GLOB_RECURSE CLS_SOURCES CONFIGURE_DEPENDS src/*.c)

add_library(cls STATIC ${CLS_SOURCES})

target_include_directories(cls
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(cls PUBLIC cglm freetype glfw glad stb)
target_compile_definitions(cls PUBLIC GLFW_INCLUDE_NONE)

if (MSVC)
    target_compile_options(cls PRIVATE
        $<$<CONFIG:Debug>:/W4 /WX>
    )
else()
    target_compile_options(cls PRIVATE
        $<$<CONFIG:Debug>:-g3 -O0 -Wall -Wextra -Wpedantic -Wmisleading-indentation -Wunused -Wuninitialized -Wshadow -Wconversion -Wdeprecated -Wpointer-arith -Wstrict-aliasing -Werror -fsanitize=address,leak,undefined>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -DENABLE_PROFILER>
    )
    target_link_options(cls PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,leak,undefined>
    )
endif()

set_target_properties(cls PROPERTIES
    C_CLANG_TIDY "$<$<CONFIG:Debug>:clang-tidy;--use-color>"
)

# Compile commands
execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

# Assets
set(ASSETS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(ASSETS_DEST_DIR ${CMAKE_CURRENT_BINARY_DIR}/data)

if (CMAKE_BUILD_TYPE STREQUAL Debug)
    add_custom_target(assets
        COMMAND ${CMAKE_COMMAND} -E create_symlink ${ASSETS_SOURCE_DIR} ${ASSETS_DEST_DIR}
        COMMENT "Creating symbolic link for assets in debug mode"
    )
else()
    add_custom_target(assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${ASSETS_SOURCE_DIR} ${ASSETS_DEST_DIR}
        COMMENT "Copying assets in release mode"
    )
endif()

# Examples
if (CLS_BUILD_EXAMPLES)
    file(GLOB EXAMPLE_SOURCES CONFIGURE_DEPENDS examples/*.c)

    foreach(EXAMPLE_SOURCE ${EXAMPLE_SOURCES})
        get_filename_component(EXAMPLE_NAME ${EXAMPLE_SOURCE} NAME_WE)
        add_executable(${EXAMPLE_NAME} ${EXAMPLE_SOURCE})
        target_link_libraries(${EXAMPLE_NAME} PRIVATE cls)
        add_dependencies(${EXAMPLE_NAME} assets)
        if (NOT MSVC)
            target_link_options(${EXAMPLE_NAME} PRIVATE
                $<$<CONFIG:Debug>:-fsanitize=address,leak,undefined>
            )
        endif()
    endforeach()
endif()
