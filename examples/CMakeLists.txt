cmake_minimum_required(VERSION 3.5)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/../avr-arduino-cmake/avr-arduino-toolchain.cmake)
project(avr-db-examples)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Werror")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

set(PROGRAMMER arduino)
set(SERIAL_DEV /dev/ttyACM0)

add_executable_avr(blink blink.c)
