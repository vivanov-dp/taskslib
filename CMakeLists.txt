project (TasksLib 
			VERSION 0.1
			DESCRIPTION "Multithreaded task scheduling library"
			LANGUAGES C++)
			
cmake_minimum_required(VERSION 3.11)

set (HEADERS Task.h TasksQueue.h ResourcePool.h)
set (SOURCE Task.cpp TasksQueue.cpp)

add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_compile_features(TasksLib PUBLIC cxx_std_14)
