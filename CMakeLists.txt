cmake_minimum_required(VERSION 3.5)
project(avr-db CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -pedantic")

include_directories(simulator/include)

add_subdirectory(simulator simulator/build)
add_executable(avr-db main.cpp)
target_link_libraries(avr-db simulator)
