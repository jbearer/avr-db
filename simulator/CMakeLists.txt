cmake_minimum_required(VERSION 3.5)

project(simulator CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

include_directories(include)
include_directories(../)

file(GLOB_RECURSE SIMULATOR_CXX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_library(simulator SHARED ${SIMULATOR_CXX_SOURCE})

add_executable(segment_test src/segment_test.cpp)
target_link_libraries(segment_test simulator)

# GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

file(GLOB_RECURSE SIMULATOR_TEST_CXX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/test/*.cpp)
add_executable(test-simulator ${SIMULATOR_TEST_CXX_SOURCE})
target_link_libraries(test-simulator simulator)
target_link_libraries(test-simulator ${GTEST_BOTH_LIBRARIES})
target_link_libraries(test-simulator ${CMAKE_THREAD_LIBS_INIT})
