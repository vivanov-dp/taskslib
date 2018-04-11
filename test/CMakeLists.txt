include(ExternalProject)

# variables to help keep track of gtest paths
set(GTEST_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/gtest")
set(GTEST_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build/googlemock/gtest")
set(GMOCK_LOCATION "${GTEST_PREFIX}/src/GTestExternal-build/googlemock/")
set(GTEST_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/googletest/include")
set(GMOCK_INCLUDES "${GTEST_PREFIX}/src/GTestExternal/googlemock/include")

# external project download and build (no install for gtest)
ExternalProject_Add(GTestExternal
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG master
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
    )
set_target_properties(GTestExternal PROPERTIES EXCLUDE_FROM_DEFAULT_BUILD TRUE)

# variables defining the import location properties for the generated gtest* libraries
if (MSVC)
    set(GTEST_IMPORTED_LOCATION
        IMPORTED_LOCATION_DEBUG           "${GTEST_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtestd${CMAKE_STATIC_LIBRARY_SUFFIX}"
        IMPORTED_LOCATION_RELEASE         "${GTEST_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    set(GTESTMAIN_IMPORTED_LOCATION
        IMPORTED_LOCATION_DEBUG           "${GTEST_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_maind${CMAKE_STATIC_LIBRARY_SUFFIX}"
        IMPORTED_LOCATION_RELEASE         "${GTEST_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    set(GMOCK_IMPORTED_LOCATION
        IMPORTED_LOCATION_DEBUG           "${GMOCK_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gmockd${CMAKE_STATIC_LIBRARY_SUFFIX}"
        IMPORTED_LOCATION_RELEASE         "${GMOCK_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
    set(GMOCKMAIN_IMPORTED_LOCATION
        IMPORTED_LOCATION_DEBUG           "${GMOCK_LOCATION}/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}gmock_maind${CMAKE_STATIC_LIBRARY_SUFFIX}"
        IMPORTED_LOCATION_RELEASE         "${GMOCK_LOCATION}/Release/${CMAKE_STATIC_LIBRARY_PREFIX}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX}"
        )
else()
    set(GTEST_IMPORTED_LOCATION
        IMPORTED_LOCATION                 "${GTEST_LOCATION}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(GTESTMAIN_IMPORTED_LOCATION
        IMPORTED_LOCATION                 "${GTEST_LOCATION}/${CMAKE_STATIC_LIBRARY_PREFIX}gtest_main${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(GMOCK_IMPORTED_LOCATION
        IMPORTED_LOCATION                 "${GMOCK_LOCATION}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(GMOCKMAIN_IMPORTED_LOCATION
        IMPORTED_LOCATION                 "${GMOCK_LOCATION}/${CMAKE_STATIC_LIBRARY_PREFIX}gmock_main${CMAKE_STATIC_LIBRARY_SUFFIX}")
endif()

# the gtest include directories exist only after they are build, but they are used/needed
# for the set_target_properties calls below, so make them to avoid an error
file(MAKE_DIRECTORY ${GTEST_INCLUDES})
file(MAKE_DIRECTORY ${GMOCK_INCLUDES})

# define imported library GTest
add_library(GTest IMPORTED STATIC GLOBAL)
set_target_properties(GTest PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES     "${GTEST_INCLUDES}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
    ${GTEST_IMPORTED_LOCATION}
    )

# define imported library GTestMain
add_library(GTestMain IMPORTED STATIC GLOBAL)
set_target_properties(GTestMain PROPERTIES
    IMPORTED_LINK_INTERFACE_LIBRARIES GTest
    ${GTESTMAIN_IMPORTED_LOCATION}
    )

# define imported library GMock
add_library(GMock IMPORTED STATIC GLOBAL)
set_target_properties(GMock PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES     "${GTEST_INCLUDES};${GMOCK_INCLUDES}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
    ${GMOCK_IMPORTED_LOCATION}
    )

# define imported library GMockMain
add_library(GMockMain IMPORTED STATIC GLOBAL)
set_target_properties(GMockMain PROPERTIES
    IMPORTED_LINK_INTERFACE_LIBRARIES GMock
    ${GMOCKMAIN_IMPORTED_LOCATION}
    )

# make GTest&GMock depend on GTestExternal
add_dependencies(GTest GTestExternal)
add_dependencies(GMock GTestExternal)



# Finally add the tests
include_directories (${TasksLib_SOURCE_DIR}/src)

add_executable(TestTask TestTools.h TestTask.cpp)
target_link_libraries(TestTask TasksLib GTestMain)
target_compile_features(TestTask PUBLIC cxx_std_17)

add_executable(TestTaskOptions TestTools.h TestTaskOptions.cpp)
target_link_libraries(TestTaskOptions TasksLib GTestMain)
target_compile_features(TestTaskOptions PUBLIC cxx_std_17)

add_executable(TestResourcePool TestTools.h TestResourcePool.cpp)
target_link_libraries(TestResourcePool TasksLib GMockMain)
target_compile_features(TestResourcePool PUBLIC cxx_std_17)

add_executable(TestTasksThread TestTools.h TestTasksThread.cpp)
target_link_libraries(TestTasksThread TasksLib GMockMain)
target_compile_features(TestTasksThread PUBLIC cxx_std_17)

add_executable(TestTasksQueue TestTools.h TestTasksQueue.cpp)
target_link_libraries(TestTasksQueue TasksLib GMockMain)
target_compile_features(TestTasksQueue PUBLIC cxx_std_17)

add_executable(TestTasksQueueContainer TestTools.h TestTasksQueueContainer.cpp)
target_link_libraries(TestTasksQueueContainer TasksLib GMockMain)
target_compile_features(TestTasksQueueContainer PUBLIC cxx_std_17)

add_test(NAME TestTask COMMAND TestTask)
add_test(NAME TestTaskOptions COMMAND TestTaskOptions)
add_test(NAME TestResourcePool COMMAND TestResourcePool)
add_test(NAME TestTasksThread COMMAND TestTasksThread)
add_test(NAME TestTasksQueue COMMAND TestTasksQueue)
add_test(NAME TestTasksQueueContainer COMMAND TestTasksQueueContainer)
