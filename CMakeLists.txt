cmake_minimum_required(VERSION 3.20)
project(LittleShell)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(LittleShell
        LittleShell.cpp
        LittleShell.hpp)
