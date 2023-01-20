# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

project(project4)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads REQUIRED)

add_subdirectory(external)
add_subdirectory(src)
add_subdirectory(test)