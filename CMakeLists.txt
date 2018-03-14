cmake_minimum_required(VERSION 3.10)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project (TasksLib 
			VERSION 0.1.0.0
			DESCRIPTION "Multithreaded task scheduling library"
			LANGUAGES CXX)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
			