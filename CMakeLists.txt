cmake_minimum_required(VERSION 3.10)
project(RingBufferProject)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(RingBuffer src/main.cpp src/RingBuffer.cpp) 