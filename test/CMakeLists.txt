include(ExternalProject)
set_property(DIRECTORY PROPERTY EP_BASE externals/cmake)

set(CMAKE_CXX_FLAGS "")
set(CMAKE_CXX_STANDARD 17)

# external project download and build (no install for gtest)
ExternalProject_Add(external-gtest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG release-1.11.0
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
    PREFIX "${GTEST_PREFIX}"

	EXCLUDE_FROM_ALL TRUE

    # cmake arguments (required on msvc)
    CMAKE_ARGS -Dgtest_force_shared_crt=ON

    # Disable steps
	INSTALL_COMMAND ""
	TEST_COMMAND ""

    # Wrap download, configure and build steps in a script to log output
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON

    CMAKE_CACHE_ARGS "-DCMAKE_MAKE_PROGRAM:STRING=${CMAKE_MAKE_PROGRAM}"
    )
ExternalProject_Get_Property(external-gtest SOURCE_DIR)

# This should auto-regenerate the CMake build system to include gtest, but a defect in the bundled Ninja with CLion
# is getting in the way
#ExternalProject_Add_Step(external-gtest rebuild-cmake
#			COMMENT "Rebuilding CMake build system"
#			COMMAND ${CMAKE_COMMAND} -S ${CMAKE_CURRENT_SOURCE_DIR}/.. -B ${CMAKE_CURRENT_BINARY_DIR}/..
#			BYPRODUCTS ${SOURCE_DIR}/CMakeLists.txt
#			DEPENDEES update
#			EXCLUDE_FROM_MAIN
#		)

if(EXISTS ${SOURCE_DIR}/CMakeLists.txt)
	add_subdirectory(${SOURCE_DIR} ${SOURCE_DIR}/dist)

	# Add the tests
	include_directories (
			"${TasksLib_SOURCE_DIR}/src"
			"${SOURCE_DIR}/googletest/include"
			"${SOURCE_DIR}/googlemock/include"
	)
	link_directories(
			"${TasksLib_BINARY_DIR}"
	)

	add_executable(TestTask TestTools.h TestTask.cpp)
	target_link_libraries(TestTask TasksLib gtest_main)

	add_executable(TestTaskOptions TestTools.h TestTaskOptions.cpp)
	target_link_libraries(TestTaskOptions TasksLib gtest_main)

	add_executable(TestResourcePool TestTools.h TestResourcePool.cpp)
	target_link_libraries(TestResourcePool TasksLib gmock_main)

	add_executable(TestTasksThread TestTools.h TestTasksThread.cpp)
	target_link_libraries(TestTasksThread TasksLib gmock_main)

	add_executable(TestTasksQueue TestTools.h TestTasksQueue.cpp)
	target_link_libraries(TestTasksQueue TasksLib gmock_main)

	add_executable(TestTasksQueueContainer TestTools.h TestTasksQueueContainer.cpp)
	target_link_libraries(TestTasksQueueContainer TasksLib gmock_main)

	add_test(NAME TestTask COMMAND TestTask)
	add_test(NAME TestTaskOptions COMMAND TestTaskOptions)
	add_test(NAME TestResourcePool COMMAND TestResourcePool)
	add_test(NAME TestTasksThread COMMAND TestTasksThread)
	add_test(NAME TestTasksQueue COMMAND TestTasksQueue)
	add_test(NAME TestTasksQueueContainer COMMAND TestTasksQueueContainer)
endif()
