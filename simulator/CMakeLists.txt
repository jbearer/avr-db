cmake_minimum_required(VERSION 3.5)

project(simulator CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

include_directories(include)
include_directories(../)

file(GLOB project_sources ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(simulator src/segment.cpp)
