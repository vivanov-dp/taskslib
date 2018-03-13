set (HEADERS Task.h TasksQueue.h ResourcePool.h)
set (SOURCE Task.cpp TasksQueue.cpp)

add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_compile_features(TasksLib PUBLIC cxx_std_14)
