set (HEADERS
        include/taskslib/Types.h include/taskslib/TaskOptions.h include/taskslib/Task.h include/taskslib/TasksThread.h
        include/taskslib/TasksQueue.h include/taskslib/TasksQueuesContainer.h include/taskslib/ResourcePool.h
    )
set (SOURCE TaskOptions.cpp Task.cpp TasksQueue.cpp TasksQueuesContainer.cpp)



add_library(TasksLib STATIC ${HEADERS} ${SOURCE})
target_include_directories(TasksLib PUBLIC include)
target_compile_features(TasksLib PUBLIC cxx_std_17)
