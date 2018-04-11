set (HEADERS Types.h TaskOptions.h Task.h TasksThread.h TasksQueue.h TasksQueueContainer.h ResourcePool.h)
set (SOURCE TaskOptions.cpp Task.cpp TasksQueue.cpp TasksQueueContainer.cpp)

add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_compile_features(TasksLib PUBLIC cxx_std_17)
