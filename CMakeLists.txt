cmake_minimum_required(VERSION 3.16)
project(poller)

set(CMAKE_CXX_STANDARD 17)

add_library(poller poller.cpp Callback.cpp)
