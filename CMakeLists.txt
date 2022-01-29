cmake_minimum_required(VERSION 3.10)

project(Uniform)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined -D_DEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(tester tester.cpp)
