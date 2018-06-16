# Tasks Lib #

This is a very simple tasks scheduler library, using thread pools.

It is written with C++17 specs, has no dependencies on any external libraries and serves the singular purpose of running executable pieces of code multi-threaded, while avoiding creation and destruction of threads.

## Small Footprint, Multiplatform And Production Ready ##

TasksLib was created as part of the development of a mobile game which is being released on the iOS, Android and Windows platforms. The library is used extensively both in the client and the in the server, which is a Linux application. There are some structural differences between the version that is used in the game and this project, mainly because we tend to use game engine objects there when available, and we write C++11 code for the game, but most of the code is the same and is in daily use.

***Note:*** *This particular version of the library is ported from the original and at this point it is developed and tested only on **Windows** with **MS Visual Studio 2017**. Getting it to compile and testing it on **Linux** is being worked on. There are no guarantees however for **MacOS** and **Android**, unless somebody helps to compile and test the library on these platforms - it is based on code that is used on all platforms and should be relatively easy to make it conform, but that is not done at this stage.*

## How To Use ##

1. [Build Instructions](docs/Build.md)
2. [Architecture & Tutorial](docs/Architecture.adoc)
3. [API Reference](docs/Api.md)
