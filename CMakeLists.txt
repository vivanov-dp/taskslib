cmake_minimum_required(VERSION 3.11)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

project (TasksLib 
			VERSION 0.1
			DESCRIPTION "Multithreaded task scheduling library"
			LANGUAGES CXX)
			
set (HEADERS src/Task.h src/TasksQueue.h src/ResourcePool.h)
set (SOURCE src/Task.cpp src/TasksQueue.cpp)

add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_compile_features(TasksLib PUBLIC cxx_std_14)
