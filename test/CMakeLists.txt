include_directories (${TasksLib_SOURCE_DIR}/src)

add_executable(TestTask test_task.cpp)
target_link_libraries(TestTask TasksLib)

add_test(NAME Task COMMAND TestTask)
