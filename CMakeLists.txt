cmake_minimum_required(VERSION 3.0.0)

project(echoserver)

add_executable(echoserver main.cpp)
target_link_libraries(echoserver pthread)
