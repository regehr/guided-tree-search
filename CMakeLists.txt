cmake_minimum_required(VERSION 3.10)

project(Guide)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wwrite-strings -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined,leak -D_DEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(tester tester.cpp)

add_library(regex STATIC regex.cpp)
add_executable(regex_test regex_test.cpp)
target_link_libraries(regex_test regex)

if (AFLPLUSPLUS_DIR)
  add_library(aflplusplus-mutator SHARED aflplusplus-mutator.cpp)
   target_include_directories(aflplusplus-mutator SYSTEM PUBLIC "${AFLPLUSPLUS_DIR}/include" "${AFLPLUSPLUS_DIR}/custom_mutators/examples")
endif()

