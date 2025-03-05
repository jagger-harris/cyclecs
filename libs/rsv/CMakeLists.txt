# Project definitions
cmake_minimum_required(VERSION 3.10)
project(reservoir C)

# Project options
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Get all sources
file(GLOB_RECURSE SOURCE_FILES tests/*.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

if(MSVC)
  target_compile_options(${PROJECT_NAME} PRIVATE /W4 /WX)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Library
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(rsv_test PRIVATE ${LIB_DIR})

# CTest
add_test(NAME AllTests COMMAND ${PROJECT_NAME})
set_tests_properties(AllTests PROPERTIES FAIL_REGULAR_EXPRESSION failed)
enable_testing()
