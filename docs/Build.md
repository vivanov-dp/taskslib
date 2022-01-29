# TasksLib Build Instructions #

*Note: We don't provide a dynamic library configuration for TasksLib at 
this time. If you require dynamic linking, you have to modify the build 
configurations and the source code accordingly*

You can take a look at [test/CMakeLists.txt](../test/CMakeLists.txt) for 
an example of how to integrate the build into your own CMake based project 
using ExternalProject_Add() - we have done this with the 
googletest/googlemock framework. You will probably want to build only 
the TasksLib target in this case, not all.

Alternatively just clone the source in a subfolder in your project, or
add it as a Git submodule and use CMake's `add_subdirectory()` to include it
in your build. Include 'taskslib/src' in this case, not the root taskslib
folder which will draw in the targets for testing and the gtest framework.

For example if you clone it in a subfolder named `externals/taskslib`, you 
would want to add the following line to CmakeFiles.txt: 

  `add_subdirectory(externals/taskslib/src)`

and then use the `TasksLib` target to add as a library to your targets, like
this:

  `target_link_libraries(<your_target> PRIVATE TasksLib)`

## Stand-alone build 

If you want to build the library stand-alone (and/or the tests for it):

- Make sure you have CMake version 3.10 or newer installed

- Clone or download the repo, feel free to use the master branch or any 
release.

- Run CMake. 
  By convention we put the output of CMake in a subdirectory, so you need to
  ```
  mkdir cmake-build-debug-win64-gcc
  cd cmake-build-debug-win64-gcc
  cmake .. -GNinja
  ```
  Alternatively check the CMake documentation for what parameters you can 
  use to select the build system that gets generated.

## Build TasksLib library ##

Use the **TasksLib** target to build

## Tests suite ##

- If you want to build and run the tests suite, build the 
  `external-gtest` target first - this will download and build google's 
  gtest/gmock suite in a self-contained subdirectory. This target is 
  excluded from the global solution build to save time checking and 
  updating gtest from the internet.
- Regenerate the build system using CMake. This is required to pick up the
  gtest build and add the tests. It should happen automatically, but there
  is a bug in the build tools that prevents this from working with Ninja and
  MinGW.
- Run the tests using the `All CTest` target (`RUN TESTS` on MSVC), or 
  manually from the `dist/bin` folder.
