# Tasks Lib #

This is a very simple tasks scheduler library, using thread pools.

It is written with C++17 specs, has no dependencies on any external libraries and serves the singular purpose of running executable pieces of code multi-threaded, while avoiding creation and destruction of threads.

## Small Footprint, Multiplatform And Production Ready ##

TasksLib was created as part of the development of a mobile game which is being released on the iOS, Android and Windows platforms. The library is used extensively both in the client and the in the server, which is a Linux application. There are some structural differences between the version that is used in the game and this project, mainly because we tend to use game engine objects there when available, and we write C++11 code for the game, but most of the code is the same and is in daily use.

## How To Build ##

## How To Use ##

### 1. Task ###

### 2. TasksQueue ###

### 3. TasksQueueContainer ###

### 4. ResourcePool ###
