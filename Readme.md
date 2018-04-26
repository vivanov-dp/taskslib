# Tasks Lib #

This is a very simple tasks scheduler library, using thread pools.

It is written with C++17 specs, has no dependencies on any external libraries and serves the singular purpose of running executable pieces of code multi-threaded, while avoiding creation and destruction of threads.

## Small Footprint, Multiplatform And Production Ready ##

TasksLib was created as part of the development of a mobile game which is being released on the iOS, Android and Windows platforms. The library is used extensively both in the client and the in the server, which is a Linux application. There are some structural differences between the version that is used in the game and this project, mainly because we tend to use game engine objects there when available, and we write C++11 code for the game, but most of the code is the same and is in daily use.

## How To Build ##

*Note: We don't provide a dynamic library configuration for TasksLib at this time. If you require dynamic linking, you have to modify the build configurations and the source code accordingly*

- Clone or download the repo, use the master branch. Use shallow copy (depth=1) if you only intend to use the library and not to participate in the development.

- Run CMake. 
By convention we put the output of CMake in the build subdirectory, so you need to
  ```
  mkdir build
  cd build
  cmake ..
  ```
  Alternatively use the provided batch script for your platform (*currenly there is only the windows/visual studio 2017 one*). Generated project files will appear in the build subdirectory. Compile them according to your platform:

### Windows - VisualStudio ###

#### TasksLib library ####

- Open the **TasksLib.sln** solution with VisualStudio.
- Build the **TasksLib** project with `Debug` or `Release` configuration. The compiled static library files will appear in the `build/bin/Debug/` or `build/bin/Release/` directory. You can copy them to your project, or use them where they are, whichever is your preference. Your are all set.

#### Tests suite ####

- If you want to build and run the tests suite, build the **GTestExternal** project first - this will download and build google's gtest/gmock suite in a self-contained subdirectory. This project is excluded from the global solution build to save time checking and updating gtest from the internet.
- Build the whole solution, or build the **ALL_BUILD** project.
- Run the tests using VisualStudio's test explorer: `Test->Windows->Test Explorer`, `Run All`. 
- Alternativelly build the **RUN_TESTS** project - it is going to run the test suite at the end of it's build steps.
- After the tests are built, you can also run them from `build/bin/Debug/Test*.exe` or `build/bin/Release/Test*.exe`

## How To Use ##

### 1. Task ###

### 2. TasksQueue ###

### 3. TasksQueueContainer ###

### 4. ResourcePool ###
