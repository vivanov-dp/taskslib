# TasksLib Build Instructions #

*Note: We don't provide a dynamic library configuration for TasksLib at this time. If you require dynamic linking, you have to modify the build configurations and the source code accordingly*

You can take a look at [test/CMakeLists.txt](../test/CMakeLists.txt) for an example of how to integrate the build into your own CMake based project using ExternalProject_Add() - we have done this with the googletest/googlemock framework. You will probably want to build only the TasksLib project in this case, not use the ALL_BUILD. 

If you want to build the library stand-alone (and/or the tests for it), keep reading:

- Make sure you have CMake version 3.10 or newer installed

- Clone or download the repo, feel free to use the master branch or any release. If using the master it is recommended to checkout a specific commit by it's ID, rather than to rely on the latest version, because backward incompatible changes may be introduced at any time. Use shallow copy (depth=1) if you only intend to use the library and not to participate in the development.

- Run CMake. 
By convention we put the output of CMake in the build subdirectory, so you need to
  ```
  mkdir build
  cd build
  cmake ..
  ```
  Alternatively use the provided batch script for your platform. Generated project files will appear in the build subdirectory. Compile them according to your platform.

## Windows - VisualStudio ##

### TasksLib library ###

- Open the **TasksLib.sln** solution with VisualStudio.
- Build the **TasksLib** project with `Debug` or `Release` configuration. The compiled static library files will appear in the `build/bin/Debug/` or `build/bin/Release/` directory. You can copy them to your project, or use them where they are, whichever is your preference. Your are all set.

### Tests suite ###

- If you want to build and run the tests suite, build the **GTestExternal** project first - this will download and build google's gtest/gmock suite in a self-contained subdirectory. This project is excluded from the global solution build to save time checking and updating gtest from the internet.
- Build the whole solution, or build the **ALL_BUILD** project.
- Run the tests using VisualStudio's test explorer: `Test->Windows->Test Explorer`, `Run All`. 
- Alternativelly build the **RUN_TESTS** project - it is going to run the test suite at the end of it's build steps.
- After the tests are built, you can also run them from `build/bin/Debug/Test*.exe` or `build/bin/Release/Test*.exe`

## Linux - GCC ##

### TaskLib library ###

- cd build
- make TasksLib - The compiled librabry is in `build/bin/`

### Tests suite ###

- cd build
- make GTestExternal
- make
- make RUN_TESTS

you can also run the tests separately from `build/bin/test*`
