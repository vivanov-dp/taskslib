cmake_minimum_required(VERSION 3.21)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/bin)

if (NOT ${MSVC})
    set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wpedantic")
    set(CMAKE_CXX_FLAGS_DEBUG "-Og -ggdb")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

project (TasksLib
         VERSION 1.0.0
         DESCRIPTION "Multithreaded task scheduling library"
         LANGUAGES CXX
    )

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
