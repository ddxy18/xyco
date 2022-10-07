cmake_minimum_required(VERSION 3.16.0)

project(
  xyco
  VERSION 0.1.0
  DESCRIPTION "C++20 based coroutine library"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "-std=c++20 -stdlib=libc++ -pthread")

include(FetchContent)
FetchContent_Declare(googletest
                     GIT_REPOSITORY "https://github.com/google/googletest.git"
                     GIT_TAG main)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(GSL
                     GIT_REPOSITORY "https://github.com/microsoft/GSL.git"
                     GIT_TAG main)
FetchContent_MakeAvailable(GSL)
FetchContent_Declare(spdlog
                     GIT_REPOSITORY "https://github.com/gabime/spdlog.git"
                     GIT_TAG v1.x)
FetchContent_MakeAvailable(spdlog)
FetchContent_Declare(asio
                     GIT_REPOSITORY "https://github.com/chriskohlhoff/asio.git"
                     GIT_TAG master)
FetchContent_MakeAvailable(asio)

include_directories(src)
include_directories(/usr/include/libunwind)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmark)
add_subdirectory(examples)