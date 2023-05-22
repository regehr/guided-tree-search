cmake_minimum_required(VERSION 3.18)

project(Guide)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wwrite-strings -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined -D_DEBUG -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(include)
add_library(gen_regex STATIC tests/gen_regex.cpp)

add_executable(regex_test tests/regex_test.cpp)
target_link_libraries(regex_test gen_regex)

add_executable(saver_test tests/saver_test.cpp)
target_link_libraries(saver_test gen_regex)

if (AFLPLUSPLUS_DIR)
  add_library(aflplusplus-mutator SHARED aflplusplus/aflplusplus-mutator.cpp)
   target_include_directories(aflplusplus-mutator SYSTEM PUBLIC "${AFLPLUSPLUS_DIR}/include" "${AFLPLUSPLUS_DIR}/custom_mutators/examples")
endif()

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.0-preview4
)

FetchContent_MakeAvailable(Catch2)

add_executable(runtests tests/test.cpp)
target_link_libraries(runtests PRIVATE Catch2::Catch2WithMain)

enable_testing()
add_test(NAME main_test COMMAND runtests)
add_test(NAME saver_test COMMAND saver_test)
add_test(NAME regex_test COMMAND regex_test)
