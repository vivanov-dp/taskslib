set (HEADERS Types.h TaskOptions.h Task.h TasksThread.h TasksQueue.h TasksQueuesContainer.h ResourcePool.h)
set (SOURCE TaskOptions.cpp Task.cpp TasksQueue.cpp TasksQueuesContainer.cpp)

add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_compile_features(TasksLib PUBLIC cxx_std_17)
