include_directories (${TasksLib_SOURCE_DIR}/src)

add_executable(TestTask test_task.cpp)
target_link_libraries(TestTask TasksLib GTestMain)
target_compile_features(TestTask PUBLIC cxx_std_17)

add_test(NAME TestTask COMMAND TestTask)
