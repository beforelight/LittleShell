cmake_minimum_required(VERSION 3.20)
project(LittleShell)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DCLI_DEBUG)

include_directories(.)
include_directories(TelnetServLib)

add_executable(TelnetShell TelnetShell.cpp LittleShell.cpp TelnetServLib/telnetservlib.cpp )
