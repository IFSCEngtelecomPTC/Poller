cmake_minimum_required(VERSION 3.22)
project(poller)

set(CMAKE_CXX_STANDARD 17)

add_library(poller poller.cpp Callback.cpp)
